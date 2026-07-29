package main

import (
	"flag"
	"fmt"
	"os"
	"runtime"

	stdui "github.com/BigJk/stdui/sdk/go"
)

const contentTpl = `
<style>
* { box-sizing: border-box; margin: 0; padding: 0; }
body {
  font-family: sans-serif;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 40px 20px;
  height: 100%%;
}
.title {
  font-size: 1.2rem;
  font-weight: bold;
  margin-bottom: 8px;
}
.subtitle {
  color: #666;
  font-size: 0.9rem;
  margin-bottom: 16px;
}
.button {
  width: 150px;
}
</style>

<div class="title">IPC transport example</div>
<div class="subtitle">%s</div>
<div class="button">
  <ui-button text="Click me" action="ping"></ui-button>
</div>
`

func main() {
	socket := flag.String("socket", defaultSocketPath(), "connect via Unix domain socket at this path")
	pipe := flag.String("pipe", defaultPipePath(), "connect via named pipe (Unix domain socket on non-Windows, Windows named pipe on Windows)")
	binary := flag.String("binary", "./build/stdui", "path to the stdui binary")
	flag.Parse()

	settings := stdui.Settings{
		Title:        "IPC Example",
		WindowWidth:  stdui.Ptr(500),
		WindowHeight: stdui.Ptr(300),
		Resizable:    stdui.Ptr(false),
	}

	var (
		client      *stdui.Client
		err         error
		transportID string
	)

	switch {
	case isFlagSet("socket"):
		transportID = "Unix domain socket: " + *socket
		client, err = stdui.StartWithSocket(*binary, *socket, settings)
	default:
		transportID = "Named pipe: " + *pipe
		client, err = stdui.StartWithNamedPipe(*binary, *pipe, settings)
	}

	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to start stdui: %v\n", err)
		os.Exit(1)
	}

	client.OnReady(func() {
		_ = client.UpdateContent(fmt.Sprintf(contentTpl, transportID))
	})

	client.OnButtonClicked(func(attrs map[string]string, _ string) {
		if attrs["action"] == "ping" {
			fmt.Println("pong")
		}
	})

	client.OnLog(func(namespace, message string) {
		fmt.Fprintf(os.Stderr, "[log] %s: %s\n", namespace, message)
	})

	client.OnError(func(err error) {
		fmt.Fprintf(os.Stderr, "[error] %v\n", err)
	})

	client.Wait()
}

// defaultSocketPath returns the default Unix domain socket path.
func defaultSocketPath() string {
	return "/tmp/stdui-ipc-example.sock"
}

// defaultPipePath returns a sensible default named-pipe path for the current
// platform.
func defaultPipePath() string {
	if runtime.GOOS == "windows" {
		return `\\.\pipe\stdui-ipc-example`
	}
	return "/tmp/stdui-ipc-example.pipe"
}

// isFlagSet reports whether the flag with the given name was explicitly
// provided on the command line.
func isFlagSet(name string) bool {
	found := false
	flag.Visit(func(f *flag.Flag) {
		if f.Name == name {
			found = true
		}
	})
	return found
}
