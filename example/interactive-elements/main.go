package main

import (
	"fmt"
	"os"
	"sync"

	"github.com/BigJk/stdui/sdk/go"
)

// FormState holds the current values of all interactive elements.
type FormState struct {
	Name          string
	Age           string
	Theme         string
	Volume        string
	Enabled       string
	Notes         string
	FetchedID     string
	FetchedValue  string
	DroppedFiles  []string
	Progress      float64
	SelectedFile  string
	SelectedDir   string
	SaveFile      string
	ConfirmResult string
	AccentColor   string
	OverlayColor  string
}

// stylesheet returns the CSS applied to every page render.
const stylesheet = `
<style>
/* ── Reset ──────────────────────────────────────────────────────────── */
* { box-sizing: border-box; margin: 0; padding: 0; }

/* ── Page ───────────────────────────────────────────────────────────── */
body {
  color: #1f2328;
  background: #ffffff;
  padding: 10px;
  margin: 0;
}

/* ── Typography ─────────────────────────────────────────────────────── */
h1 { font-size: 1.4rem; font-weight: bold;  color: #1f2328; margin-bottom: 4px; }
h2 { font-size: 1.1rem; font-weight: bold;  color: #1f2328; margin-bottom: 12px; }
h3 { font-size: 1rem;   font-weight: bold;  color: #656d76; margin-bottom: 6px; text-transform: uppercase; letter-spacing: 1px; }
p  { margin-bottom: 4px; color: #1f2328; }
b  { font-weight: bold; color: #1f2328; }
i  { color: #656d76; }
hr { border: none; border-top: 1px solid #d0d7de; margin: 14px 0; }

/* ── Layout card ────────────────────────────────────────────────────── */
.card {
  background: #fcfcfd;
  border: 1px solid #d0d7de;
  border-radius: 6px;
  padding: 12px 16px;
  margin-bottom: 10px;
}

.card-title {
  font-size: 1rem;
  font-weight: bold;
  color: #656d76;
  text-transform: uppercase;
  letter-spacing: 1px;
  margin-bottom: 8px;
}

/* ── Two-column form row ────────────────────────────────────────────── */
.field {
  display: block;
  margin-bottom: 8px;
}

.field-label {
  display: block;
  font-size: 1rem;
  font-weight: bold;
  color: #656d76;
  margin-bottom: 3px;
}

/* ── Info / result box ──────────────────────────────────────────────── */
.info-box {
  background: #ddf4ff;
  border: 1px solid #54aeff;
  border-radius: 6px;
  padding: 8px 12px;
  margin-bottom: 10px;
  color: #0550ae;
}

/* ── Value table ────────────────────────────────────────────────────── */
.kv-row {
  display: block;
  margin-bottom: 3px;
  color: #656d76;
}

.kv-key {
  color: #656d76;
}

.kv-val {
  font-weight: bold;
  color: #1f2328;
}

/* ── File drop hint ─────────────────────────────────────────────────── */
.drop-hint {
  color: #656d76;
  font-style: italic;
}

.drop-item {
  color: #0969da;
  margin-bottom: 2px;
}

/* ── Section spacing ────────────────────────────────────────────────── */
.section { margin-bottom: 12px; }
</style>
`

// formHTML returns the HTML for the interactive elements demo.
func formHTML(state FormState) string {
	fetchedSection := ""
	if state.FetchedID != "" {
		fetchedSection = fmt.Sprintf(
			`<div class="info-box">get-value result &mdash; <b>%s</b> = <b>%s</b></div>`,
			state.FetchedID, state.FetchedValue,
		)
	}

	confirmSection := ""
	if state.ConfirmResult != "" {
		confirmSection = fmt.Sprintf(
			`<div class="info-box">confirm-result &mdash; <b>%s</b></div>`,
			state.ConfirmResult,
		)
	}

	droppedSection := ""
	for _, p := range state.DroppedFiles {
		droppedSection += fmt.Sprintf(`<p class="drop-item">%s</p>`, p)
	}
	if droppedSection == "" {
		droppedSection = `<p class="drop-hint">Drop files onto the window to see them here.</p>`
	}

	overlayPct := fmt.Sprintf("%.0f%%", state.Progress*100)

	return stylesheet + fmt.Sprintf(`
<h2>Elements Demo</h2>

<!-- Inputs card -->
<div class="card">
  <div class="card-title">Input Fields</div>

  <div class="field">
    <span class="field-label">Name</span>
    <ui-input id="name" type="text" placeholder="Enter your name" value="%s"></ui-input>
  </div>

  <div class="field">
    <span class="field-label">Age</span>
    <ui-input id="age" type="number" min="0" max="120" value="%s"></ui-input>
  </div>

  <div class="field">
    <span class="field-label">Notes</span>
    <ui-textarea id="notes" placeholder="Enter your notes here..." value="%s"></ui-textarea>
  </div>
</div>

<!-- Controls card -->
<div class="card">
  <div class="card-title">Controls</div>

  <div class="field">
    <span class="field-label">Theme</span>
    <ui-select id="theme" options="Light|Dark|System" value="%s"></ui-select>
  </div>

  <div class="field">
    <span class="field-label">Volume</span>
    <ui-slider id="volume" min="0" max="1" value="%s"></ui-slider>
  </div>

  <div class="field">
    <ui-checkbox id="enabled" label="Enable notifications" checked="%s"></ui-checkbox>
  </div>
</div>

<!-- Color card -->
<div class="card">
  <div class="card-title">Color Picker</div>

  <div class="field">
    <span class="field-label">Accent (no alpha)</span>
    <ui-color id="accent-color" value="%s"></ui-color>
  </div>

  <div class="field">
    <span class="field-label">Overlay (with alpha)</span>
    <ui-color id="overlay-color" value="%s" alpha="true"></ui-color>
  </div>
</div>

<!-- Progress card -->
<div class="card">
  <div class="card-title">Progress</div>
  <div class="field">
    <ui-progress id="progress" value="%.2f" min="0" max="1" overlay="%s"></ui-progress>
  </div>
  <ui-button text="-10%%" action="progress-dec" tooltip="Decrease progress"></ui-button>
  <ui-button text="+10%%" action="progress-inc" tooltip="Increase progress"></ui-button>
</div>

<!-- File / Folder card -->
<div class="card">
  <div class="card-title">File &amp; Folder</div>

  <div class="field">
    <span class="field-label">File</span>
    <ui-file-select id="selected-file" label="Browse..." filter="png,jpg,txt" value="%s"></ui-file-select>
  </div>

  <div class="field">
    <span class="field-label">Folder</span>
    <ui-folder-select id="selected-dir" label="Browse..." value="%s"></ui-folder-select>
  </div>

  <div class="field">
    <span class="field-label">Save As</span>
    <ui-file-save id="save-file" label="Save..." filter="txt,csv" value="%s"></ui-file-save>
  </div>
</div>

<!-- Dropped files card -->
<div class="card">
  <div class="card-title">Dropped Files</div>
  %s
</div>

<hr>

<!-- Current values -->
<div class="card">
  <div class="card-title">Current Values</div>
  <p class="kv-row"><span class="kv-key">Name: </span><span class="kv-val">%s</span></p>
  <p class="kv-row"><span class="kv-key">Age: </span><span class="kv-val">%s</span></p>
  <p class="kv-row"><span class="kv-key">Theme: </span><span class="kv-val">%s</span></p>
  <p class="kv-row"><span class="kv-key">Volume: </span><span class="kv-val">%s</span></p>
  <p class="kv-row"><span class="kv-key">Notifications: </span><span class="kv-val">%s</span></p>
  <p class="kv-row"><span class="kv-key">Accent: </span><span class="kv-val">%s</span></p>
  <p class="kv-row"><span class="kv-key">Overlay: </span><span class="kv-val">%s</span></p>
  <p class="kv-row"><span class="kv-key">File: </span><span class="kv-val">%s</span></p>
  <p class="kv-row"><span class="kv-key">Folder: </span><span class="kv-val">%s</span></p>
  <p class="kv-row"><span class="kv-key">Save As: </span><span class="kv-val">%s</span></p>
</div>

%s

<!-- Toast card -->
<div class="card">
  <div class="card-title">Toast</div>
  <ui-button text="Show Toast" action="show-toast" tooltip="Display a toast notification"></ui-button>
</div>

<!-- Fetch name button card -->
<div class="card">
	<ui-button style="margin-bottom: 10px;" text="Fetch Name" action="fetch-name" tooltip="Fetch the current name via get-value"></ui-button>
	<ui-button style="margin-bottom: 10px;" text="Trigger Confirm" action="trigger-confirm" tooltip="Open a confirm dialog"></ui-button>
	<ui-button text="Reset" action="reset" tooltip="Reset all fields to defaults"></ui-button>
</div>

%s
`,
		// input fields
		state.Name, state.Age, state.Notes,
		// controls
		state.Theme, state.Volume, state.Enabled,
		// colors
		state.AccentColor, state.OverlayColor,
		// progress
		state.Progress, overlayPct,
		// file / folder
		state.SelectedFile, state.SelectedDir, state.SaveFile,
		// dropped files
		droppedSection,
		// current values
		state.Name, state.Age, state.Theme, state.Volume, state.Enabled,
		state.AccentColor, state.OverlayColor,
		state.SelectedFile, state.SelectedDir, state.SaveFile,
		// fetched result
		fetchedSection,
		// confirm result
		confirmSection,
	)
}

func main() {
	defaultState := FormState{
		Age:          "25",
		Theme:        "Light",
		Volume:       "0.5",
		Enabled:      "true",
		Progress:     0.0,
		AccentColor:  "#0969da",
		OverlayColor: "#0969da80",
	}

	var (
		state = defaultState
		mu    sync.Mutex // guards state
	)

	client, err := stdui.Start("./build/stdui", stdui.Settings{
		Title:          "Interactive Elements Demo",
		WindowWidth:    stdui.Ptr(500),
		WindowHeight:   stdui.Ptr(700),
		Resizable:      stdui.Ptr(true),
		BaseFontSize:   stdui.Ptr(20.0),
		SfxVolume:      stdui.Ptr(0.5),
		FontRegular:    "./assets/fonts/Iosevka-Regular.ttf",
		FontBold:       "./assets/fonts/Iosevka-Bold.ttf",
		FontItalic:     "./assets/fonts/Iosevka-Italic.ttf",
		FontBoldItalic: "./assets/fonts/Iosevka-BoldItalic.ttf",
	})
	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to start stdui: %v\n", err)
		os.Exit(1)
	}

	client.OnReady(func() {
		mu.Lock()
		html := formHTML(state)
		mu.Unlock()
		client.UpdateContent(html) //nolint:errcheck
	})

	client.OnLog(func(namespace, message string) {
		fmt.Fprintf(os.Stderr, "[log] %s: %s\n", namespace, message)
	})

	client.OnError(func(err error) {
		fmt.Fprintf(os.Stderr, "[error] %v\n", err)
	})

	client.OnInputChanged(func(id, value, _ string) {
		mu.Lock()
		switch id {
		case "name":
			state.Name = value
		case "age":
			state.Age = value
		case "theme":
			state.Theme = value
		case "volume":
			state.Volume = value
		case "enabled":
			state.Enabled = value
		case "notes":
			state.Notes = value
		case "selected-file":
			state.SelectedFile = value
		case "selected-dir":
			state.SelectedDir = value
		case "save-file":
			state.SaveFile = value
		case "accent-color":
			state.AccentColor = value
		case "overlay-color":
			state.OverlayColor = value
		}
		html := formHTML(state)
		mu.Unlock()
		client.UpdateContent(html) //nolint:errcheck
	})

	client.OnButtonClicked(func(attrs map[string]string, _ string) {
		switch attrs["action"] {
		case "show-toast":
			mu.Lock()
			accent := state.AccentColor
			mu.Unlock()
			client.Toast(stdui.ToastRequest{ //nolint:errcheck
				Content: fmt.Sprintf(
					`<div style="background:#1f2328;color:#e6edf3;border-radius:8px;padding:10px 16px;border-left:4px solid %s;">
					   <b>Toast notification</b><br><span style="color:#7d8590;">This dismisses after 4s or on double-click.</span>
					 </div>`,
					accent,
				),
				Width: 320,
				TTL:   4.0,
			})

		case "fetch-name":
			client.GetValue("name") //nolint:errcheck

		case "trigger-confirm":
			client.Confirm(stdui.ConfirmRequest{ //nolint:errcheck
				ID:         "example-confirm",
				Question:   "Are you sure you want to proceed?",
				Title:      "Confirm Action",
				OKText:     "Yes, proceed",
				CancelText: "No, cancel",
			})

		case "progress-inc":
			mu.Lock()
			if state.Progress < 1.0 {
				state.Progress = min(state.Progress+0.1, 1.0)
			}
			html := formHTML(state)
			mu.Unlock()
			client.SetValue("progress", fmt.Sprintf("%.2f", state.Progress)) //nolint:errcheck
			client.UpdateContent(html)                                       //nolint:errcheck

		case "progress-dec":
			mu.Lock()
			if state.Progress > 0.0 {
				state.Progress = max(state.Progress-0.1, 0.0)
			}
			html := formHTML(state)
			mu.Unlock()
			client.SetValue("progress", fmt.Sprintf("%.2f", state.Progress)) //nolint:errcheck
			client.UpdateContent(html)                                       //nolint:errcheck

		case "reset":
			mu.Lock()
			state = defaultState
			html := formHTML(state)
			mu.Unlock()
			// Reset element values in stdui so the widgets reflect the new state.
			for id, val := range map[string]string{
				"name":          defaultState.Name,
				"age":           defaultState.Age,
				"theme":         defaultState.Theme,
				"volume":        defaultState.Volume,
				"enabled":       defaultState.Enabled,
				"notes":         defaultState.Notes,
				"progress":      fmt.Sprintf("%.2f", defaultState.Progress),
				"selected-file": defaultState.SelectedFile,
				"selected-dir":  defaultState.SelectedDir,
				"accent-color":  defaultState.AccentColor,
				"overlay-color": defaultState.OverlayColor,
			} {
				client.SetValue(id, val) //nolint:errcheck
			}
			client.SetTitle("Interactive Elements Demo") //nolint:errcheck
			client.UpdateContent(html)                   //nolint:errcheck
		}
	})

	client.OnFileDropped(func(path string) {
		mu.Lock()
		state.DroppedFiles = append(state.DroppedFiles, path)
		// Update the title to show how many files have been dropped.
		title := fmt.Sprintf("Interactive Elements Demo — %d file(s) dropped", len(state.DroppedFiles))
		html := formHTML(state)
		mu.Unlock()
		client.SetTitle(title)     //nolint:errcheck
		client.UpdateContent(html) //nolint:errcheck
	})

	client.OnValueResult(func(id, value string) {
		fmt.Fprintf(os.Stderr, "[info] value-result: %s = %q\n", id, value)
		mu.Lock()
		state.FetchedID = id
		state.FetchedValue = value
		html := formHTML(state)
		mu.Unlock()
		client.UpdateContent(html) //nolint:errcheck
	})

	client.OnConfirmResult(func(id string, result bool) {
		fmt.Fprintf(os.Stderr, "[info] confirm-result: %s = %v\n", id, result)
		mu.Lock()
		if result {
			state.ConfirmResult = fmt.Sprintf("✓ Confirmed (id: %s)", id)
		} else {
			state.ConfirmResult = fmt.Sprintf("✗ Cancelled (id: %s)", id)
		}
		html := formHTML(state)
		mu.Unlock()
		client.UpdateContent(html) //nolint:errcheck
	})

	client.Wait()
}
