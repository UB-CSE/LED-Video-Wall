package main

import (
	"crypto/sha256"
	"encoding/hex"
	"encoding/json"
	"errors"
	"io"
	"io/fs"
	"log"
	"net/http"
	"os"
	"path/filepath"
	"strings"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/gorilla/websocket"
)

var upgrader = websocket.Upgrader{}

const (
	wsWriteWait  = 10 * time.Second
	wsPongWait   = 20 * time.Second
	wsPingPeriod = (wsPongWait * 9) / 10
)

func startWebsocketKeepalive(wsConn *websocket.Conn) func() {
	_ = wsConn.SetReadDeadline(time.Now().Add(wsPongWait))
	wsConn.SetPongHandler(func(string) error {
		return wsConn.SetReadDeadline(time.Now().Add(wsPongWait))
	})

	done := make(chan struct{})
	go func() {
		ticker := time.NewTicker(wsPingPeriod)
		defer ticker.Stop()
		for {
			select {
			case <-done:
				return
			case <-ticker.C:
				err := wsConn.WriteControl(websocket.PingMessage, nil, time.Now().Add(wsWriteWait))
				if err != nil {
					log.Println("ping:", err)
					_ = wsConn.Close()
					return
				}
			}
		}
	}()

	return func() { close(done) }
}

var vwServerManager = NewLEDVWServerManager()

func getFileNames(dir string, extensions []string) []string {
	entries, err := os.ReadDir(dir)
	if err != nil {
		log.Fatal(err)
	}

	var names []string
	for _, entry := range entries {
		ext := filepath.Ext(entry.Name())
		if entry.IsDir() {
			continue
		}
		for e := range extensions {
			if strings.EqualFold(extensions[e], ext) {
				names = append(names, entry.Name())
				break
			}
		}
	}

	return names
}

func setupRouter(serverConfig *ServerConfig) *gin.Engine {
	r := gin.Default()

	r.GET("/ping", func(c *gin.Context) {
		c.String(http.StatusOK, "pong")
	})

	r.GET("/api/list-video-walls", func(c *gin.Context) {
		entries, err := os.ReadDir(serverConfig.MatrixConfigsDir)
		if err != nil {
			log.Fatal(err)
		}

		var names []string
		for _, entry := range entries {
			if !entry.IsDir() && strings.EqualFold(filepath.Ext(entry.Name()), ".yaml") {
				name := strings.TrimSuffix(entry.Name(), filepath.Ext(entry.Name()))
				names = append(names, name)
			}
		}

		c.JSON(http.StatusOK, names)
	})

	r.GET("/api/resources/:resource", func(ctx *gin.Context) {
		resourceName := ctx.Params.ByName("resource")
		ctx.File(filepath.Join(serverConfig.UserResourceUploadsDir, resourceName))
	})

	r.GET("/api/font/:font", func(ctx *gin.Context) {
		fontName := ctx.Params.ByName("font")
		ctx.File(filepath.Join(serverConfig.FontsDir, fontName))
	})

	r.GET("/api/list-fonts", func(c *gin.Context) {
		names := getFileNames(serverConfig.FontsDir, []string{".ttf"})
		c.JSON(http.StatusOK, names)
	})

	r.GET("/api/list-images", func(c *gin.Context) {
		names := getFileNames(serverConfig.UserResourceUploadsDir, []string{".png", ".jpg", ".jpeg"})
		c.JSON(http.StatusOK, names)
	})

	const maxUploadSize = 10 << 20 // 10 MB

	r.POST("/api/upload-image", func(c *gin.Context) {
		c.Request.Body = http.MaxBytesReader(
			c.Writer,
			c.Request.Body,
			maxUploadSize+(1<<20),
		)

		fileHeader, err := c.FormFile("file")
		if err != nil {
			c.JSON(http.StatusBadRequest, gin.H{
				"error": "Missing or invalid file upload",
			})
			return
		}

		if fileHeader.Size == 0 {
			c.JSON(http.StatusBadRequest, gin.H{"error": "Empty file"})
			return
		}

		if fileHeader.Size > maxUploadSize {
			c.JSON(http.StatusRequestEntityTooLarge, gin.H{
				"error": "File exceeds the 10 MB upload limit",
			})
			return
		}

		// Only allow PNG and JPEG images.
		ext := strings.ToLower(filepath.Ext(fileHeader.Filename))
		if ext != ".png" && ext != ".jpg" && ext != ".jpeg" {
			c.JSON(http.StatusBadRequest, gin.H{
				"error": "Only PNG and JPEG images are supported",
			})
			return
		}

		src, err := fileHeader.Open()
		if err != nil {
			c.JSON(http.StatusInternalServerError, gin.H{
				"error": "Failed to read uploaded file",
			})
			return
		}
		defer src.Close()

		// Verify that the file contents match the extension.
		header := make([]byte, 512)
		n, err := io.ReadFull(src, header)
		if err != nil && err != io.EOF && err != io.ErrUnexpectedEOF {
			c.JSON(http.StatusBadRequest, gin.H{"error": "Invalid image"})
			return
		}

		contentType := http.DetectContentType(header[:n])

		if (ext == ".png" && contentType != "image/png") ||
			((ext == ".jpg" || ext == ".jpeg") && contentType != "image/jpeg") {
			c.JSON(http.StatusBadRequest, gin.H{
				"error": "File contents do not match the image extension",
			})
			return
		}

		// Rewind the uploaded file.
		if _, err := src.Seek(0, io.SeekStart); err != nil {
			c.JSON(http.StatusInternalServerError, gin.H{
				"error": "Failed to read uploaded file",
			})
			return
		}

		// Ensure the upload directory exists.
		if err = os.MkdirAll(serverConfig.UserResourceUploadsDir, 0755); err != nil {
			c.JSON(http.StatusInternalServerError, gin.H{
				"error": "Failed to create upload directory",
			})
			return
		}

		// Save to a temporary file in the upload directory while hashing the
		// contents. The temporary name's extension keeps it out of the image list.
		tmp, err := os.CreateTemp(serverConfig.UserResourceUploadsDir, ".upload-*.tmp")
		if err != nil {
			c.JSON(http.StatusInternalServerError, gin.H{
				"error": "Failed to save image",
			})
			return
		}
		tmpPath := tmp.Name()
		defer func() { _ = os.Remove(tmpPath) }()

		hasher := sha256.New()
		written, copyErr := io.Copy(io.MultiWriter(tmp, hasher), io.LimitReader(src, maxUploadSize+1))
		closeErr := tmp.Close()

		if copyErr != nil || closeErr != nil || written > maxUploadSize {
			c.JSON(http.StatusInternalServerError, gin.H{
				"error": "Failed to save image",
			})
			return
		}

		// Name the image after its contents so identical images share one file.
		// The extension comes from the detected type, so the same JPEG uploaded
		// as .jpg and .jpeg is still recognized as a duplicate.
		canonicalExt := ".png"
		if contentType == "image/jpeg" {
			canonicalExt = ".jpg"
		}
		filename := hex.EncodeToString(hasher.Sum(nil)) + canonicalExt
		dstPath := filepath.Join(serverConfig.UserResourceUploadsDir, filename)

		// Linking fails if the destination exists, so this is an atomic
		// "create unless a duplicate already exists", even for concurrent uploads.
		if err := os.Link(tmpPath, dstPath); err != nil {
			if errors.Is(err, fs.ErrExist) {
				c.JSON(http.StatusOK, gin.H{
					"filename":  filename,
					"duplicate": true,
				})
				return
			}

			c.JSON(http.StatusInternalServerError, gin.H{
				"error": "Failed to save image",
			})
			return
		}

		// Return the image's filename to the frontend.
		c.JSON(http.StatusCreated, gin.H{
			"filename":  filename,
			"duplicate": false,
		})
	})

	r.GET("/api/list-config-files", func(c *gin.Context) {
		entries, err := os.ReadDir(serverConfig.UserCanvasConfigsDir)
		if err != nil {
			log.Fatal(err)
		}

		var names []string
		for _, entry := range entries {
			if !entry.IsDir() && strings.EqualFold(filepath.Ext(entry.Name()), ".yaml") {
				names = append(names, entry.Name())
			}
		}

		c.JSON(http.StatusOK, names)
	})

	r.GET("/api/video-wall/:name", func(ctx *gin.Context) {
		wallName := ctx.Params.ByName("name")

		_, ok := GetMatrixConfigFile(wallName, serverConfig)
		if !ok {
			ctx.AbortWithStatus(http.StatusNotFound)
			return
		}

		wsConn, err := upgrader.Upgrade(ctx.Writer, ctx.Request, nil)
		if err != nil {
			log.Println("upgrade:", err)
			return
		}

		defer func() { _ = wsConn.Close() }()

		ok = vwServerManager.StartServer(wallName, serverConfig)
		if !ok {
			log.Println("failed to start server for wall", wallName)
			return
		}

		stopKeepalive := startWebsocketKeepalive(wsConn)
		defer stopKeepalive()

		for {
			command := &LEDVWCommand{}
			err := wsConn.ReadJSON(command)
			if err != nil {
				log.Println("read:", err)
				break
			}

			response, err := vwServerManager.RunCommand(command, wallName)
			serverStopped := errors.Is(err, ErrServerNotRunning)
			if err != nil {
				log.Printf("failed to run command %q on wall %s: %v", command.Name, wallName, err)

				// Still send exactly one response per command so the client doesn't
				// mismatch later responses.
				errorResponse, _ := json.Marshal(map[string]any{
					"success": false,
					"error":   err.Error(),
				})
				response = string(errorResponse)
			}

			// Pongs aren't processed while blocked on a command, so give the client a
			// fresh window once the command completes.
			_ = wsConn.SetReadDeadline(time.Now().Add(wsPongWait))

			err = wsConn.WriteMessage(websocket.TextMessage, []byte(response))
			if err != nil {
				log.Println("write:", err)
				break
			}

			if serverStopped {
				break
			}
		}
	})

	return r
}

func main() {
	log.SetPrefix("[LEDVW-Web-Server] ")

	serverConfig := GetServerConfig()
	if serverConfig.DebugMode {
		gin.SetMode(gin.DebugMode)
	} else {
		gin.SetMode(gin.ReleaseMode)
	}

	r := setupRouter(serverConfig)

	_ = r.Run(":5000")
}
