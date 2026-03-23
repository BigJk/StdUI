package main

import (
	"fmt"
	"os"

	"github.com/BigJk/stdui"
)

const content = `
<style>
* { box-sizing: border-box; margin: 0; padding: 0; }
body {
  font-family: sans-serif;
  background: #f5f5f5;
  color: #1c1e21;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 40px 20px;
  height: 100%;
}

img {
  display: block;
  margin-bottom: 24px;
  width: 300px;
}

.title {
  font-size: 1.25rem;
  font-weight: bold;
  margin-bottom: 24px;
}

.button {
	width: 150px;
}
</style>

<img src="./assets/images/logo_300.png">
<div class="title">Hello World from StdUI</div>
<div class="button">
	<ui-button id="do-something" text="Do Something" action="do-something"></ui-button>
</div>
`

func main() {
	client, err := stdui.Start("./build/stdui", stdui.Settings{
		Title:        "Simple",
		WindowWidth:  stdui.Ptr(700),
		WindowHeight: stdui.Ptr(400),
		Resizable:    stdui.Ptr(false),
		ColorScheme: &stdui.ColorScheme{
			ElementBg:      "#223446",
			ElementHovered: "#2e4053",
			ElementActive:  "#223446",
			Text:           "#ffffff",
		},
	})
	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to start stdui: %v\n", err)
		os.Exit(1)
	}

	client.OnReady(func() {
		client.SetWindowIcon("./assets/images/icon_250.png")
		client.UpdateContent(content) //nolint:errcheck
	})

	client.OnButtonClicked(func(attrs map[string]string, _ string) {
		if attrs["action"] == "do-something" {
			fmt.Println("button pressed")
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
