package main

import (
	"fmt"
	"html"
	"os"
	"strings"
	"sync"
	"sync/atomic"

	"github.com/BigJk/stdui"
)

// Priority levels for tasks.
type Priority int

const (
	PriorityLow Priority = iota
	PriorityMedium
	PriorityHigh
)

func (p Priority) String() string {
	switch p {
	case PriorityHigh:
		return "High"
	case PriorityMedium:
		return "Medium"
	default:
		return "Low"
	}
}

func (p Priority) BadgeStyle() string {
	switch p {
	case PriorityHigh:
		return "background:#ffd8d8;color:#a10000;border:1px solid #f5c0c0;"
	case PriorityMedium:
		return "background:#fff3cd;color:#7d5a00;border:1px solid #ffe08a;"
	default:
		return "background:#d4edda;color:#155724;border:1px solid #c3e6cb;"
	}
}

// Task represents a single to-do item.
type Task struct {
	ID       int
	Title    string
	Done     bool
	Priority Priority
}

// AppState holds all mutable state for the app.
type AppState struct {
	Tasks       []Task
	Filter      string // "all", "active", "done"
	NewTitle    string
	NewPriority string
}

// nextID is an atomic counter for generating unique task IDs.
var nextID atomic.Int32

func newID() int {
	return int(nextID.Add(1))
}

const baseStyle = `
<style>
* { box-sizing: border-box; margin: 0; padding: 0; }

body {
  font-family: sans-serif;
  background: #f6f8fa;
  color: #1f2328;
  padding: 16px;
}

/* ── Cards ──────────────────────────────────────────────────────────── */
.card {
  background: #fff;
  border: 1px solid #d0d7de;
  border-radius: 8px;
  padding: 14px 16px;
  margin-bottom: 12px;
}

.card-title {
  font-size: 1rem;
  font-weight: bold;
  color: #656d76;
  text-transform: uppercase;
  letter-spacing: 1px;
  margin-bottom: 10px;
}

/* ── Form rows ──────────────────────────────────────────────────────── */
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

/* ── Stats ──────────────────────────────────────────────────────────── */
.stats {
  color: #656d76;
  font-size: 1rem;
}

.stat-val {
  font-weight: bold;
  color: #1f2328;
}

/* ── Filter buttons ─────────────────────────────────────────────────── */
.filter-btn-row {
  display: flex;
  flex-direction: row;
  margin-bottom: 6px;
}

.filter-btn-row ui-button {
  flex-grow: 1;
  margin-right: 6px;
}

.filter-btn-row ui-button:last-child {
  margin-right: 0;
}

/* ── Task cards ─────────────────────────────────────────────────────── */
.list-header {
  margin-bottom: 10px;
}

.list-header h2 {
  font-size: 1rem;
  font-weight: bold;
  color: #656d76;
  text-transform: uppercase;
  letter-spacing: 1px;
}

.task-card {
  background: #fff;
  border: 1px solid #d0d7de;
  border-radius: 8px;
  padding: 12px 14px;
  margin-bottom: 8px;
}

.task-card-done {
  background: #f6f8fa;
  border-color: #e4e7eb;
}

.task-title {
  font-size: 1.05rem;
  font-weight: bold;
  color: #1f2328;
  flex: 1;
}

.task-title-done {
  color: #8c959f;
  text-decoration: line-through;
}

.task-header {
  display: flex;
  flex-direction: row;
  align-items: center;
  margin-bottom: 8px;
}

.task-header .badge {
  flex-shrink: 0;
}

.badge {
  font-size: 1rem;
  font-weight: bold;
  padding: 2px 0;
  border-radius: 4px;
}

.task-actions {
  display: flex;
  flex-direction: row;
}

.task-actions ui-button {
  flex-grow: 1;
  margin-right: 6px;
}

.task-actions ui-button:last-child {
  margin-right: 0;
}

/* ── Empty state ────────────────────────────────────────────────────── */
.empty {
  text-align: center;
  padding: 40px 0;
  color: #8c959f;
}
</style>
`

// renderActions builds the HTML for the left "actions" pane.
func renderActions(state AppState) string {
	total := len(state.Tasks)
	done := 0
	for _, t := range state.Tasks {
		if t.Done {
			done++
		}
	}
	active := total - done

	newPriority := state.NewPriority
	if newPriority == "" {
		newPriority = "Medium"
	}

	return baseStyle + fmt.Sprintf(`
<div class="card" style="margin-bottom:12px;">
  <div style="font-size:1.6rem;font-weight:bold;color:#1f2328;">Task Manager</div>
  <div style="color:#656d76;font-size:1rem;margin-top:4px;">Keep track of what needs to get done.</div>
</div>

<div class="card">
  <div class="card-title">New Task</div>
  <div class="field">
    <span class="field-label">Title</span>
    <ui-input id="new-title" type="text" placeholder="What needs doing?" value="%s"></ui-input>
  </div>
  <div class="field">
    <span class="field-label">Priority</span>
    <ui-select id="new-priority" options="Low,Medium,High" value="%s"></ui-select>
  </div>
  <ui-button id="btn-add-task" text="Add Task" action="add-task" tooltip="Add a new task"></ui-button>
</div>

<div class="card">
  <div class="card-title">Stats</div>
  <div class="stats">
    <span class="stat-val">%d</span> total &nbsp;|&nbsp;
    <span class="stat-val">%d</span> active &nbsp;|&nbsp;
    <span class="stat-val">%d</span> done
  </div>
</div>

<div class="card">
  <div class="card-title">Filter</div>
  <div class="filter-btn-row">
    <ui-button id="btn-filter-all"    text="All"    action="filter-all"    tooltip="Show all tasks"></ui-button>
    <ui-button id="btn-filter-active" text="Active" action="filter-active" tooltip="Show active tasks"></ui-button>
    <ui-button id="btn-filter-done"   text="Done"   action="filter-done"   tooltip="Show completed tasks"></ui-button>
  </div>
  <ui-button id="btn-clear-done" text="Clear Done" action="clear-done" tooltip="Remove all completed tasks"></ui-button>
</div>
`,
		html.EscapeString(state.NewTitle), newPriority,
		total, active, done,
	)
}

// renderTasks builds the HTML for the right "tasks" pane.
func renderTasks(state AppState) string {
	// Filter tasks
	var visible []Task
	for _, t := range state.Tasks {
		switch state.Filter {
		case "active":
			if !t.Done {
				visible = append(visible, t)
			}
		case "done":
			if t.Done {
				visible = append(visible, t)
			}
		default:
			visible = append(visible, t)
		}
	}

	activeFilter := state.Filter
	if activeFilter == "" {
		activeFilter = "all"
	}

	var taskListHTML strings.Builder
	if len(visible) == 0 {
		taskListHTML.WriteString(`<div class="empty">Nothing here yet.</div>`)
	} else {
		for _, t := range visible {
			cardClass := "task-card"
			titleClass := "task-title"
			if t.Done {
				cardClass += " task-card-done"
				titleClass += " task-title-done"
			}

			toggleLabel := "Mark done"
			if t.Done {
				toggleLabel = "Mark active"
			}

			fmt.Fprintf(&taskListHTML, `
<div class="%s">
  <div class="task-header">
    <span class="%s">%s</span>
    <span class="badge" style="%s">&nbsp;%s&nbsp;</span>
  </div>
  <div class="task-actions">
    <ui-button id="toggle-%d" text="%s" action="toggle-%d" tooltip="Toggle completion status"></ui-button>
    <ui-button id="delete-%d" text="Delete" action="delete-%d" tooltip="Delete this task"></ui-button>
  </div>
</div>`,
				cardClass,
				titleClass, html.EscapeString(t.Title),
				t.Priority.BadgeStyle(), t.Priority.String(),
				t.ID, toggleLabel, t.ID,
				t.ID, t.ID)
		}
	}

	return baseStyle + fmt.Sprintf(`
<div class="list-header">
  <h2>Tasks &mdash; %s</h2>
</div>
%s
`,
		activeFilter,
		taskListHTML.String(),
	)
}

func parsePriority(s string) Priority {
	switch strings.ToLower(s) {
	case "high":
		return PriorityHigh
	case "low":
		return PriorityLow
	default:
		return PriorityMedium
	}
}

func main() {
	var (
		mu    sync.Mutex
		state = AppState{
			Filter:      "all",
			NewPriority: "Medium",
			Tasks: []Task{
				{ID: newID(), Title: "Buy groceries", Priority: PriorityLow},
				{ID: newID(), Title: "Write unit tests", Priority: PriorityHigh},
				{ID: newID(), Title: "Review pull request", Priority: PriorityMedium},
			},
		}
		// pendingDeleteID is set when a delete confirmation is in flight.
		pendingDeleteID int
	)

	client, err := stdui.Start("./build/stdui", stdui.Settings{
		Title:          "Task Manager",
		WindowWidth:    stdui.Ptr(1100),
		WindowHeight:   stdui.Ptr(700),
		Resizable:      stdui.Ptr(true),
		BaseFontSize:   stdui.Ptr(18.0),
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
		client.UpdatePaneContent("actions", renderActions(state)) //nolint:errcheck
		client.UpdatePaneContent("tasks", renderTasks(state))     //nolint:errcheck
	}

	client.OnReady(func() {
		// Set up a horizontal split: actions pane (1 part) | tasks pane (2 parts).
		client.SetPaneLayout(stdui.HSplit([]*stdui.PaneNode{ //nolint:errcheck
			stdui.Pane("actions", 1),
			stdui.Pane("tasks", 2),
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
		mu.Lock()
		defer mu.Unlock()
		switch id {
		case "new-title":
			state.NewTitle = value
		case "new-priority":
			state.NewPriority = value
		}
		// No re-render needed — inputs manage their own display state.
	})

	client.OnButtonClicked(func(attrs map[string]string, _ string) {
		action := attrs["action"]

		mu.Lock()
		defer mu.Unlock()

		switch {
		case action == "add-task":
			title := strings.TrimSpace(state.NewTitle)
			if title == "" {
				return
			}
			state.Tasks = append(state.Tasks, Task{
				ID:       newID(),
				Title:    title,
				Priority: parsePriority(state.NewPriority),
			})
			state.NewTitle = ""
			render()

		case action == "filter-all":
			state.Filter = "all"
			render()

		case action == "filter-active":
			state.Filter = "active"
			render()

		case action == "filter-done":
			state.Filter = "done"
			render()

		case action == "clear-done":
			var remaining []Task
			for _, t := range state.Tasks {
				if !t.Done {
					remaining = append(remaining, t)
				}
			}
			state.Tasks = remaining
			render()

		case strings.HasPrefix(action, "toggle-"):
			var id int
			fmt.Sscanf(action, "toggle-%d", &id)
			for i := range state.Tasks {
				if state.Tasks[i].ID == id {
					state.Tasks[i].Done = !state.Tasks[i].Done
					break
				}
			}
			render()

		case strings.HasPrefix(action, "delete-"):
			var id int
			fmt.Sscanf(action, "delete-%d", &id)
			title := ""
			for _, t := range state.Tasks {
				if t.ID == id {
					title = t.Title
					break
				}
			}
			pendingDeleteID = id
			// Release lock before sending over the pipe to avoid a deadlock.
			mu.Unlock()
			client.Confirm(stdui.ConfirmRequest{ //nolint:errcheck
				ID:         fmt.Sprintf("delete-%d", id),
				Question:   fmt.Sprintf("Delete \"%s\"?", title),
				Title:      "Delete Task",
				OKText:     "Delete",
				CancelText: "Cancel",
			})
			mu.Lock()
		}
	})

	client.OnConfirmResult(func(id string, result bool) {
		if !result {
			return
		}
		var taskID int
		if _, err := fmt.Sscanf(id, "delete-%d", &taskID); err != nil {
			return
		}

		mu.Lock()
		defer mu.Unlock()

		if pendingDeleteID != taskID {
			return
		}
		var remaining []Task
		for _, t := range state.Tasks {
			if t.ID != taskID {
				remaining = append(remaining, t)
			}
		}
		state.Tasks = remaining
		pendingDeleteID = 0
		render()
	})

	client.Wait()
}
