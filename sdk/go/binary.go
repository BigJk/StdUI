package stdui

import (
	"archive/zip"
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"os"
	"path/filepath"
	"runtime"
	"strings"
)

const releaseAPI = "https://api.github.com/repos/BigJk/StdUI/releases/latest"

// releaseAsset is a single asset entry from the GitHub releases API.
type releaseAsset struct {
	Name               string `json:"name"`
	BrowserDownloadURL string `json:"browser_download_url"`
}

// releaseResponse is the subset of the GitHub releases API response we need.
type releaseResponse struct {
	TagName string         `json:"tag_name"`
	Assets  []releaseAsset `json:"assets"`
}

// platformAssetName returns the platform-specific release zip name for the
// current OS and architecture, e.g. "stdui-linux.zip". Returns an error if
// the platform is not supported.
func platformAssetName() (string, error) {
	switch runtime.GOOS {
	case "linux":
		return "stdui-linux.zip", nil
	case "darwin":
		if runtime.GOARCH == "arm64" {
			return "stdui-macos-arm.zip", nil
		}
		return "stdui-macos-intel.zip", nil
	case "windows":
		return "stdui-windows-msvc.zip", nil
	default:
		return "", fmt.Errorf("unsupported platform: %s/%s", runtime.GOOS, runtime.GOARCH)
	}
}

// fetchLatestRelease queries the GitHub API for the latest StdUI release.
func fetchLatestRelease() (releaseResponse, error) {
	resp, err := http.Get(releaseAPI)
	if err != nil {
		return releaseResponse{}, fmt.Errorf("fetching release info: %w", err)
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		return releaseResponse{}, fmt.Errorf("GitHub API returned %s", resp.Status)
	}

	var rel releaseResponse
	if err := json.NewDecoder(resp.Body).Decode(&rel); err != nil {
		return releaseResponse{}, fmt.Errorf("parsing release info: %w", err)
	}
	return rel, nil
}

// downloadBytes fetches url and returns the response body as a byte slice.
func downloadBytes(url string) ([]byte, error) {
	resp, err := http.Get(url)
	if err != nil {
		return nil, fmt.Errorf("downloading %s: %w", url, err)
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		return nil, fmt.Errorf("downloading %s: HTTP %s", url, resp.Status)
	}

	data, err := io.ReadAll(resp.Body)
	if err != nil {
		return nil, fmt.Errorf("reading download body: %w", err)
	}
	return data, nil
}

// extractReleaseZip extracts all files from the "release/" folder inside the
// zip archive data into destDir, stripping the "release/" path prefix so that
// the files land directly in destDir.
func extractReleaseZip(data []byte, destDir string) error {
	zr, err := zip.NewReader(bytes.NewReader(data), int64(len(data)))
	if err != nil {
		return fmt.Errorf("opening zip: %w", err)
	}

	for _, f := range zr.File {
		if !strings.HasPrefix(f.Name, "release/") {
			continue
		}

		rel := strings.TrimPrefix(f.Name, "release/")
		if rel == "" {
			continue
		}

		dest := filepath.Join(destDir, filepath.FromSlash(rel))

		if f.FileInfo().IsDir() {
			if err := os.MkdirAll(dest, 0755); err != nil {
				return fmt.Errorf("creating directory %s: %w", dest, err)
			}
			continue
		}

		if err := os.MkdirAll(filepath.Dir(dest), 0755); err != nil {
			return fmt.Errorf("creating parent directory for %s: %w", dest, err)
		}

		rc, err := f.Open()
		if err != nil {
			return fmt.Errorf("opening zip entry %s: %w", f.Name, err)
		}

		outBytes, err := io.ReadAll(rc)
		rc.Close()
		if err != nil {
			return fmt.Errorf("reading zip entry %s: %w", f.Name, err)
		}

		mode := f.Mode()
		if mode == 0 {
			mode = 0644
		}
		if err := os.WriteFile(dest, outBytes, mode); err != nil {
			return fmt.Errorf("writing %s: %w", dest, err)
		}
	}
	return nil
}

// EnsureBinary checks whether the stdui binary already exists at binaryPath
// and downloads it from the latest GitHub release if not. The contents of the
// "release/" folder inside the zip are extracted directly into destDir.
//
// binaryPath is the path to check and write the binary to, e.g. "./stdui".
// Pass an empty string to use the platform default ("stdui" or "stdui.exe")
// inside destDir. Pass "" for destDir to use the current working directory.
//
// On macOS and Linux the binary is made executable automatically.
// The function is a no-op if the binary is already present.
func EnsureBinary(binaryPath string, destDir string) error {
	if destDir == "" {
		destDir = "."
	}

	if binaryPath == "" {
		if runtime.GOOS == "windows" {
			binaryPath = filepath.Join(destDir, "stdui.exe")
		} else {
			binaryPath = filepath.Join(destDir, "stdui")
		}
	}

	if _, err := os.Stat(binaryPath); err == nil {
		return nil
	}

	assetName, err := platformAssetName()
	if err != nil {
		return err
	}

	rel, err := fetchLatestRelease()
	if err != nil {
		return err
	}

	var downloadURL string
	for _, a := range rel.Assets {
		if a.Name == assetName {
			downloadURL = a.BrowserDownloadURL
			break
		}
	}
	if downloadURL == "" {
		return fmt.Errorf("asset %q not found in release %s", assetName, rel.TagName)
	}

	data, err := downloadBytes(downloadURL)
	if err != nil {
		return err
	}

	if err := extractReleaseZip(data, destDir); err != nil {
		return err
	}

	if runtime.GOOS != "windows" {
		if err := os.Chmod(binaryPath, 0755); err != nil {
			return fmt.Errorf("chmod %s: %w", binaryPath, err)
		}
	}

	return nil
}
