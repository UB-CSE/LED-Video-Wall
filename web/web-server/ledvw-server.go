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

const (
	commandResponseTimeout = 5 * time.Second
	serverConnectTimeout   = 30 * time.Second
)

var (
	ErrServerNotRunning = errors.New("video wall server is not running")
	ErrNotConnected     = errors.New("timed out connecting to video wall server")
	ErrDisconnected     = errors.New("video wall server disconnected before responding")
	ErrResponseTimeout  = errors.New("timed out waiting for video wall server response")
	ErrEmptyCommand     = errors.New("empty command")
)

/**
 * ledvwConnection represents a single connection to an LEDVW server's command
 * socket.
 *
 * The LEDVW server processes commands in the order it receives them and sends
 * exactly one response per command, so responses are matched to commands in
 * FIFO order. A command that times out keeps its place in the queue, so its
 * late response is discarded instead of being mistaken for the response to a
 * later command.
 */
type ledvwConnection struct {
	conn net.Conn

	mutex   sync.Mutex
	writer  *bufio.Writer
	pending []chan string // One entry per command awaiting a response.
	closed  bool
}

func newLEDVWConnection(conn net.Conn) *ledvwConnection {
	return &ledvwConnection{conn: conn, writer: bufio.NewWriter(conn)}
}

// send writes a command and returns a channel that receives its response. The
// channel is closed without a value if the connection is lost first.
func (c *ledvwConnection) send(command string) (<-chan string, error) {
	c.mutex.Lock()
	defer c.mutex.Unlock()

	if c.closed {
		return nil, ErrDisconnected
	}

	_ = c.conn.SetWriteDeadline(time.Now().Add(commandResponseTimeout))

	err := binary.Write(c.writer, binary.NativeEndian, uint32(len(command)))
	if err == nil {
		_, err = c.writer.WriteString(command)
	}
	if err == nil {
		err = c.writer.Flush()
	}
	if err != nil {
		// A partially written message would desynchronize the stream, so drop the connection. The reader will notice and clean up.
		_ = c.conn.Close()
		return nil, err
	}

	responseChannel := make(chan string, 1)
	c.pending = append(c.pending, responseChannel)
	return responseChannel, nil
}

// deliver hands a response to the oldest command awaiting one.
func (c *ledvwConnection) deliver(response string) bool {
	c.mutex.Lock()
	defer c.mutex.Unlock()

	if len(c.pending) == 0 {
		return false
	}

	responseChannel := c.pending[0]
	c.pending[0] = nil
	c.pending = c.pending[1:]

	// Buffered with capacity 1 and written exactly once, so this never blocks,
	// even if the command already timed out and nobody is listening.
	responseChannel <- response
	return true
}

// close closes the connection and fails every command still awaiting a response.
func (c *ledvwConnection) close() {
	c.mutex.Lock()
	defer c.mutex.Unlock()

	if c.closed {
		return
	}
	c.closed = true
	_ = c.conn.Close()

	for _, responseChannel := range c.pending {
		close(responseChannel)
	}
	c.pending = nil
}

// readMessages reads messages until the connection fails.
func (c *ledvwConnection) readMessages(name string) {
	reader := bufio.NewReader(c.conn)
	for {
		var messageSize uint32
		if err := binary.Read(reader, binary.NativeEndian, &messageSize); err != nil {
			if !errors.Is(err, io.EOF) && !errors.Is(err, net.ErrClosed) {
				log.Println("LEDVW server " + name + " failed to read message size: " + err.Error())
			}
			return
		}

		// The server sends empty heartbeat messages periodically.
		if messageSize == 0 {
			continue
		}

		messageBuffer := make([]byte, messageSize)
		if _, err := io.ReadFull(reader, messageBuffer); err != nil {
			log.Println("LEDVW server " + name + " failed to read message: " + err.Error())
			return
		}

		if !c.deliver(string(messageBuffer)) {
			log.Println("LEDVW server " + name + " sent a message with no pending command, discarding it")
		}
	}
}

type LEDVWServer struct {
	Name             string
	MatrixConfigPath string
	CmdSocketPath    string

	// Closed when the server process exits.
	stopped chan struct{}

	connMutex  sync.Mutex
	connection *ledvwConnection
	// Closed and replaced whenever connection changes.
	connChanged chan struct{}
}

func newLEDVWServer(name, matrixConfigPath string) *LEDVWServer {
	return &LEDVWServer{
		Name:             name,
		MatrixConfigPath: matrixConfigPath,
		CmdSocketPath:    "/tmp/ledvw-cmd-" + name,
		stopped:          make(chan struct{}),
		connChanged:      make(chan struct{}),
	}
}

func (s *LEDVWServer) IsActive() bool {
	_, err := os.Stat(s.CmdSocketPath)
	return err == nil
}

func (s *LEDVWServer) setConnection(c *ledvwConnection) {
	s.connMutex.Lock()
	s.connection = c
	close(s.connChanged)
	s.connChanged = make(chan struct{})
	s.connMutex.Unlock()
}

func (s *LEDVWServer) currentConnection() *ledvwConnection {
	s.connMutex.Lock()
	defer s.connMutex.Unlock()
	return s.connection
}

func (s *LEDVWServer) waitForConnection(timeout time.Duration) (*ledvwConnection, error) {
	timer := time.NewTimer(timeout)
	defer timer.Stop()

	for {
		s.connMutex.Lock()
		connection, changed := s.connection, s.connChanged
		s.connMutex.Unlock()

		if connection != nil {
			return connection, nil
		}

		select {
		case <-changed:
		case <-s.stopped:
			return nil, ErrServerNotRunning
		case <-timer.C:
			return nil, ErrNotConnected
		}
	}
}

// sleep waits for the given duration, returning false if the server stopped.
func (s *LEDVWServer) sleep(duration time.Duration) bool {
	select {
	case <-s.stopped:
		return false
	case <-time.After(duration):
		return true
	}
}

// manageConnection keeps a connection to the server's command socket open,
// reconnecting if it drops, until the server process exits.
func (s *LEDVWServer) manageConnection() {
	for {
		select {
		case <-s.stopped:
			return
		default:
		}

		// Wait for the socket to get created
		if !s.IsActive() {
			if !s.sleep(50 * time.Millisecond) {
				return
			}
			continue
		}

		conn, err := net.DialTimeout("unix", s.CmdSocketPath, time.Second)
		if err != nil {
			log.Println("LEDVW server " + s.Name + " failed to connect: " + err.Error())
			if !s.sleep(50 * time.Millisecond) {
				return
			}
			continue
		}

		log.Println("LEDVW server " + s.Name + " connected")

		connection := newLEDVWConnection(conn)
		s.setConnection(connection)

		// The server may have stopped while we were connecting, in which case
		// nothing else will close this connection.
		select {
		case <-s.stopped:
			connection.close()
		default:
		}

		connection.readMessages(s.Name)

		s.setConnection(nil)
		connection.close()

		log.Println("LEDVW server " + s.Name + " disconnected")
	}
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

func (m *LEDVWServerManager) getServer(name string) (*LEDVWServer, bool) {
	m.mutex.Lock()
	server, ok := m.runningServers[name]
	m.mutex.Unlock()
	return server, ok
}

func (m *LEDVWServerManager) isServerRunning(name string) bool {
	_, ok := m.getServer(name)
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

func (m *LEDVWServerManager) monitorServerProcess(server *LEDVWServer, command *exec.Cmd) {
	_ = command.Wait()
	log.Println("LEDVW server " + server.Name + " stopped")
	m.removeServer(server.Name)

	close(server.stopped)
	if connection := server.currentConnection(); connection != nil {
		connection.close()
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

	server := newLEDVWServer(name, matrixConfigPath)

	// Start the server

	args := []string{matrixConfigPath}

	if serverConfig.DisableGPU {
		args = append(args, "--disable-gpu")
	}
	if !serverConfig.DebugMode {
		args = append(args, "--prod")
	}
	args = append(args, "--canvas-configs-dir", serverConfig.UserCanvasConfigsDir)
	args = append(args, "--fonts-dir", serverConfig.FontsDir)
	args = append(args, "--resources-dir", serverConfig.UserResourceUploadsDir)
	args = append(args, "--logs-dir", serverConfig.LogsDir)

	command := exec.Command(serverConfig.VWServerPath, args...)
	if err := command.Start(); err != nil {
		return false
	}

	m.addServer(name, server)

	go m.monitorServerProcess(server, command)
	go server.manageConnection()

	return true
}

type LEDVWCommand struct {
	Name string   `json:"command-name"`
	Args []string `json:"args"`
}

// RunCommand sends a command to the named LEDVW server and waits for its
// response, failing if none arrives within commandResponseTimeout.
func (m *LEDVWServerManager) RunCommand(command *LEDVWCommand, name string) (string, error) {
	// The LEDVW server ignores empty messages, so it would never respond.
	if command.Name == "" {
		return "", ErrEmptyCommand
	}

	server, ok := m.getServer(name)
	if !ok {
		return "", ErrServerNotRunning
	}

	connection, err := server.waitForConnection(serverConnectTimeout)
	if err != nil {
		return "", err
	}

	var commandString strings.Builder
	commandString.WriteString(command.Name)

	for _, arg := range command.Args {
		commandString.WriteString(" \"" + arg + "\"")
	}

	responseChannel, err := connection.send(commandString.String())
	if err != nil {
		return "", err
	}

	timer := time.NewTimer(commandResponseTimeout)
	defer timer.Stop()

	select {
	case response, ok := <-responseChannel:
		if !ok {
			return "", ErrDisconnected
		}
		return response, nil
	case <-timer.C:
		return "", ErrResponseTimeout
	}
}
