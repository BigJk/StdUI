package main

import (
	"fmt"
	"html"
	"os"
	"strings"
	"sync"
	"time"

	stdui "github.com/BigJk/stdui"
	irc "github.com/fluffle/goirc/client"
)

// ── Data types ────────────────────────────────────────────────────────────────

// Message is a single line in a channel's history.
type Message struct {
	From string
	Text string
	Time time.Time
	Kind msgKind
}

type msgKind int

const (
	kindPrivmsg msgKind = iota // regular PRIVMSG
	kindNotice                 // NOTICE
	kindJoin                   // JOIN / PART / QUIT system lines
	kindError                  // connection-level errors shown inline
)

// Channel holds the state for one joined IRC channel.
type Channel struct {
	Name    string
	History []Message
	Unread  int // messages received while not the active channel
}

// AppState is all mutable UI state.
type AppState struct {
	// Connect-screen fields (before connection is established).
	Server       string
	Port         string
	Nick         string
	Channel      string
	ConnectError string // non-empty while/after a failed connection attempt

	// Post-connection state.
	Connected bool
	Channels  []*Channel
	Active    int       // index into Channels
	Nick_     string    // effective nick (may differ from requested after collision)
	Conn      *irc.Conn // live IRC connection, nil when disconnected
}

func (s *AppState) activeChannel() *Channel {
	if len(s.Channels) == 0 {
		return nil
	}
	return s.Channels[s.Active]
}

func (s *AppState) channelByName(name string) *Channel {
	for _, ch := range s.Channels {
		if strings.EqualFold(ch.Name, name) {
			return ch
		}
	}
	return nil
}

func (s *AppState) addChannel(name string) *Channel {
	if ch := s.channelByName(name); ch != nil {
		return ch
	}
	ch := &Channel{Name: name}
	s.Channels = append(s.Channels, ch)
	return ch
}

func (s *AppState) removeChannel(name string) {
	for i, ch := range s.Channels {
		if strings.EqualFold(ch.Name, name) {
			s.Channels = append(s.Channels[:i], s.Channels[i+1:]...)
			if s.Active >= len(s.Channels) && s.Active > 0 {
				s.Active--
			}
			return
		}
	}
}

func (s *AppState) appendMsg(target string, msg Message) {
	ch := s.channelByName(target)
	if ch == nil {
		return
	}
	ch.History = append(ch.History, msg)
	if !strings.EqualFold(s.Channels[s.Active].Name, target) {
		ch.Unread++
	}
}

// ── Styles ────────────────────────────────────────────────────────────────────

const baseStyle = `<style>
* { box-sizing: border-box; margin: 0; padding: 0; }
body { font-family: monospace; background: #1e1e2e; color: #cdd6f4; }

/* ── Connect screen ───────────────────────────────────────────────── */
.connect-wrap {
  padding: 32px 24px;
}
.connect-title {
  font-size: 1.3rem;
  font-weight: bold;
  color: #cba6f7;
  margin-bottom: 20px;
}
.connect-error {
  background: #f38ba8;
  color: #1e1e2e;
  border-radius: 6px;
  padding: 10px 14px;
  margin-bottom: 16px;
  font-size: 1rem;
}
.field-label {
  font-size: 1rem;
  color: #a6adc8;
  margin-bottom: 4px;
}
.field-row {
  margin-bottom: 12px;
}

/* ── Connecting screen ────────────────────────────────────────────── */
.connecting-wrap {
  display: flex;
  justify-content: center;
  align-items: center;
  height: 100%;
  padding: 40px;
}
.connecting-text {
  font-size: 1.1rem;
  color: #a6adc8;
}

/* ── Sidebar ──────────────────────────────────────────────────────── */
.sidebar-header {
  font-size: 1rem;
  font-weight: bold;
  color: #cba6f7;
  padding: 10px 12px 8px 12px;
  border-bottom: 1px solid #313244;
  background: #181825;
}
a.chan {
  display: block;
  padding: 8px 12px;
  border-bottom: 1px solid #1e1e2e;
  text-decoration: none;
  color: #cdd6f4;
  background: #181825;
}
a.chan-active {
  background: #313244;
  color: #cba6f7;
}
.chan-name { font-size: 1rem; font-weight: bold; }
.chan-badge {
  font-size: 1rem;
  color: #f38ba8;
  font-weight: bold;
}

/* ── Message history ──────────────────────────────────────────────── */
.history {
  padding: 10px 12px;
  background: #1e1e2e;
}
.msg-row { margin-bottom: 4px; }
.msg-time  { color: #585b70; font-size: 1rem; }
.msg-nick  { color: #89b4fa; font-size: 1rem; font-weight: bold; }
.msg-nick-self { color: #a6e3a1; font-size: 1rem; font-weight: bold; }
.msg-text  { color: #cdd6f4; font-size: 1rem; }
.msg-notice { color: #fab387; font-size: 1rem; font-style: italic; }
.msg-system { color: #585b70; font-size: 1rem; font-style: italic; }
.msg-error  { color: #f38ba8; font-size: 1rem; font-style: italic; }
.empty-history {
  text-align: center;
  padding: 40px 0;
  color: #585b70;
  font-size: 1rem;
}

/* ── Compose ──────────────────────────────────────────────────────── */
.compose {
  padding: 8px 12px;
  background: #181825;
  border-top: 1px solid #313244;
}
</style>
`

// ── Render functions ──────────────────────────────────────────────────────────

// renderConnect renders the pre-connection setup screen into the single "main" pane.
// If state.ConnectError is non-empty, an error banner is shown above the form.
func renderConnect(state AppState) string {
	var b strings.Builder
	b.WriteString(baseStyle)
	b.WriteString(`<div class="connect-wrap">`)
	b.WriteString(`<div class="connect-title">Connect to IRC</div>`)

	if state.ConnectError != "" {
		fmt.Fprintf(&b,
			`<div class="connect-error">%s</div>`,
			html.EscapeString(state.ConnectError),
		)
	}

	field := func(label, id, typ, placeholder, value string) {
		fmt.Fprintf(&b,
			`<div class="field-row">`+
				`<div class="field-label">%s</div>`+
				`<ui-input id="%s" type="%s" placeholder="%s" value="%s"></ui-input>`+
				`</div>`,
			label, id, typ,
			html.EscapeString(placeholder),
			html.EscapeString(value),
		)
	}

	field("Server", "cfg-server", "text", "irc.libera.chat", state.Server)
	field("Port", "cfg-port", "text", "6667", state.Port)
	field("Nickname", "cfg-nick", "text", "stdui-user", state.Nick)
	field("Channel", "cfg-channel", "text", "#general", state.Channel)

	b.WriteString(`<ui-button text="Connect" action="connect"></ui-button>`)
	b.WriteString(`</div>`)
	return b.String()
}

// renderConnecting renders a centered "connecting…" screen in the single "main" pane.
func renderConnecting(addr string) string {
	return baseStyle + fmt.Sprintf(
		`<div class="connecting-wrap">`+
			`<div class="connecting-text">Connecting to %s…</div>`+
			`</div>`,
		html.EscapeString(addr),
	)
}

// renderSidebar renders the joined channel list.
func renderSidebar(state AppState) string {
	var b strings.Builder
	b.WriteString(baseStyle)
	b.WriteString(`<div class="sidebar-header">Channels</div>`)

	for i, ch := range state.Channels {
		cls := "chan"
		if i == state.Active {
			cls = "chan chan-active"
		}
		badge := ""
		if ch.Unread > 0 && i != state.Active {
			badge = fmt.Sprintf(` <span class="chan-badge">(%d)</span>`, ch.Unread)
		}
		fmt.Fprintf(&b,
			`<a href="select-chan:%d" class="%s">`+
				`<span class="chan-name">%s</span>%s`+
				`</a>`,
			i, cls,
			html.EscapeString(ch.Name),
			badge,
		)
	}
	return b.String()
}

// renderHistory renders the message history for the active channel.
func renderHistory(state AppState) string {
	var b strings.Builder
	b.WriteString(baseStyle)
	b.WriteString(`<div class="history">`)

	ch := state.activeChannel()
	if ch == nil || len(ch.History) == 0 {
		name := ""
		if ch != nil {
			name = ch.Name
		}
		fmt.Fprintf(&b,
			`<div class="empty-history">No messages in %s yet.</div>`,
			html.EscapeString(name))
		b.WriteString(`</div>`)
		return b.String()
	}

	for _, msg := range ch.History {
		ts := msg.Time.Format("15:04")
		switch msg.Kind {
		case kindPrivmsg:
			nickClass := "msg-nick"
			if strings.EqualFold(msg.From, state.Nick_) {
				nickClass = "msg-nick-self"
			}
			fmt.Fprintf(&b,
				`<div class="msg-row">`+
					`<span class="msg-time">[%s]</span> `+
					`<span class="%s">&lt;%s&gt;</span> `+
					`<span class="msg-text">%s</span>`+
					`</div>`,
				ts, nickClass,
				html.EscapeString(msg.From),
				html.EscapeString(msg.Text),
			)
		case kindNotice:
			fmt.Fprintf(&b,
				`<div class="msg-row">`+
					`<span class="msg-time">[%s]</span> `+
					`<span class="msg-notice">-%s- %s</span>`+
					`</div>`,
				ts,
				html.EscapeString(msg.From),
				html.EscapeString(msg.Text),
			)
		case kindJoin:
			fmt.Fprintf(&b,
				`<div class="msg-row">`+
					`<span class="msg-time">[%s]</span> `+
					`<span class="msg-system">%s</span>`+
					`</div>`,
				ts,
				html.EscapeString(msg.Text),
			)
		case kindError:
			fmt.Fprintf(&b,
				`<div class="msg-row">`+
					`<span class="msg-time">[%s]</span> `+
					`<span class="msg-error">%s</span>`+
					`</div>`,
				ts,
				html.EscapeString(msg.Text),
			)
		}
	}

	b.WriteString(`</div>`)
	return b.String()
}

// renderCompose renders the message input bar.
func renderCompose(state AppState) string {
	ch := state.activeChannel()
	placeholder := "Message…"
	if ch != nil {
		placeholder = fmt.Sprintf("Message %s…", ch.Name)
	}
	return baseStyle + fmt.Sprintf(
		`<div class="compose">`+
			`<ui-input id="draft" type="text" placeholder="%s" value=""></ui-input>`+
			`</div>`,
		html.EscapeString(placeholder),
	)
}

// ── IRC connection ────────────────────────────────────────────────────────────

// connectIRC dials the IRC server and wires up all handlers. All mutations to
// state are protected by mu; render is called (with mu already held) whenever
// the UI needs refreshing. onConnected is called (without mu held) once the
// server handshake completes successfully, allowing the caller to switch layouts.
// onDisconnected is called (without mu held) whenever the connection drops,
// including after an explicit /disconnect, allowing the caller to reset the UI.
func connectIRC(
	server, nick string,
	initialChannel string,
	mu *sync.Mutex,
	state *AppState,
	ui *stdui.Client,
	onConnected func(),
	onDisconnected func(),
	render func(),
) {
	cfg := irc.NewConfig(nick)
	cfg.Server = server
	cfg.NewNick = func(n string) string { return n + "_" }

	conn := irc.Client(cfg)

	// ── CONNECTED ────────────────────────────────────────────────────────
	conn.HandleFunc(irc.CONNECTED, func(c *irc.Conn, _ *irc.Line) {
		mu.Lock()
		state.Connected = true
		state.Nick_ = c.Me().Nick
		state.Conn = c
		mu.Unlock()

		onConnected()
		c.Join(initialChannel)
	})

	// ── DISCONNECTED ─────────────────────────────────────────────────────
	conn.HandleFunc(irc.DISCONNECTED, func(_ *irc.Conn, _ *irc.Line) {
		mu.Lock()
		state.Connected = false
		state.Conn = nil
		state.Channels = nil
		state.Active = 0
		mu.Unlock()
		onDisconnected()
	})

	// ── PRIVMSG ───────────────────────────────────────────────────────────
	conn.HandleFunc(irc.PRIVMSG, func(c *irc.Conn, line *irc.Line) {
		target := line.Target()
		// Ignore private messages not directed at a joined channel.
		if !strings.HasPrefix(target, "#") && !strings.HasPrefix(target, "&") {
			return
		}
		mu.Lock()
		defer mu.Unlock()
		state.appendMsg(target, Message{
			From: line.Nick,
			Text: line.Text(),
			Time: line.Time,
			Kind: kindPrivmsg,
		})
		render()
	})

	// ── NOTICE ────────────────────────────────────────────────────────────
	conn.HandleFunc(irc.NOTICE, func(_ *irc.Conn, line *irc.Line) {
		target := line.Target()
		if !strings.HasPrefix(target, "#") && !strings.HasPrefix(target, "&") {
			return
		}
		mu.Lock()
		defer mu.Unlock()
		state.appendMsg(target, Message{
			From: line.Nick,
			Text: line.Text(),
			Time: line.Time,
			Kind: kindNotice,
		})
		render()
	})

	// ── JOIN ──────────────────────────────────────────────────────────────
	conn.HandleFunc(irc.JOIN, func(c *irc.Conn, line *irc.Line) {
		channel := line.Target()
		mu.Lock()
		defer mu.Unlock()

		ch := state.addChannel(channel)
		// Only post a join message for other users, not ourselves.
		if !strings.EqualFold(line.Nick, c.Me().Nick) {
			ch.History = append(ch.History, Message{
				Text: fmt.Sprintf("→ %s joined %s", line.Nick, channel),
				Time: line.Time,
				Kind: kindJoin,
			})
		}
		// Switch to the newly joined channel automatically if it's ours.
		if strings.EqualFold(line.Nick, c.Me().Nick) {
			for i, c2 := range state.Channels {
				if strings.EqualFold(c2.Name, channel) {
					state.Active = i
					break
				}
			}
		}
		render()
	})

	// ── PART ──────────────────────────────────────────────────────────────
	conn.HandleFunc(irc.PART, func(c *irc.Conn, line *irc.Line) {
		channel := line.Target()
		mu.Lock()
		defer mu.Unlock()

		if strings.EqualFold(line.Nick, c.Me().Nick) {
			state.removeChannel(channel)
		} else {
			if ch := state.channelByName(channel); ch != nil {
				ch.History = append(ch.History, Message{
					Text: fmt.Sprintf("← %s left %s", line.Nick, channel),
					Time: line.Time,
					Kind: kindJoin,
				})
			}
		}
		render()
	})

	// ── QUIT ──────────────────────────────────────────────────────────────
	conn.HandleFunc(irc.QUIT, func(_ *irc.Conn, line *irc.Line) {
		mu.Lock()
		defer mu.Unlock()
		// Append a quit notice to every channel this user was in (goirc
		// already tracks membership, but we just add it to all for simplicity).
		for _, ch := range state.Channels {
			ch.History = append(ch.History, Message{
				Text: fmt.Sprintf("← %s quit (%s)", line.Nick, line.Text()),
				Time: line.Time,
				Kind: kindJoin,
			})
		}
		render()
	})

	// ── KICK ──────────────────────────────────────────────────────────────
	conn.HandleFunc(irc.KICK, func(c *irc.Conn, line *irc.Line) {
		channel := line.Target()
		mu.Lock()
		defer mu.Unlock()
		if strings.EqualFold(line.Args[1], c.Me().Nick) {
			state.removeChannel(channel)
		} else {
			if ch := state.channelByName(channel); ch != nil {
				ch.History = append(ch.History, Message{
					Text: fmt.Sprintf("← %s was kicked from %s by %s (%s)",
						line.Args[1], channel, line.Nick, line.Text()),
					Time: line.Time,
					Kind: kindJoin,
				})
			}
		}
		render()
	})

	// ── NICK ──────────────────────────────────────────────────────────────
	conn.HandleFunc(irc.NICK, func(c *irc.Conn, line *irc.Line) {
		mu.Lock()
		defer mu.Unlock()
		newNick := line.Text()
		if strings.EqualFold(line.Nick, state.Nick_) {
			state.Nick_ = newNick
		}
		for _, ch := range state.Channels {
			ch.History = append(ch.History, Message{
				Text: fmt.Sprintf("· %s is now known as %s", line.Nick, newNick),
				Time: line.Time,
				Kind: kindJoin,
			})
		}
		render()
	})

	if err := conn.Connect(); err != nil {
		mu.Lock()
		state.Connected = false
		state.Conn = nil
		state.ConnectError = fmt.Sprintf("Connection failed: %v", err)
		mu.Unlock()
		ui.UpdateContent(renderConnect(*state)) //nolint:errcheck
	}
}

// ── Main ──────────────────────────────────────────────────────────────────────

func main() {
	var (
		mu    sync.Mutex
		state = AppState{
			Server:  "irc.libera.chat",
			Port:    "6667",
			Nick:    "stdui-user",
			Channel: "#test",
		}
	)

	ui, err := stdui.Start("./build/stdui", stdui.Settings{
		Title:        "IRC",
		WindowWidth:  stdui.Ptr(1100),
		WindowHeight: stdui.Ptr(680),
		Resizable:    stdui.Ptr(true),
		ColorScheme: &stdui.ColorScheme{
			WindowBg:       "#1e1e2e",
			Text:           "#cdd6f4",
			TextMuted:      "#a6adc8",
			ElementBg:      "#313244",
			ElementHovered: "#45475a",
			ElementActive:  "#585b70",
			TitleBg:        "#181825",
			Border:         "#313244",
			Primary:        "#cba6f7",
		},
		FontRegular:    "./assets/fonts/Iosevka-Regular.ttf",
		FontBold:       "./assets/fonts/Iosevka-Bold.ttf",
		FontItalic:     "./assets/fonts/Iosevka-Italic.ttf",
		FontBoldItalic: "./assets/fonts/Iosevka-BoldItalic.ttf",
	})
	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to start stdui: %v\n", err)
		os.Exit(1)
	}

	// renderConnect is single-pane; renderChat uses a three-pane layout.
	// We switch layouts when the connection is established.
	setupConnectLayout := func() {
		ui.SetPaneLayout(stdui.VSplit([]*stdui.PaneNode{ //nolint:errcheck
			stdui.Pane("main", 1),
		}))
		ui.UpdateContent(renderConnect(state)) //nolint:errcheck
	}

	setupChatLayout := func() {
		ui.SetPaneLayout(stdui.HSplit([]*stdui.PaneNode{ //nolint:errcheck
			stdui.Pane("sidebar", 180),
			stdui.VSplit([]*stdui.PaneNode{
				stdui.Pane("history", 1),
				stdui.Pane("compose", 46),
			}, 1),
		}))
	}

	renderChat := func() {
		ui.UpdatePaneContent("sidebar", renderSidebar(state)) //nolint:errcheck
		ui.UpdatePaneContent("history", renderHistory(state)) //nolint:errcheck
		ui.UpdatePaneContent("compose", renderCompose(state)) //nolint:errcheck
		ui.ScrollToBottom("history")                          //nolint:errcheck
	}

	ui.OnLog(func(ns, msg string) {
		fmt.Fprintf(os.Stderr, "[log] %s: %s\n", ns, msg)
	})

	ui.OnError(func(err error) {
		fmt.Fprintf(os.Stderr, "[error] %v\n", err)
	})

	ui.OnReady(func() {
		mu.Lock()
		defer mu.Unlock()
		setupConnectLayout()
	})

	// ── Button clicks ─────────────────────────────────────────────────────
	ui.OnButtonClicked(func(attrs map[string]string, _ string) {
		if attrs["action"] != "connect" {
			return
		}

		mu.Lock()
		server := strings.TrimSpace(state.Server)
		port := strings.TrimSpace(state.Port)
		nick := strings.TrimSpace(state.Nick)
		channel := strings.TrimSpace(state.Channel)
		if port == "" {
			port = "6667"
		}
		if !strings.HasPrefix(channel, "#") && !strings.HasPrefix(channel, "&") {
			channel = "#" + channel
		}
		state.Channel = channel
		state.Nick_ = nick
		state.ConnectError = ""
		serverAddr := fmt.Sprintf("%s:%s", server, port)
		mu.Unlock()

		// Show the connecting screen in the single pane while dialling.
		ui.UpdateContent(renderConnecting(serverAddr)) //nolint:errcheck

		go connectIRC(serverAddr, nick, channel, &mu, &state, ui,
			// onConnected: switch to the three-pane chat layout.
			func() {
				setupChatLayout()
				renderChat()
			},
			// onDisconnected: reset to the connect screen.
			func() {
				setupConnectLayout()
			},
			// render: refresh the chat panes (called by IRC event handlers).
			func() {
				renderChat()
			},
		)
	})

	// ── Input committed ───────────────────────────────────────────────────
	ui.OnInputChanged(func(id, value, _ string) {
		mu.Lock()

		// Connect-screen field sync.
		switch id {
		case "cfg-server":
			state.Server = value
			mu.Unlock()
			return
		case "cfg-port":
			state.Port = value
			mu.Unlock()
			return
		case "cfg-nick":
			state.Nick = value
			mu.Unlock()
			return
		case "cfg-channel":
			state.Channel = value
			mu.Unlock()
			return
		}

		// Chat compose box.
		if id != "draft" {
			mu.Unlock()
			return
		}
		text := strings.TrimSpace(value)
		if text == "" {
			mu.Unlock()
			return
		}

		ch := state.activeChannel()
		if ch == nil {
			mu.Unlock()
			return
		}

		conn := state.Conn
		if conn == nil {
			ch.History = append(ch.History, Message{
				Text: "Not connected.",
				Time: time.Now(),
				Kind: kindError,
			})
			mu.Unlock()
			renderChat()
			ui.SetValue("draft", "") //nolint:errcheck
			return
		}

		// Handle /commands.
		if strings.HasPrefix(text, "/") {
			parts := strings.SplitN(text[1:], " ", 2)
			cmd := strings.ToUpper(parts[0])
			arg := ""
			if len(parts) > 1 {
				arg = parts[1]
			}

			switch cmd {
			case "DISCONNECT", "QUIT":
				mu.Unlock()
				conn.Quit("Goodbye")
				// onDisconnected will fire and navigate back to the connect screen.
			case "JOIN":
				mu.Unlock()
				if arg != "" {
					arg = strings.TrimSpace(arg)
					if !strings.HasPrefix(arg, "#") && !strings.HasPrefix(arg, "&") {
						arg = "#" + arg
					}
					conn.Join(arg)
				}
			case "PART", "LEAVE":
				partChan := ch.Name
				mu.Unlock()
				conn.Part(partChan)
			case "NICK":
				mu.Unlock()
				if arg != "" {
					conn.Nick(arg)
				}
			case "ME":
				if arg != "" {
					conn.Action(ch.Name, arg)
					ch.History = append(ch.History, Message{
						From: state.Nick_,
						Text: fmt.Sprintf("* %s %s", state.Nick_, arg),
						Time: time.Now(),
						Kind: kindPrivmsg,
					})
				}
				mu.Unlock()
			default:
				ch.History = append(ch.History, Message{
					Text: fmt.Sprintf("Unknown command: /%s", strings.ToLower(cmd)),
					Time: time.Now(),
					Kind: kindError,
				})
				mu.Unlock()
			}

			renderChat()
			ui.SetValue("draft", "") //nolint:errcheck
			return
		}

		// Regular message — send to server and optimistically append to history.
		// Most servers do NOT echo PRIVMSGs back to the sender, so we add it here.
		target := ch.Name
		nick := state.Nick_
		mu.Unlock()
		conn.Privmsg(target, text)
		mu.Lock()
		ch.History = append(ch.History, Message{
			From: nick,
			Text: text,
			Time: time.Now(),
			Kind: kindPrivmsg,
		})
		mu.Unlock()
		renderChat()
		ui.SetValue("draft", "") //nolint:errcheck
	})

	// ── URL clicks (channel selection in sidebar) ─────────────────────────
	ui.OnURLClicked(func(url, _ string) {
		var idx int
		if _, err := fmt.Sscanf(url, "select-chan:%d", &idx); err != nil {
			return
		}
		mu.Lock()
		defer mu.Unlock()
		if idx < 0 || idx >= len(state.Channels) {
			return
		}
		state.Active = idx
		state.Channels[idx].Unread = 0
		renderChat()
	})

	ui.Wait()
}
