# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.9.2] - 2025-10-03
### Added
- JUCE framework now properly configured as a git submodule for easier cloning and setup.
- Universal binary support for macOS (i386, x86_64, arm64) in release builds to support 32-bit Intel, 64-bit Intel, and Apple Silicon Macs.

### Changed
- Release build script now automatically builds universal binaries with all architecture support.
- Build script supports optional `release` flag to build universal binaries for distribution.

## [0.9.1] - 2025-10-02
### Added
- Global transpose control (±24 semitones) and fine-tune (±100 cents) parameters, with corresponding UI knobs and state persistence.
- Automatic sample restoration when reopening projects: saved sessions now reload their associated sample files (with gain and transpose settings) whenever the audio files are still present on disk.
- Standalone release pipeline script (`build_release.sh`) that builds, codesigns, and packages AU/VST3 binaries into signed `.pkg` and `.dmg` installers, including a `--package-only` mode for regenerating installers without rebuilding binaries.

### Changed
- Updated UI layout to accommodate the new tuning controls while keeping the control groups compact and readable.
- Bumped the project version to `0.9.1` for the new feature set.

### Fixed
- Ensured user-loaded samples persist across host sessions by serializing their file paths and reloading them on state restore.
- Hardened release packaging so plugin artefacts are discovered reliably regardless of the CMake build folder structure.
