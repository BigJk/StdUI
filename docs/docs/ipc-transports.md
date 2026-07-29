---
sidebar_position: 3
sidebar_label: IPC Transports
---

# IPC Transports

By default stdui communicates with the controlling application via **stdin/stdout**. Two alternative transports are available for cases where subprocess pipes are inconvenient — for example, when the controlling process is not the direct parent of stdui, or when you want to connect to an already-running stdui instance.

| Transport | CLI flag | Platform |
| --------- | -------- | -------- |
| stdin/stdout | _(default, no flag needed)_ | All |
| Unix domain socket | `--socket <path>` | All (Windows 10 1803+) |
| Named pipe | `--pipe <path>` | All |

:::note Platform note — named pipes on Unix
On Unix/macOS, `--pipe` is an alias for a Unix domain socket. FIFOs cannot carry reliable bidirectional IPC, so stdui creates a Unix domain socket at the given path regardless. On Windows, `--pipe` uses a real Windows named pipe (`CreateNamedPipe`).
:::

## How it works

When either flag is passed, stdui **creates** the socket or pipe itself before entering its main loop. The controlling application connects after the file appears on disk. The socket/pipe file is removed by stdui on shutdown.

```
App (Go, Python, anything)
       │  JSON commands  →
       │  ← JSON events
       ▼  (over socket or pipe instead of stdin/stdout)
  stdui binary (C++)
```

The message format and protocol are identical to stdin/stdout — every message is a single line of JSON terminated by a newline.

## CLI flags

### `--socket <path>`

Starts stdui in Unix domain socket mode. stdui creates and listens on a Unix domain socket at `<path>`. The controlling application dials that path after the file is visible.

```sh
./stdui --socket /tmp/myapp.sock
```

### `--pipe <path>`

Starts stdui in named-pipe mode.

- **Unix/macOS**: creates a Unix domain socket at `<path>` (same as `--socket`).
- **Windows**: creates a Windows named pipe at `<path>` (must be of the form `\\.\pipe\<name>`).

```sh
# Unix / macOS
./stdui --pipe /tmp/myapp.pipe

# Windows
stdui.exe --pipe \\.\pipe\myapp
```

## Go SDK

The Go SDK provides two convenience functions that spawn stdui with the appropriate flag and connect to it automatically. Both functions retry the connection with exponential back-off for up to 5 seconds while stdui is starting up.

### `StartWithSocket`

```go
import stdui "github.com/BigJk/stdui/sdk/go"

client, err := stdui.StartWithSocket(
    "./build/stdui",        // path to stdui binary
    "/tmp/myapp.sock",      // socket path — created by stdui
    stdui.Settings{
        Title:        "My App",
        WindowWidth:  stdui.Ptr(800),
        WindowHeight: stdui.Ptr(600),
    },
)
if err != nil {
    log.Fatal(err)
}
```

### `StartWithNamedPipe`

```go
client, err := stdui.StartWithNamedPipe(
    "./build/stdui",        // path to stdui binary
    "/tmp/myapp.pipe",      // pipe path — created by stdui
    stdui.Settings{
        Title:        "My App",
        WindowWidth:  stdui.Ptr(800),
        WindowHeight: stdui.Ptr(600),
    },
)
if err != nil {
    log.Fatal(err)
}
```

On Windows pass a named-pipe path:

```go
client, err := stdui.StartWithNamedPipe(
    `C:\path\to\stdui.exe`,
    `\\.\pipe\myapp`,
    settings,
)
```

After obtaining the `*Client`, usage is identical to `Start()` — register handlers and call `client.Wait()`.

## Full example

A runnable example is in [`example/ipc/main.go`](https://github.com/BigJk/stdui/blob/main/example/ipc/main.go). It accepts `-socket` and `-pipe` flags to select the transport at runtime:

```sh
# Use Unix domain socket
go run ./example/ipc -socket /tmp/stdui-demo.sock

# Use named pipe (default when no -socket flag is given)
go run ./example/ipc
```
