# IPC Transports

Demonstrates connecting to stdui via a **Unix domain socket** or a **named pipe** instead of the default stdin/stdout transport.

## Usage

```sh
# Named pipe (default — Unix domain socket on non-Windows, Windows named pipe on Windows)
go run . -binary ../../build/stdui

# Unix domain socket
go run . -binary ../../build/stdui -socket /tmp/stdui-ipc-example.sock
```

## Flags

| Flag      | Default                                                                       | Description                                 |
| --------- | ----------------------------------------------------------------------------- | ------------------------------------------- |
| `-binary` | `./build/stdui`                                                               | Path to the stdui binary                    |
| `-socket` | `/tmp/stdui-ipc-example.sock`                                                 | Connect via Unix domain socket at this path |
| `-pipe`   | `/tmp/stdui-ipc-example.pipe` (Unix) / `\\.\pipe\stdui-ipc-example` (Windows) | Connect via named pipe                      |

When `-socket` is passed explicitly it takes priority. Otherwise `-pipe` is used.

## Notes

- stdui **creates** the socket/pipe itself — you do not need to create it beforehand.
- On Unix/macOS `--pipe` is backed by a Unix domain socket (FIFOs cannot carry bidirectional IPC reliably).
- On Windows `--pipe` uses a real Windows named pipe (`CreateNamedPipe`).

See the [IPC Transports docs](https://bigjk.github.io/StdUI/docs/ipc-transports) for a full explanation.
