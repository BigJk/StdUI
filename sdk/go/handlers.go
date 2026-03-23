package stdui

import "encoding/json"

// handlers holds all registered event callbacks. All fields are optional;
// nil handlers are silently ignored.
type handlers struct {
	onReady               func()
	onLog                 func(namespace, message string)
	onInputChanged        func(id, value, pane string)
	onButtonClicked       func(attrs map[string]string, pane string)
	onValueResult         func(id, value string)
	onConfirmResult       func(id string, result bool)
	onFileDropped         func(path string)
	onURLClicked          func(url, pane string)
	onWindowClosed        func()
	onClipboardTextResult func(text string)
	onKeyPressed          func(id string)
	onUnknown             func(action string, raw json.RawMessage)
	onError               func(err error)
}

// OnReady registers a callback invoked once after stdui has fully initialized
// and is ready to receive content. This is the correct place to call
// UpdateContent for the first time.
func (c *Client) OnReady(fn func()) {
	c.mu.Lock()
	c.handlers.onReady = fn
	c.mu.Unlock()
}

// OnLog registers a callback invoked for every internal log message emitted by
// stdui. namespace is the subsystem that produced the message (e.g. "Main",
// "Raylib"). Defaults to no-op if not registered.
func (c *Client) OnLog(fn func(namespace, message string)) {
	c.mu.Lock()
	c.handlers.onLog = fn
	c.mu.Unlock()
}

// OnInputChanged registers a callback invoked when the user commits a change
// to any interactive element (ui-input, ui-select, ui-slider, ui-checkbox,
// ui-textarea). id is the element's id attribute; value is the new value as a
// string (checkboxes use "true" / "false"); pane is the id of the pane the
// element belongs to.
func (c *Client) OnInputChanged(fn func(id, value, pane string)) {
	c.mu.Lock()
	c.handlers.onInputChanged = fn
	c.mu.Unlock()
}

// OnButtonClicked registers a callback invoked when the user clicks a
// ui-button. attrs contains all HTML attributes of the button, including
// "action" and "text". pane is the id of the pane the button belongs to.
func (c *Client) OnButtonClicked(fn func(attrs map[string]string, pane string)) {
	c.mu.Lock()
	c.handlers.onButtonClicked = fn
	c.mu.Unlock()
}

// OnValueResult registers a callback invoked in response to a GetValue call.
// id is the element id that was requested; value is its current value.
func (c *Client) OnValueResult(fn func(id, value string)) {
	c.mu.Lock()
	c.handlers.onValueResult = fn
	c.mu.Unlock()
}

// OnConfirmResult registers a callback invoked when the user dismisses a
// confirm dialog. id is the identifier from the originating Confirm call;
// result is true if the user clicked OK and false if they clicked Cancel.
func (c *Client) OnConfirmResult(fn func(id string, result bool)) {
	c.mu.Lock()
	c.handlers.onConfirmResult = fn
	c.mu.Unlock()
}

// OnFileDropped registers a callback invoked once per file when the user drags
// and drops files onto the stdui window. path is the absolute path to the
// dropped file.
func (c *Client) OnFileDropped(fn func(path string)) {
	c.mu.Lock()
	c.handlers.onFileDropped = fn
	c.mu.Unlock()
}

// OnURLClicked registers a callback invoked when the user clicks a hyperlink
// inside any pane. url is the href value; pane is the id of the pane the link
// was clicked in.
func (c *Client) OnURLClicked(fn func(url, pane string)) {
	c.mu.Lock()
	c.handlers.onURLClicked = fn
	c.mu.Unlock()
}

// OnWindowClosed registers a callback invoked when the stdui window is
// closed, either by the user or by calling Close. The callback fires before
// Wait returns.
func (c *Client) OnWindowClosed(fn func()) {
	c.mu.Lock()
	c.handlers.onWindowClosed = fn
	c.mu.Unlock()
}

// OnClipboardTextResult registers a callback invoked in response to a
// GetClipboardText call. text is the current system clipboard contents.
func (c *Client) OnClipboardTextResult(fn func(text string)) {
	c.mu.Lock()
	c.handlers.onClipboardTextResult = fn
	c.mu.Unlock()
}

// OnKeyPressed registers a callback invoked when a registered keyboard
// shortcut is activated. id is the application-defined identifier supplied
// when the shortcut was registered via SetKeybinds.
func (c *Client) OnKeyPressed(fn func(id string)) {
	c.mu.Lock()
	c.handlers.onKeyPressed = fn
	c.mu.Unlock()
}

// OnUnknown registers a callback invoked for any action not recognised by the
// SDK. raw is the unparsed JSON data payload. Useful for forward-compatibility
// with newer stdui versions.
func (c *Client) OnUnknown(fn func(action string, raw json.RawMessage)) {
	c.mu.Lock()
	c.handlers.onUnknown = fn
	c.mu.Unlock()
}

// OnError registers a callback invoked when a message from stdui cannot be
// parsed. If not registered, parse errors are silently dropped.
func (c *Client) OnError(fn func(err error)) {
	c.mu.Lock()
	c.handlers.onError = fn
	c.mu.Unlock()
}
