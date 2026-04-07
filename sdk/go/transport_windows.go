//go:build windows

package stdui

import (
	"bufio"
	"fmt"
	"net"
	"os"
	"time"
)

// openNamedPipe connects to the Windows named pipe at path.
//
// path must be a valid Windows named-pipe path such as \\.\pipe\myapp.
// We retry opening until the pipe server (stdui) has created it.
func openNamedPipe(path string, timeout time.Duration) (net.Conn, error) {
	deadline := time.Now().Add(timeout)
	delay := 10 * time.Millisecond

	var f *os.File
	var err error
	for {
		// os.OpenFile works for named pipes on Windows.
		f, err = os.OpenFile(path, os.O_RDWR, os.ModeNamedPipe)
		if err == nil {
			break
		}
		if time.Now().After(deadline) {
			return nil, fmt.Errorf("timed out waiting for named pipe %s: %w", path, err)
		}
		time.Sleep(delay)
		delay *= 2
		if delay > 250*time.Millisecond {
			delay = 250 * time.Millisecond
		}
	}

	conn := &pipeConn{
		rw: bufio.NewReadWriter(bufio.NewReader(f), bufio.NewWriter(f)),
		f:  f,
	}
	return conn, nil
}
