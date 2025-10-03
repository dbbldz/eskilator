# Release Instructions

Quick reference for creating and publishing a new release.

## 1. Bump Version

```bash
# For bug fixes (0.9.1 -> 0.9.2)
npm run version:patch

# For new features (0.9.1 -> 0.10.0)
npm run version:minor

# For breaking changes (0.9.1 -> 1.0.0)
npm run version:major
```

This updates `package.json`. Then manually update:
- `CMakeLists.txt` - line 13: `VERSION "X.X.X"`

## 2. Update Changelog

Add release entry to `CHANGELOG.md`:
```markdown
## [X.X.X] - YYYY-MM-DD
### Added
- New features

### Changed
- Changes to existing features

### Fixed
- Bug fixes
```

## 3. Commit and Push

```bash
git add .
git commit -m "Release vX.X.X"
git push origin your-branch
```

## 4. Merge to Main

Create and merge PR to main branch.

## 5. Create Git Tag

```bash
# Switch to main and pull latest
git checkout main
git pull origin main

# Create annotated tag
git tag -a vX.X.X -m "Release vX.X.X"

# Push tag to GitHub
git push origin vX.X.X
```

## 6. Build Release Package

```bash
./build_release.sh
```

This will:
- Build universal binaries (Intel + Apple Silicon)
- Code sign with Developer ID
- Create installer `.pkg`
- Create distributable `.dmg`

Output: `release/Eskilator-vX.X.X.dmg`

## 7. Notarize with Apple

```bash
# Submit for notarization
xcrun notarytool submit release/Eskilator-vX.X.X.dmg \
  --keychain-profile "AC_PASSWORD" \
  --wait

# Staple the notarization ticket
xcrun stapler staple release/Eskilator-vX.X.X.dmg

# Verify
xcrun stapler validate release/Eskilator-vX.X.X.dmg
```

### First Time Setup

Generate app-specific password at https://appleid.apple.com, then:

```bash
xcrun notarytool store-credentials "AC_PASSWORD" \
  --apple-id "your-apple-id@email.com" \
  --team-id "BFPDK49ZHT" \
  --password "xxxx-xxxx-xxxx-xxxx"
```

## 8. Create GitHub Release

1. Go to https://github.com/dbbldz/eskilator/releases
2. Click "Draft a new release"
3. Choose tag: `vX.X.X`
4. Release title: `vX.X.X`
5. Copy changelog entry to description
6. Attach `release/Eskilator-vX.X.X.dmg`
7. Publish release

## 9. Test

Download and verify:
- DMG opens without Gatekeeper warnings
- Installer works
- Plugin loads in DAWs
- All features work correctly
