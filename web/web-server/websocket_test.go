package main

import (
	"log"
	"net/url"
	"testing"
	"time"

	"github.com/gorilla/websocket"
)

// Run this while the webserver is running

func TestWebsockets(t *testing.T) {
	time.Sleep(50 * time.Millisecond)

	u := url.URL{Scheme: "ws", Host: "localhost:5000", Path: "/api/video-wall/p3-demo-wall"}
	log.Printf("connecting to %s", u.String())

	c, _, err := websocket.DefaultDialer.Dial(u.String(), nil)
	if err != nil {
		t.Fatal(err)
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
	//command.Name = "canvas-get-elements"

	command.Name = "canvas-load"
	command.Args = append(command.Args, "input.yaml")

	err = c.WriteJSON(command)
	if err != nil {
		t.Fatal(err)
	}

	deadline := time.Now().Add(time.Second * 5)
	for !gotResponse && time.Now().Before(deadline) {
		time.Sleep(10 * time.Millisecond)
	}
	if !gotResponse {
		t.Fatal("no response")
	}
}
