package stdui

// ColorScheme configures the UI color palette. All fields are hex color strings
// (e.g. "#1f2328" or "#0969da"). Omitted fields fall back to the built-in
// light theme defaults.
type ColorScheme struct {
	// Text is the primary foreground color.
	Text string `json:"text,omitempty"`
	// TextMuted is the secondary / muted foreground color.
	TextMuted string `json:"textMuted,omitempty"`
	// WindowBg is the background color of all pane windows.
	WindowBg string `json:"windowBg,omitempty"`
	// ElementBg is the default background color for interactive elements.
	ElementBg string `json:"elementBg,omitempty"`
	// ElementHovered is the background color when hovering an interactive element.
	ElementHovered string `json:"elementHovered,omitempty"`
	// ElementActive is the background color when pressing an interactive element.
	ElementActive string `json:"elementActive,omitempty"`
	// TitleBg is the background color of window title bars.
	TitleBg string `json:"titleBg,omitempty"`
	// Border is the color used for element and window borders.
	Border string `json:"border,omitempty"`
	// Primary is the accent / brand color used for buttons and highlights.
	Primary string `json:"primary,omitempty"`
	// Warn is the color used for warning indicators.
	Warn string `json:"warn,omitempty"`
	// Danger is the color used for destructive actions and error indicators.
	Danger string `json:"danger,omitempty"`
	// PrimaryTranslucent is a semi-transparent variant of Primary.
	PrimaryTranslucent string `json:"primaryTranslucent,omitempty"`
	// PrimarySubtle is a very lightly tinted variant of Primary for backgrounds.
	PrimarySubtle string `json:"primarySubtle,omitempty"`
	// TextSelectionBg is the background color of selected text.
	TextSelectionBg string `json:"textSelectionBg,omitempty"`
	// ModalDim is the overlay color behind modal dialogs.
	ModalDim string `json:"modalDim,omitempty"`
}

// Settings configures the stdui window and renderer. All fields are optional;
// omitted fields use stdui's built-in defaults.
type Settings struct {
	// Title is the window title bar text. Defaults to "StdUI".
	Title string `json:"title,omitempty"`

	// Resizable controls whether the window can be resized. Defaults to true.
	Resizable *bool `json:"resizable,omitempty"`

	// WindowWidth is the initial window width in pixels. Defaults to 800.
	WindowWidth *int `json:"windowWidth,omitempty"`

	// WindowHeight is the initial window height in pixels. Defaults to 600.
	WindowHeight *int `json:"windowHeight,omitempty"`

	// WindowMinWidth is the minimum allowed window width in pixels. Defaults to 400.
	WindowMinWidth *int `json:"windowMinWidth,omitempty"`

	// WindowMinHeight is the minimum allowed window height in pixels. Defaults to 300.
	WindowMinHeight *int `json:"windowMinHeight,omitempty"`

	// WindowMaxWidth is the maximum allowed window width in pixels. Defaults to 3840.
	WindowMaxWidth *int `json:"windowMaxWidth,omitempty"`

	// WindowMaxHeight is the maximum allowed window height in pixels. Defaults to 2160.
	WindowMaxHeight *int `json:"windowMaxHeight,omitempty"`

	// BaseFontSize is the base UI font size in pixels. Defaults to 16.0.
	BaseFontSize *float64 `json:"baseFontSize,omitempty"`

	// AudioVolume is the audio volume level (0.0–1.0). Defaults to 1.0.
	AudioVolume *float64 `json:"audioVolume,omitempty"`

	// TargetFPS is the target frame rate in frames per second. Defaults to 60.
	TargetFPS *int `json:"targetFps,omitempty"`

	// FontRegular is the path to the regular font file.
	FontRegular string `json:"fontRegular,omitempty"`

	// FontBold is the path to the bold font file.
	FontBold string `json:"fontBold,omitempty"`

	// FontItalic is the path to the italic font file.
	FontItalic string `json:"fontItalic,omitempty"`

	// FontBoldItalic is the path to the bold-italic font file.
	FontBoldItalic string `json:"fontBoldItalic,omitempty"`

	// ColorScheme overrides the built-in light theme. Omit to use the default.
	ColorScheme *ColorScheme `json:"colorScheme,omitempty"`
}

// Ptr returns a pointer to v. Useful for populating optional pointer fields in
// Settings without declaring intermediate variables.
//
//	client, _ := stdui.Start("./build/game", stdui.Settings{
//	    WindowWidth:  stdui.Ptr(800),
//	    WindowHeight: stdui.Ptr(600),
//	    BaseFontSize: stdui.Ptr(16.0),
//	})
func Ptr[T any](v T) *T { return &v }
