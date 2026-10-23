package main

import (
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
