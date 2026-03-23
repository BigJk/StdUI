// Package stdui provides a Go SDK for controlling a stdui subprocess.
//
// stdui is a lightweight GUI framework that renders HTML content via an
// external binary. The controlling application communicates with the binary
// over stdin/stdout using newline-delimited JSON messages.
//
// Basic usage:
//
//	client, err := stdui.Start("./build/game", stdui.Settings{
//	    Title:        "My App",
//	    WindowWidth:  ptr(800),
//	    WindowHeight: ptr(600),
//	})
//	if err != nil {
//	    log.Fatal(err)
//	}
//
//	client.OnReady(func() {
//	    client.UpdateContent("<h1>Hello</h1>")
//	})
//	client.OnInputChanged(func(id, value string) {
//	    fmt.Println(id, "=", value)
//	})
//
//	client.Wait()
package stdui

import (
	"bufio"
	"context"
	"encoding/json"
	"fmt"
	"io"
	"os/exec"
	"sync"
)

// message is the wire format for all stdin/stdout communication.
type message struct {
	Action string `json:"action"`
	Data   any    `json:"data,omitempty"`
}

// Client manages a stdui subprocess and exposes a typed API for sending
// commands and receiving events.
type Client struct {
	cmd   *exec.Cmd
	stdin io.WriteCloser

	mu       sync.Mutex
	handlers handlers

	sendMu sync.Mutex // guards writes to stdin

	done chan struct{}
	once sync.Once // guards close(done)
}

// Start spawns the stdui binary at the given path, sends the initial settings,
// and begins reading events from the subprocess stdout. Handlers may be
// registered before or after Start — any handler registered before the
// corresponding event arrives will be called.
//
// The binary path may be absolute or relative to the working directory of the
// calling process.
func Start(binaryPath string, settings Settings) (*Client, error) {
	cmd := exec.Command(binaryPath)

	stdinPipe, err := cmd.StdinPipe()
	if err != nil {
		return nil, fmt.Errorf("stdui: create stdin pipe: %w", err)
	}
	stdoutPipe, err := cmd.StdoutPipe()
	if err != nil {
		return nil, fmt.Errorf("stdui: create stdout pipe: %w", err)
	}

	if err := cmd.Start(); err != nil {
		return nil, fmt.Errorf("stdui: start process: %w", err)
	}

	c := &Client{
		cmd:   cmd,
		stdin: stdinPipe,
		done:  make(chan struct{}),
	}

	// stdui blocks on startup until it receives the settings message.
	if err := c.send(message{Action: "settings", Data: settings}); err != nil {
		cmd.Process.Kill() //nolint:errcheck
		return nil, fmt.Errorf("stdui: send settings: %w", err)
	}

	go c.readLoop(stdoutPipe)

	return c, nil
}

// UpdateContent sets the HTML content rendered in the stdui window.
// It is safe to call from any goroutine.
func (c *Client) UpdateContent(html string) error {
	return c.send(message{Action: "update-content", Data: html})
}

// SetValue programmatically sets the value of an interactive element by id.
// It is safe to call from any goroutine.
func (c *Client) SetValue(id, value string) error {
	return c.send(message{
		Action: "set-value",
		Data:   map[string]string{"id": id, "value": value},
	})
}

// SetTitle updates the window title bar text at runtime.
// It is safe to call from any goroutine.
func (c *Client) SetTitle(title string) error {
	return c.send(message{Action: "set-title", Data: title})
}

// SetWindowIcon sets the window icon from a single image file.
// The file must be a format supported by raylib (PNG, BMP, TGA, JPG, …).
// It is safe to call from any goroutine.
func (c *Client) SetWindowIcon(path string) error {
	return c.send(message{Action: "set-window-icon", Data: path})
}

// SetWindowIcons sets the window icon from multiple image files, allowing the
// OS to pick the most appropriate resolution. The files must be in a format
// supported by raylib (PNG, BMP, TGA, JPG, …).
// It is safe to call from any goroutine.
func (c *Client) SetWindowIcons(paths []string) error {
	return c.send(message{Action: "set-window-icons", Data: paths})
}

// GetValue requests the current value of an interactive element by id.
// stdui responds asynchronously with a value-result event, which is
// delivered to the handler registered via OnValueResult.
// It is safe to call from any goroutine.
func (c *Client) GetValue(id string) error {
	return c.send(message{
		Action: "get-value",
		Data:   map[string]string{"id": id},
	})
}

// Close instructs stdui to close the window and exit cleanly.
// stdui will emit a window-closed event in response.
// It is safe to call from any goroutine.
func (c *Client) Close() error {
	return c.send(message{Action: "close"})
}

// Minimize minimizes the stdui window.
// It is safe to call from any goroutine.
func (c *Client) Minimize() error {
	return c.send(message{Action: "minimize"})
}

// Maximize maximizes the stdui window.
// It is safe to call from any goroutine.
func (c *Client) Maximize() error {
	return c.send(message{Action: "maximize"})
}

// SetPosition moves the stdui window to the given screen coordinates.
// It is safe to call from any goroutine.
func (c *Client) SetPosition(x, y int) error {
	return c.send(message{
		Action: "set-position",
		Data:   map[string]int{"x": x, "y": y},
	})
}

// PlaySound plays the sound file at the given path through stdui's audio system.
// The path may be absolute or relative to the stdui working directory.
// It is safe to call from any goroutine.
func (c *Client) PlaySound(file string) error {
	return c.send(message{Action: "play-sound", Data: file})
}

// SetVolume updates the sound effects volume at runtime. The value is clamped
// to the range [0.0, 1.0] by stdui. It is safe to call from any goroutine.
func (c *Client) SetVolume(volume float64) error {
	return c.send(message{Action: "set-volume", Data: volume})
}

// SetFPS updates the target frame rate at runtime.
// Values of 0 or below are ignored by stdui.
// It is safe to call from any goroutine.
func (c *Client) SetFPS(fps int) error {
	return c.send(message{Action: "set-fps", Data: fps})
}

// ConfirmRequest holds the parameters for a confirm dialog.
// ID and Question are required; the remaining fields are optional and fall back
// to sensible defaults when left as empty strings.
type ConfirmRequest struct {
	// ID is an opaque identifier echoed back in the confirm-result response.
	ID string
	// Question is the message shown inside the dialog.
	Question string
	// Title is the dialog heading. Defaults to "Confirm" when empty.
	Title string
	// OKText is the label for the confirmation button. Defaults to "OK" when empty.
	OKText string
	// CancelText is the label for the cancellation button. Defaults to "Cancel" when empty.
	CancelText string
}

// PaneNodeType identifies whether a PaneNode is a leaf pane or an interior split.
type PaneNodeType string

const (
	// PaneNodeTypePane is a leaf pane that renders HTML content.
	PaneNodeTypePane PaneNodeType = "pane"
	// PaneNodeTypeSplit is an interior node that divides its space among children.
	PaneNodeTypeSplit PaneNodeType = "split"
)

// PaneSplitDirection controls how a split node arranges its children.
type PaneSplitDirection string

const (
	// PaneSplitHorizontal arranges children left-to-right.
	PaneSplitHorizontal PaneSplitDirection = "horizontal"
	// PaneSplitVertical arranges children top-to-bottom.
	PaneSplitVertical PaneSplitDirection = "vertical"
)

// PaneNode is one node in a pane layout tree. Set Type to PaneNodeTypePane for
// a leaf pane (requires ID) or PaneNodeTypeSplit for an interior split (requires
// Direction and Children). Size is a relative weight used by the parent split
// (defaults to 1 when zero).
type PaneNode struct {
	Type      PaneNodeType       `json:"type"`
	ID        string             `json:"id,omitempty"`
	Direction PaneSplitDirection `json:"direction,omitempty"`
	Size      float64            `json:"size,omitempty"`
	Children  []*PaneNode        `json:"children,omitempty"`
}

// Pane is a convenience constructor that returns a leaf PaneNode with the
// given id and optional relative size weight.
func Pane(id string, size ...float64) *PaneNode {
	n := &PaneNode{Type: PaneNodeTypePane, ID: id}
	if len(size) > 0 {
		n.Size = size[0]
	}
	return n
}

// HSplit is a convenience constructor that returns a horizontal split PaneNode
// with the given children and an optional relative size weight (defaults to 1).
func HSplit(children []*PaneNode, size ...float64) *PaneNode {
	n := &PaneNode{Type: PaneNodeTypeSplit, Direction: PaneSplitHorizontal, Children: children}
	if len(size) > 0 {
		n.Size = size[0]
	}
	return n
}

// VSplit is a convenience constructor that returns a vertical split PaneNode
// with the given children and an optional relative size weight (defaults to 1).
func VSplit(children []*PaneNode, size ...float64) *PaneNode {
	n := &PaneNode{Type: PaneNodeTypeSplit, Direction: PaneSplitVertical, Children: children}
	if len(size) > 0 {
		n.Size = size[0]
	}
	return n
}

// SetPaneLayout sends a set-pane-layout action to stdui, replacing the current
// layout with the given tree. Use the Pane, HSplit, and VSplit constructors to
// build the tree.
// It is safe to call from any goroutine.
func (c *Client) SetPaneLayout(root *PaneNode) error {
	return c.send(message{Action: "set-pane-layout", Data: root})
}

// UpdatePaneContent sets the HTML content of a specific named pane. The pane
// must exist in the current layout set via SetPaneLayout.
// It is safe to call from any goroutine.
func (c *Client) UpdatePaneContent(pane, html string) error {
	return c.send(message{
		Action: "update-content",
		Data:   map[string]string{"pane": pane, "content": html},
	})
}

// ScrollToTop scrolls the named pane to the top.
// It is safe to call from any goroutine.
func (c *Client) ScrollToTop(pane string) error {
	return c.send(message{
		Action: "scroll-to",
		Data:   map[string]any{"pane": pane, "position": 0},
	})
}

// ScrollToBottom scrolls the named pane to the bottom.
// It is safe to call from any goroutine.
func (c *Client) ScrollToBottom(pane string) error {
	return c.send(message{
		Action: "scroll-to",
		Data:   map[string]any{"pane": pane, "position": "bottom"},
	})
}

// ScrollTo scrolls the named pane to an absolute vertical pixel offset.
// It is safe to call from any goroutine.
func (c *Client) ScrollTo(pane string, y float64) error {
	return c.send(message{
		Action: "scroll-to",
		Data:   map[string]any{"pane": pane, "position": y},
	})
}

// ToastRequest holds the parameters for a toast notification.
// Content is required; all other fields are optional and fall back to
// sensible defaults when left as zero values.
type ToastRequest struct {
	// Content is the HTML to render inside the toast.
	Content string
	// Width is the toast window width in pixels. Defaults to 300 when zero.
	Width float64
	// Height is the toast window height in pixels. Defaults to 80 when zero.
	Height float64
	// TTL is the time-to-live in seconds. Defaults to 3 when zero.
	TTL float64
}

// Toast displays a short-lived notification window anchored to the top-center
// of the stdui display. Toasts stack downward and disappear after their TTL
// expires or when the user double-clicks inside them.
// It is safe to call from any goroutine.
func (c *Client) Toast(req ToastRequest) error {
	data := map[string]any{"content": req.Content}
	if req.Width > 0 {
		data["width"] = req.Width
	}
	if req.Height > 0 {
		data["height"] = req.Height
	}
	if req.TTL > 0 {
		data["ttl"] = req.TTL
	}
	return c.send(message{Action: "toast", Data: data})
}

// SetClipboardText writes the given text to the system clipboard.
// It is safe to call from any goroutine.
func (c *Client) SetClipboardText(text string) error {
	return c.send(message{Action: "set-clipboard-text", Data: text})
}

// GetClipboardText requests the current system clipboard text from stdui.
// stdui responds asynchronously with a clipboard-text-result event, which is
// delivered to the handler registered via OnClipboardTextResult.
// It is safe to call from any goroutine.
func (c *Client) GetClipboardText() error {
	return c.send(message{Action: "get-clipboard-text"})
}

// KeybindRequest describes a single keyboard shortcut to register.
// ID and Key are required. Modifier fields default to false when omitted.
type KeybindRequest struct {
	// ID is an application-defined identifier echoed back in key-pressed events.
	ID string `json:"id"`
	// Key is the primary key name, case-insensitive.
	// Examples: "s", "f5", "escape", "enter", "space", "tab",
	//           "delete", "home", "end", "pageup", "pagedown",
	//           "left", "right", "up", "down",
	//           "0"–"9", "a"–"z", "f1"–"f12".
	Key string `json:"key"`
	// Ctrl requires the Ctrl key to be held.
	Ctrl bool `json:"ctrl,omitempty"`
	// Shift requires the Shift key to be held.
	Shift bool `json:"shift,omitempty"`
	// Alt requires the Alt key to be held.
	Alt bool `json:"alt,omitempty"`
	// Meta requires the Super/Cmd key to be held.
	Meta bool `json:"meta,omitempty"`
}

// SetKeybinds registers a set of keyboard shortcuts with stdui. Calling this
// action replaces any previously registered keybinds. When a registered
// shortcut is activated, stdui emits a key-pressed event carrying the
// keybind's ID.
//
// It is safe to call from any goroutine.
func (c *Client) SetKeybinds(keybinds []KeybindRequest) error {
	return c.send(message{Action: "set-keybinds", Data: keybinds})
}

// Confirm opens a modal dialog in the stdui window with the given question and
// two buttons. The result is delivered asynchronously via the handler registered
// with OnConfirmResult.
// It is safe to call from any goroutine.
func (c *Client) Confirm(req ConfirmRequest) error {
	data := map[string]string{
		"id":       req.ID,
		"question": req.Question,
	}
	if req.Title != "" {
		data["title"] = req.Title
	}
	if req.OKText != "" {
		data["ok-text"] = req.OKText
	}
	if req.CancelText != "" {
		data["cancel-text"] = req.CancelText
	}
	return c.send(message{Action: "confirm", Data: data})
}

// Wait blocks until the stdui window is closed (either by the user or by
// calling Close). It is safe to call from any goroutine.
func (c *Client) Wait() {
	c.WaitContext(context.Background()) //nolint:errcheck
}

// WaitContext blocks until the stdui window is closed or ctx is cancelled,
// whichever comes first. If ctx is cancelled before the window closes,
// WaitContext closes stdin (causing stdui to exit) and returns ctx.Err().
// It is safe to call from any goroutine.
func (c *Client) WaitContext(ctx context.Context) error {
	select {
	case <-c.done:
		// Normal exit: window closed on its own.
	case <-ctx.Done():
		// Caller cancelled — tell stdui to exit by closing stdin.
		c.sendMu.Lock()
		c.stdin.Close() //nolint:errcheck
		c.sendMu.Unlock()
		c.cmd.Wait() //nolint:errcheck
		return ctx.Err()
	}
	// Closing stdin unblocks stdui's reader thread so the process can exit.
	c.sendMu.Lock()
	c.stdin.Close() //nolint:errcheck
	c.sendMu.Unlock()
	c.cmd.Wait() //nolint:errcheck — non-zero exit is normal on some platforms
	return nil
}

// send marshals msg as a single line of JSON and writes it to stdin.
// It is safe to call from multiple goroutines concurrently.
func (c *Client) send(msg message) error {
	data, err := json.Marshal(msg)
	if err != nil {
		return fmt.Errorf("stdui: marshal message: %w", err)
	}
	c.sendMu.Lock()
	_, err = fmt.Fprintf(c.stdin, "%s\n", data)
	c.sendMu.Unlock()
	return err
}

// readLoop reads newline-delimited JSON from the subprocess stdout and
// dispatches each message to the appropriate registered handler.
func (c *Client) readLoop(r io.Reader) {
	scanner := bufio.NewScanner(r)
	for scanner.Scan() {
		line := scanner.Text()

		var msg struct {
			Action string          `json:"action"`
			Data   json.RawMessage `json:"data,omitempty"`
		}
		if err := json.Unmarshal([]byte(line), &msg); err != nil {
			c.mu.Lock()
			h := c.handlers.onError
			c.mu.Unlock()
			if h != nil {
				h(fmt.Errorf("stdui: parse message %q: %w", line, err))
			}
			continue
		}

		c.dispatch(msg.Action, msg.Data)
	}

	// EOF or scanner error — treat as window closed if not already done.
	c.once.Do(func() { close(c.done) })
}

// dispatch routes a parsed message to the appropriate handler.
func (c *Client) dispatch(action string, raw json.RawMessage) {
	c.mu.Lock()
	h := c.handlers
	c.mu.Unlock()

	switch action {
	case "ready":
		if h.onReady != nil {
			h.onReady()
		}

	case "log":
		if h.onLog != nil {
			var d struct {
				Namespace string `json:"namespace"`
				Message   string `json:"message"`
			}
			if json.Unmarshal(raw, &d) == nil {
				h.onLog(d.Namespace, d.Message)
			}
		}

	case "input-changed":
		if h.onInputChanged != nil {
			var d struct {
				ID    string `json:"id"`
				Value string `json:"value"`
				Pane  string `json:"pane"`
			}
			if json.Unmarshal(raw, &d) == nil {
				h.onInputChanged(d.ID, d.Value, d.Pane)
			}
		}

	case "button-clicked":
		if h.onButtonClicked != nil {
			var attrs map[string]string
			if json.Unmarshal(raw, &attrs) == nil {
				pane := attrs["pane"]
				delete(attrs, "pane")
				h.onButtonClicked(attrs, pane)
			}
		}

	case "value-result":
		if h.onValueResult != nil {
			var d struct {
				ID    string `json:"id"`
				Value string `json:"value"`
			}
			if json.Unmarshal(raw, &d) == nil {
				h.onValueResult(d.ID, d.Value)
			}
		}

	case "confirm-result":
		if h.onConfirmResult != nil {
			var d struct {
				ID     string `json:"id"`
				Result bool   `json:"result"`
			}
			if json.Unmarshal(raw, &d) == nil {
				h.onConfirmResult(d.ID, d.Result)
			}
		}

	case "url-clicked":
		if h.onURLClicked != nil {
			var d struct {
				URL  string `json:"url"`
				Pane string `json:"pane"`
			}
			if json.Unmarshal(raw, &d) == nil {
				h.onURLClicked(d.URL, d.Pane)
			}
		}

	case "file-dropped":
		if h.onFileDropped != nil {
			var d struct {
				Path string `json:"path"`
			}
			if json.Unmarshal(raw, &d) == nil {
				h.onFileDropped(d.Path)
			}
		}

	case "clipboard-text-result":
		if h.onClipboardTextResult != nil {
			var d struct {
				Text string `json:"text"`
			}
			if json.Unmarshal(raw, &d) == nil {
				h.onClipboardTextResult(d.Text)
			}
		}

	case "key-pressed":
		if h.onKeyPressed != nil {
			var d struct {
				ID string `json:"id"`
			}
			if json.Unmarshal(raw, &d) == nil {
				h.onKeyPressed(d.ID)
			}
		}

	case "window-closed":
		if h.onWindowClosed != nil {
			h.onWindowClosed()
		}
		c.once.Do(func() { close(c.done) })

	default:
		if h.onUnknown != nil {
			h.onUnknown(action, raw)
		}
	}
}
