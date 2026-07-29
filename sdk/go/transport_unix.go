//go:build !windows

package stdui

import (
	"net"
	"time"
)

// openNamedPipe connects to the Unix domain socket that stdui creates when
// given the --pipe flag on Unix.
//
// On non-Windows platforms stdui backs --pipe with a Unix domain socket
// (identical to --socket) because POSIX FIFOs cannot carry bidirectional IPC
// reliably. We therefore just dial the path as a Unix socket.
func openNamedPipe(path string, timeout time.Duration) (net.Conn, error) {
	return dialRetry("unix", path, timeout)
}
