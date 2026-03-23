package main

import (
	"fmt"
	"html"
	"os"
	"strings"
	"sync"
	"time"

	"github.com/BigJk/stdui/sdk/go"
)

// Message is a single chat message.
type Message struct {
	From string
	Text string
	Time time.Time
	Self bool // true when sent by "You"
}

// Contact is a person in the sidebar.
type Contact struct {
	Name string
}

// AppState is all mutable state for the chat app.
type AppState struct {
	Contacts      []Contact
	ActiveContact int // index into Contacts
	History       map[string][]Message
}

const baseStyle = `
<style>
* { box-sizing: border-box; margin: 0; padding: 0; }
body { font-family: sans-serif; background: #f0f2f5; color: #1c1e21; }

/* ── Sidebar ──────────────────────────────────────────────────────── */
.sidebar-title {
  font-size: 1.1rem;
  font-weight: bold;
  color: #1c1e21;
  padding: 14px 12px 10px 12px;
  border-bottom: 1px solid #dde1e7;
  background: #ffffff;
}

a.contact {
  display: block;
  padding: 12px 14px;
  border-bottom: 1px solid #eaedf0;
  text-decoration: none;
  color: #1c1e21;
  background: #ffffff;
}

a.contact-active {
  background: #e3f2fd;
}

.contact-name {
  font-size: 1rem;
  font-weight: bold;
  color: #1c1e21;
  margin-bottom: 2px;
}

.contact-preview {
  font-size: 1rem;
  color: #65676b;
}

/* ── Message history ──────────────────────────────────────────────── */
.history { padding: 14px; background: #f0f2f5; }

.day-divider {
  text-align: center;
  font-size: 1rem;
  color: #8a8d91;
  margin-bottom: 12px;
}

.msg-other { margin-bottom: 8px; }

.bubble-other {
  display: block;
  background: #ffffff;
  border: 1px solid #dde1e7;
  border-radius: 0 12px 12px 12px;
  padding: 7px 12px;
}

.bubble-sender {
  font-size: 1rem;
  font-weight: bold;
  color: #65676b;
  margin-bottom: 2px;
}

.bubble-text { font-size: 1rem; color: #1c1e21; }
.bubble-time { font-size: 1rem; color: #b0b3b8; margin-top: 2px; }

/* Self messages — right-aligned via a full-width table */
.msg-self-wrap {
  width: 100%;
  margin-bottom: 8px;
}

.msg-self-wrap td { vertical-align: top; }

.bubble-self {
  display: block;
  background: #0084ff;
  border: 1px solid #0084ff;
  border-radius: 12px 0 12px 12px;
  padding: 7px 12px;
}

.bubble-text-self  { font-size: 1rem; color: #ffffff; }
.bubble-time-self  { font-size: 1rem; color: #cce4ff; margin-top: 2px; }

.empty-history {
  text-align: center;
  padding: 40px 0;
  color: #65676b;
  font-size: 1rem;
}

/* ── Compose ──────────────────────────────────────────────────────── */
.compose {
  padding: 10px 14px;
  background: #ffffff;
  border-top: 1px solid #dde1e7;
}
</style>
`

// renderSidebar builds the contact list pane.
func renderSidebar(state AppState) string {
	var b strings.Builder
	b.WriteString(baseStyle)
	b.WriteString(`<div class="sidebar-title">Chats</div>`)

	for i, c := range state.Contacts {
		history := state.History[c.Name]
		preview := "No messages yet"
		if len(history) > 0 {
			last := history[len(history)-1]
			prefix := ""
			if last.Self {
				prefix = "You: "
			}
			preview = prefix + truncate(last.Text, 30)
		}

		activeClass := "contact"
		if i == state.ActiveContact {
			activeClass = "contact contact-active"
		}

		fmt.Fprintf(&b,
			`<a href="select-contact:%d" class="%s">`+
				`<div class="contact-name">%s</div>`+
				`<div class="contact-preview">%s</div>`+
				`</a>`,
			i, activeClass,
			html.EscapeString(c.Name),
			html.EscapeString(preview),
		)
	}

	return b.String()
}

// renderHistory builds the message history pane.
func renderHistory(state AppState) string {
	contact := state.Contacts[state.ActiveContact]
	history := state.History[contact.Name]

	var b strings.Builder
	b.WriteString(baseStyle)
	b.WriteString(`<div class="history">`)

	if len(history) == 0 {
		fmt.Fprintf(&b,
			`<div class="empty-history">No messages yet. Say hi to %s!</div>`,
			html.EscapeString(contact.Name))
	} else {
		fmt.Fprintf(&b,
			`<div class="day-divider">%s</div>`,
			history[0].Time.Format("January 2, 2006"))

		for _, msg := range history {
			if msg.Self {
				// Right-align using a two-cell table: spacer | bubble
				fmt.Fprintf(&b,
					`<table width="100%%" class="msg-self-wrap"><tr>`+
						`<td style="width:20%%">&nbsp;</td>`+
						`<td><div class="bubble-self">`+
						`<div class="bubble-text-self">%s</div>`+
						`<div class="bubble-time-self">%s</div>`+
						`</div></td>`+
						`</tr></table>`,
					html.EscapeString(msg.Text), msg.Time.Format("15:04"))
			} else {
				fmt.Fprintf(&b,
					`<div class="msg-other"><div class="bubble-other">`+
						`<div class="bubble-sender">%s</div>`+
						`<div class="bubble-text">%s</div>`+
						`<div class="bubble-time">%s</div>`+
						`</div></div>`,
					html.EscapeString(msg.From), html.EscapeString(msg.Text), msg.Time.Format("15:04"))
			}
		}
	}

	b.WriteString(`</div>`)
	return b.String()
}

// renderCompose builds the message input pane.
func renderCompose(state AppState) string {
	contact := state.Contacts[state.ActiveContact]
	placeholder := fmt.Sprintf("Message %s...", contact.Name)

	return baseStyle + fmt.Sprintf(
		`<div class="compose"><ui-input id="draft" type="text" placeholder="%s" value=""></ui-input></div>`,
		html.EscapeString(placeholder))
}

// truncate shortens s to at most n runes.
func truncate(s string, n int) string {
	runes := []rune(s)
	if len(runes) <= n {
		return s
	}
	return string(runes[:n]) + "..."
}

// fakeReply returns a canned reply for the given contact.
func fakeReply(contact string) string {
	replies := map[string][]string{
		"Alice": {"Hey! What's up?", "That sounds great!", "Sure, let's do it.", "Haha yeah totally.", "Sounds good to me!"},
		"Bob":   {"Yo!", "For real?", "lol ok", "Nice.", "Sure thing."},
		"Carol": {"Oh interesting!", "Tell me more.", "I was just thinking about that.", "Absolutely!", "Makes sense."},
		"Dave":  {"On it.", "Can we talk later?", "Busy atm, will respond soon.", "Got it!", "Will do."},
	}
	opts := replies[contact]
	if len(opts) == 0 {
		return "..."
	}
	return opts[time.Now().UnixNano()%int64(len(opts))]
}

func main() {
	now := time.Now()

	var (
		mu    sync.Mutex
		state = AppState{
			Contacts: []Contact{
				{Name: "Alice"},
				{Name: "Bob"},
				{Name: "Carol"},
				{Name: "Dave"},
			},
			ActiveContact: 0,
			History: map[string][]Message{
				"Alice": {
					{From: "Alice", Text: "Hey, how are you?", Time: now.Add(-10 * time.Minute)},
					{From: "You", Text: "Doing great, thanks!", Time: now.Add(-9 * time.Minute), Self: true},
					{From: "Alice", Text: "Glad to hear it!", Time: now.Add(-8 * time.Minute)},
				},
				"Bob":   {},
				"Carol": {},
				"Dave":  {},
			},
		}
	)

	client, err := stdui.Start("./build/stdui", stdui.Settings{
		Title:          "Chat",
		WindowWidth:    stdui.Ptr(1000),
		WindowHeight:   stdui.Ptr(650),
		Resizable:      stdui.Ptr(true),
		BaseFontSize:   stdui.Ptr(16.0),
		FontRegular:    "./assets/fonts/Iosevka-Regular.ttf",
		FontBold:       "./assets/fonts/Iosevka-Bold.ttf",
		FontItalic:     "./assets/fonts/Iosevka-Italic.ttf",
		FontBoldItalic: "./assets/fonts/Iosevka-BoldItalic.ttf",
	})
	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to start stdui: %v\n", err)
		os.Exit(1)
	}

	render := func() {
		client.UpdatePaneContent("sidebar", renderSidebar(state)) //nolint:errcheck
		client.UpdatePaneContent("history", renderHistory(state)) //nolint:errcheck
		client.UpdatePaneContent("compose", renderCompose(state)) //nolint:errcheck
		client.ScrollToBottom("history")                          //nolint:errcheck
	}

	client.OnReady(func() {
		client.SetPaneLayout(stdui.HSplit([]*stdui.PaneNode{ //nolint:errcheck
			stdui.Pane("sidebar", 1),
			stdui.VSplit([]*stdui.PaneNode{
				stdui.Pane("history", 1),
				stdui.Pane("compose", 50),
			}, 3),
		}))

		mu.Lock()
		defer mu.Unlock()
		render()
	})

	client.OnLog(func(namespace, message string) {
		fmt.Fprintf(os.Stderr, "[log] %s: %s\n", namespace, message)
	})

	client.OnError(func(err error) {
		fmt.Fprintf(os.Stderr, "[error] %v\n", err)
	})

	client.OnInputChanged(func(id, value, _ string) {
		if id != "draft" {
			return
		}
		text := strings.TrimSpace(value)
		if text == "" {
			return
		}

		mu.Lock()
		contact := state.Contacts[state.ActiveContact]
		state.History[contact.Name] = append(state.History[contact.Name], Message{
			From: "You",
			Text: text,
			Time: time.Now(),
			Self: true,
		})
		render()
		mu.Unlock()

		client.SetValue("draft", "")                  //nolint:errcheck
		client.PlaySound("./assets/sounds/click.mp3") //nolint:errcheck

		go func(name string) {
			time.Sleep(time.Duration(800+time.Now().UnixNano()%1200) * time.Millisecond)
			mu.Lock()
			defer mu.Unlock()
			state.History[name] = append(state.History[name], Message{
				From: name,
				Text: fakeReply(name),
				Time: time.Now(),
			})
			if state.Contacts[state.ActiveContact].Name == name {
				render()
			} else {
				client.UpdatePaneContent("sidebar", renderSidebar(state)) //nolint:errcheck
			}
			client.PlaySound("./assets/sounds/click.mp3") //nolint:errcheck
		}(contact.Name)
	})

	client.OnURLClicked(func(url, _ string) {
		var idx int
		if _, err := fmt.Sscanf(url, "select-contact:%d", &idx); err != nil {
			return
		}

		mu.Lock()
		defer mu.Unlock()

		if idx < 0 || idx >= len(state.Contacts) {
			return
		}
		state.ActiveContact = idx
		render()
		client.PlaySound("./assets/sounds/click.mp3") //nolint:errcheck
	})

	client.Wait()
}
