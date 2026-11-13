package main

import (
	"crypto/rand"
	"encoding/hex"
	"io"
	"log"
	"net/http"
	"os"
	"path/filepath"
	"strings"

	"github.com/gin-gonic/gin"
	"github.com/gorilla/websocket"
)

var upgrader = websocket.Upgrader{}
var vwServerManager = NewLEDVWServerManager()

func newUUID() (string, error) {
	var b [16]byte

	if _, err := rand.Read(b[:]); err != nil {
		return "", err
	}

	// Set UUID version (4) and variant bits.
	b[6] = (b[6] & 0x0f) | 0x40
	b[8] = (b[8] & 0x3f) | 0x80

	return hex.EncodeToString(b[0:4]) + "-" +
		hex.EncodeToString(b[4:6]) + "-" +
		hex.EncodeToString(b[6:8]) + "-" +
		hex.EncodeToString(b[8:10]) + "-" +
		hex.EncodeToString(b[10:16]), nil
}

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

		// Generate a unique filename.
		id, err := newUUID()
		if err != nil {
			c.JSON(http.StatusInternalServerError, gin.H{
				"error": "Failed to generate filename",
			})
			return
		}

		filename := id + ext
		dstPath := filepath.Join(serverConfig.UserResourceUploadsDir, filename)

		// Ensure the upload directory exists.
		if err = os.MkdirAll(serverConfig.UserResourceUploadsDir, 0755); err != nil {
			c.JSON(http.StatusInternalServerError, gin.H{
				"error": "Failed to create upload directory",
			})
			return
		}

		// O_EXCL prevents accidentally overwriting an existing file.
		dst, err := os.OpenFile(dstPath, os.O_WRONLY|os.O_CREATE|os.O_EXCL, 0644)
		if err != nil {
			c.JSON(http.StatusInternalServerError, gin.H{
				"error": "Failed to save image",
			})
			return
		}

		written, copyErr := io.Copy(dst, io.LimitReader(src, maxUploadSize+1))
		closeErr := dst.Close()

		if copyErr != nil || closeErr != nil || written > maxUploadSize {
			os.Remove(dstPath)
			c.JSON(http.StatusInternalServerError, gin.H{
				"error": "Failed to save image",
			})
			return
		}

		// Return the generated filename to the frontend.
		c.JSON(http.StatusCreated, gin.H{
			"filename": filename,
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

		for {
			command := &LEDVWCommand{}
			err := wsConn.ReadJSON(command)
			if err != nil {
				log.Println("read:", err)
				break
			}

			response, ok := vwServerManager.RunCommand(command, wallName)
			if !ok {
				log.Println("failed to run command on wall", wallName)
				break
			}

			err = wsConn.WriteMessage(websocket.TextMessage, []byte(response))
			if err != nil {
				log.Println("write:", err)
				break
			}
		}
	})

	return r
}

func main() {
	log.SetPrefix("[LEDVW-Web-Server] ")

	serverConfig := GetServerConfig()
	r := setupRouter(serverConfig)

	_ = r.Run(":5000")
}
