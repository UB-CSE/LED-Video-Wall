package main

import (
	"log"
	"net/url"
	"time"

	"github.com/gorilla/websocket"
)

// Simple program to test sending a websocket message to the webserver.

func main() {

	u := url.URL{Scheme: "ws", Host: "localhost:5000", Path: "/api/video-wall/p3-demo-wall"}
	log.Printf("connecting to %s", u.String())

	c, _, err := websocket.DefaultDialer.Dial(u.String(), nil)
	if err != nil {
		log.Fatal("dial:", err)
	}
	defer func() { _ = c.Close() }()

	done := make(chan struct{})

	gotResponse := false

	go func() {
		defer close(done)
		for {
			_, message, err := c.ReadMessage()
			if err != nil {
				log.Println("read:", err)
				return
			}
			log.Printf("response:\n\n%s\n\n", message)
			gotResponse = true
		}
	}()

	command := LEDVWCommand{}
	command.Name = "canvas-get-elements"

	//command.Name = "canvas-load"
	//command.Args = append(command.Args, "input.yaml")

	err = c.WriteJSON(command)
	if err != nil {
		log.Println("write:", err)
		return
	}

	deadline := time.Now().Add(time.Second * 5)
	for !gotResponse && time.Now().Before(deadline) {
		time.Sleep(10 * time.Millisecond)
	}
	if !gotResponse {
		log.Println("no response")
	}
}
