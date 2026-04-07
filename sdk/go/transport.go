package stdui

import (
	"bufio"
	"fmt"
	"net"
	"os/exec"
	"time"
)

// StartWithSocket spawns the stdui binary with the --socket flag, waits for it
// to create the Unix domain socket (or Windows AF_UNIX socket), connects to
// it, sends the initial settings, and begins reading events.
//
// The socket file is created by the stdui process; the caller must not create
// it beforehand. The file is removed by stdui on shutdown.
//
// socketPath is the filesystem path for the socket, e.g. "/tmp/myapp.sock".
// On Windows this must be a path on a local volume; the same Windows 10 1803+
// AF_UNIX support that stdui relies on is used here.
//
// The binary path may be absolute or relative to the working directory of the
// calling process.
func StartWithSocket(binaryPath string, socketPath string, settings Settings) (*Client, error) {
	cmd := exec.Command(binaryPath, "--socket", socketPath)

	if err := cmd.Start(); err != nil {
		return nil, fmt.Errorf("stdui: start process: %w", err)
	}

	// The stdui process needs a moment to create and bind the socket before we
	// can connect. We retry with a short back-off for up to 5 seconds.
	conn, err := dialRetry("unix", socketPath, 5*time.Second)
	if err != nil {
		cmd.Process.Kill() //nolint:errcheck
		return nil, fmt.Errorf("stdui: connect to socket %s: %w", socketPath, err)
	}

	return newClientFromConn(cmd, conn, settings)
}

// StartWithNamedPipe spawns the stdui binary with the --pipe flag, waits for
// it to create the named pipe, connects to it, sends the initial settings,
// and begins reading events.
//
// On Unix/macOS, pipePath is the filesystem path of the Unix domain socket
// that stdui creates as the pipe backend, e.g. "/tmp/myapp.pipe". FIFOs are
// not used because they cannot carry bidirectional IPC reliably.
//
// On Windows, pipePath must be a valid Windows named-pipe path of the form
// \\.\pipe\<name>, e.g. `\\.\pipe\myapp`.
//
// The binary path may be absolute or relative to the working directory of the
// calling process.
func StartWithNamedPipe(binaryPath string, pipePath string, settings Settings) (*Client, error) {
	cmd := exec.Command(binaryPath, "--pipe", pipePath)

	if err := cmd.Start(); err != nil {
		return nil, fmt.Errorf("stdui: start process: %w", err)
	}

	conn, err := openNamedPipe(pipePath, 5*time.Second)
	if err != nil {
		cmd.Process.Kill() //nolint:errcheck
		return nil, fmt.Errorf("stdui: connect to pipe %s: %w", pipePath, err)
	}

	return newClientFromConn(cmd, conn, settings)
}

// dialRetry tries net.Dial(network, address) repeatedly until it succeeds or
// timeout elapses. The back-off starts at 10ms and doubles up to 250ms.
func dialRetry(network, address string, timeout time.Duration) (net.Conn, error) {
	deadline := time.Now().Add(timeout)
	delay := 10 * time.Millisecond
	for {
		conn, err := net.Dial(network, address)
		if err == nil {
			return conn, nil
		}
		if time.Now().After(deadline) {
			return nil, err
		}
		time.Sleep(delay)
		delay *= 2
		if delay > 250*time.Millisecond {
			delay = 250 * time.Millisecond
		}
	}
}

// newClientFromConn creates a Client that communicates over an already-open
// net.Conn (socket or pipe) instead of stdin/stdout.
func newClientFromConn(cmd *exec.Cmd, conn net.Conn, settings Settings) (*Client, error) {
	c := &Client{
		cmd:   cmd,
		stdin: &connWriteCloser{conn},
		done:  make(chan struct{}),
	}

	if err := c.send(message{Action: "settings", Data: settings}); err != nil {
		conn.Close()       //nolint:errcheck
		cmd.Process.Kill() //nolint:errcheck
		return nil, fmt.Errorf("stdui: send settings: %w", err)
	}

	go c.readLoop(conn)

	return c, nil
}

// connWriteCloser wraps a net.Conn and exposes only Write and Close so it
// satisfies the io.WriteCloser interface expected by Client.stdin.
type connWriteCloser struct {
	conn net.Conn
}

func (w *connWriteCloser) Write(p []byte) (int, error) {
	return w.conn.Write(p)
}

func (w *connWriteCloser) Close() error {
	return w.conn.Close()
}

// ---------------------------------------------------------------------------
// pipeConn — net.Conn wrapper for a plain io.ReadWriteCloser (named pipes)
// ---------------------------------------------------------------------------

// pipeConn wraps a bufio.ReadWriter backed by a raw file handle. It implements
// net.Conn via stub methods for the parts the SDK does not use.
type pipeConn struct {
	rw *bufio.ReadWriter
	f  interface{ Close() error }
}

func (c *pipeConn) Read(p []byte) (int, error) { return c.rw.Read(p) }
func (c *pipeConn) Write(p []byte) (int, error) {
	n, err := c.rw.Write(p)
	if err != nil {
		return n, err
	}
	return n, c.rw.Flush()
}
func (c *pipeConn) Close() error                       { return c.f.Close() }
func (c *pipeConn) LocalAddr() net.Addr                { return pipeAddr(0) }
func (c *pipeConn) RemoteAddr() net.Addr               { return pipeAddr(0) }
func (c *pipeConn) SetDeadline(_ time.Time) error      { return nil }
func (c *pipeConn) SetReadDeadline(_ time.Time) error  { return nil }
func (c *pipeConn) SetWriteDeadline(_ time.Time) error { return nil }

// pipeAddr is a minimal net.Addr for named pipes.
type pipeAddr int

func (pipeAddr) Network() string { return "pipe" }
func (pipeAddr) String() string  { return "pipe" }
