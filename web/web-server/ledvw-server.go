package main

import (
	"bufio"
	"encoding/binary"
	"errors"
	"io"
	"log"
	"net"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"sync"
	"time"
)

type LEDVWServer struct {
	Name             string
	MatrixConfigPath string
	CmdSocketPath    string

	// Commands to send to the LEDVW server
	SendChannel chan string

	// Responses and notifications from the LEDVW server
	ReceiveChannel chan string
}

func (s *LEDVWServer) IsActive() bool {
	_, err := os.Stat(s.CmdSocketPath)
	return err == nil
}

type LEDVWServerManager struct {
	runningServers map[string]*LEDVWServer
	mutex          sync.Mutex
}

func NewLEDVWServerManager() *LEDVWServerManager {
	m := &LEDVWServerManager{}
	m.runningServers = make(map[string]*LEDVWServer)
	return m
}

func GetMatrixConfigFile(name string, serverConfig *ServerConfig) (string, bool) {
	matrixConfigPath := filepath.Join(serverConfig.MatrixConfigsDir, name+".yaml")
	if _, err := os.Stat(matrixConfigPath); err != nil {
		return "", false
	}

	return matrixConfigPath, true
}

func (m *LEDVWServerManager) isServerRunning(name string) bool {
	m.mutex.Lock()
	_, ok := m.runningServers[name]
	m.mutex.Unlock()
	return ok
}

func (m *LEDVWServerManager) addServer(name string, server *LEDVWServer) {
	m.mutex.Lock()
	m.runningServers[name] = server
	m.mutex.Unlock()
}

func (m *LEDVWServerManager) removeServer(name string) {
	m.mutex.Lock()
	delete(m.runningServers, name)
	m.mutex.Unlock()
}

func (m *LEDVWServerManager) monitorServerProcess(name string, command *exec.Cmd) {
	_ = command.Wait()
	log.Println("LEDVW server " + name + " stopped")
	m.removeServer(name)
}

func (m *LEDVWServerManager) monitorServerUnixSocket(name string) {
	isConnected := false

	var conn net.Conn
	var reader *bufio.Reader
	var writer *bufio.Writer
	for {
		m.mutex.Lock()
		server, ok := m.runningServers[name]
		m.mutex.Unlock()

		// Server stopped
		if !ok {
			_ = conn.Close()
			return
		}

		// Check if the socket file exists yet.
		if _, err := os.Stat(server.CmdSocketPath); err == nil {
			if !isConnected {
				log.Println("LEDVW server " + name + " socket created")
			}
		} else if isConnected {
			log.Println("LEDVW server " + name + " socket destroyed")
			_ = conn.Close()
			return
		} else {
			// Wait for the socket to get created
			time.Sleep(time.Millisecond * 50)
			continue
		}

		if !isConnected {
			var err error
			conn, err = net.DialTimeout("unix", server.CmdSocketPath, time.Second)
			if err != nil {
				log.Println("LEDVW server " + name + " failed to connect: " + err.Error())
				continue
			}

			log.Println("LEDVW server " + name + " connected")
			isConnected = true

			reader = bufio.NewReader(conn)
			writer = bufio.NewWriter(conn)
		}

		if isConnected {
			_ = conn.SetReadDeadline(time.Now().Add(time.Millisecond))

			var messageSize uint32
			err := binary.Read(reader, binary.NativeEndian, &messageSize)
			deadlineReached := errors.Is(err, os.ErrDeadlineExceeded)
			if err != nil && !deadlineReached {
				log.Println("LEDVW server " + name + " failed to read message size: " + err.Error())
				_ = conn.Close()
				isConnected = false
				continue
			}

			if !deadlineReached {
				_ = conn.SetReadDeadline(time.Time{})

				if messageSize > 0 { // Occasional heartbeat messages send 0 size
					messageBuffer := make([]byte, messageSize)
					_, err := io.ReadFull(reader, messageBuffer)
					if err != nil {
						log.Println("LEDVW server " + name + " failed to read message: " + err.Error())
						_ = conn.Close()
						isConnected = false
						continue
					}

					messageString := string(messageBuffer)
					server.ReceiveChannel <- messageString
				}
			}

			select {
			case messageString := <-server.SendChannel:
				messageSize = uint32(len(messageString))
				err = binary.Write(writer, binary.NativeEndian, messageSize)
				if err != nil {
					log.Println("LEDVW server " + name + " failed to write message size: " + err.Error())
					_ = conn.Close()
					isConnected = false
					continue
				}

				_, err = writer.WriteString(messageString)
				if err != nil {
					log.Println("LEDVW server " + name + " failed to write message: " + err.Error())
					_ = conn.Close()
					isConnected = false
					continue
				}
				err = writer.Flush()
				if err != nil {
					log.Println("LEDVW server " + name + " failed to flush message: " + err.Error())
				}

			default:
			}
		}
	}
}

func (m *LEDVWServerManager) StartServer(name string, serverConfig *ServerConfig) bool {
	matrixConfigPath, ok := GetMatrixConfigFile(name, serverConfig)
	if !ok {
		return false
	}

	if m.isServerRunning(name) {
		return true
	}

	server := &LEDVWServer{}
	server.Name = name
	server.MatrixConfigPath = matrixConfigPath

	server.SendChannel = make(chan string)
	server.ReceiveChannel = make(chan string)

	// Start the server

	args := []string{matrixConfigPath}

	if serverConfig.DisableGPU {
		args = append(args, "--disable-gpu")
	}
	if !serverConfig.DebugMode {
		args = append(args, "--prod")
	}

	command := exec.Command(serverConfig.VWServerPath, args...)
	if err := command.Start(); err != nil {
		return false
	}
	server.CmdSocketPath = "/tmp/ledvw-cmd-" + name

	m.addServer(name, server)

	go m.monitorServerProcess(name, command)
	go m.monitorServerUnixSocket(name)

	return true
}

type LEDVWCommand struct {
	Name string   `json:"command-name"`
	Args []string `json:"args"`
}

func (m *LEDVWServerManager) RunCommand(command *LEDVWCommand, name string) (string, bool) {
	server, ok := m.runningServers[name]
	if !ok {
		return "", false
	}

	var commandString strings.Builder
	commandString.WriteString(command.Name)

	for _, arg := range command.Args {
		commandString.WriteString(" \"" + arg + "\"")
	}

	// TODO: Validate arguments for the command and adjust filesystem paths to point to be relative to resource locations

	server.SendChannel <- commandString.String()

	response := <-server.ReceiveChannel // TODO: Don't wait forever for a response!
	return response, true
}
