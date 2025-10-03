#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

PACKAGE_ONLY=0
VERSION_OVERRIDE=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --package-only)
            PACKAGE_ONLY=1
            shift
            ;;
        *)
            if [ -z "$VERSION_OVERRIDE" ]; then
                VERSION_OVERRIDE="$1"
            fi
            shift
            ;;
    esac
done

echo -e "${BLUE}🚀 Building Eskilator Release Package...${NC}"
echo ""

# Get version from package.json if it exists, otherwise prompt
if [ -f "package.json" ]; then
    VERSION=$(grep '"version"' package.json | head -1 | sed 's/.*"version": "\(.*\)".*/\1/')
    echo -e "${BLUE}Version from package.json: ${VERSION}${NC}"

    if [ -n "$VERSION_OVERRIDE" ]; then
        echo -e "${YELLOW}Overriding with command line argument: $VERSION_OVERRIDE${NC}"
        VERSION="$VERSION_OVERRIDE"
    fi
else
    # Get version from user or use default
    if [ -z "$VERSION_OVERRIDE" ]; then
        echo -e "${YELLOW}Enter version number (e.g., 1.0.0): ${NC}"
        read VERSION
        if [ -z "$VERSION" ]; then
            VERSION="1.0.0"
            echo -e "${YELLOW}Using default version: $VERSION${NC}"
        fi
    else
        VERSION="$VERSION_OVERRIDE"
    fi
fi

PLUGIN_NAME="Eskilator"
RELEASE_DIR="release/Eskilator-v${VERSION}"
PKG_NAME="Eskilator-v${VERSION}.pkg"
DMG_NAME="Eskilator-v${VERSION}.dmg"

echo -e "${BLUE}Building version: ${VERSION}${NC}"
echo ""

# Clean and create release directory
echo -e "${BLUE}📁 Preparing release directory...${NC}"
rm -rf release
mkdir -p "$RELEASE_DIR"
mkdir -p "release/pkg_root/Library/Audio/Plug-Ins/Components"
mkdir -p "release/pkg_root/Library/Audio/Plug-Ins/VST3"

# Clean build directory for fresh release build
if [ "$PACKAGE_ONLY" -eq 0 ]; then
    echo -e "${YELLOW}🧹 Cleaning build directory...${NC}"
    rm -rf build
    mkdir -p build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    cd ..

    # Build all targets
    echo -e "${BLUE}🔨 Building all targets...${NC}"
    cmake --build build --config Release --clean-first

    if [ $? -ne 0 ]; then
        echo -e "${RED}❌ Build failed!${NC}"
        exit 1
    fi

    echo -e "${GREEN}✅ Build successful!${NC}"
    echo ""
else
    echo -e "${YELLOW}⏭️  Skipping build; using existing artefacts.${NC}"
    echo ""
fi

# Find built plugins
ARTEFACTS_DIR="build/Eskilator_artefacts"
CONFIG_DIR="$ARTEFACTS_DIR/Release"

if [ -d "$CONFIG_DIR" ]; then
    AU_PLUGIN="$CONFIG_DIR/AU/${PLUGIN_NAME}.component"
    VST3_PLUGIN="$CONFIG_DIR/VST3/${PLUGIN_NAME}.vst3"
else
    AU_PLUGIN="$ARTEFACTS_DIR/AU/${PLUGIN_NAME}.component"
    VST3_PLUGIN="$ARTEFACTS_DIR/VST3/${PLUGIN_NAME}.vst3"
fi

# Verify plugins exist
if [ ! -d "$AU_PLUGIN" ]; then
    echo -e "${RED}❌ AU plugin not found at: $AU_PLUGIN${NC}"
    exit 1
fi

if [ ! -d "$VST3_PLUGIN" ]; then
    echo -e "${RED}❌ VST3 plugin not found at: $VST3_PLUGIN${NC}"
    exit 1
fi

echo -e "${BLUE}🔐 Code signing plugins with Developer ID...${NC}"

# Function to sign a plugin properly
sign_plugin() {
    local plugin_path="$1"
    local plugin_type="$2"

    echo -e "${YELLOW}   Signing $plugin_type...${NC}"

    # Check if we have Developer ID certificate
    if security find-identity -v -p codesigning | grep -q "Developer ID Application"; then
        CERT_NAME=$(security find-identity -v -p codesigning | grep "Developer ID Application" | head -1 | sed 's/.*"\(.*\)"/\1/')
        echo -e "${BLUE}     Using: $CERT_NAME${NC}"

        # Sign with hardened runtime and timestamp
        codesign --force --deep \
                 --sign "Developer ID Application" \
                 --timestamp \
                 --options runtime \
                 "$plugin_path"

        if [ $? -eq 0 ]; then
            echo -e "${GREEN}     ✅ Signed successfully${NC}"

            # Verify signature
            codesign --verify --verbose "$plugin_path"
            if [ $? -eq 0 ]; then
                echo -e "${GREEN}     ✅ Signature verified${NC}"
            else
                echo -e "${RED}     ❌ Signature verification failed${NC}"
                return 1
            fi
        else
            echo -e "${RED}     ❌ Signing failed${NC}"
            return 1
        fi
    else
        echo -e "${RED}     ❌ No Developer ID Application certificate found${NC}"
        echo -e "${YELLOW}     Run: security find-identity -v -p codesigning${NC}"
        exit 1
    fi
}

# Sign both plugins
sign_plugin "$AU_PLUGIN" "AU Plugin"
if [ $? -ne 0 ]; then
    echo -e "${RED}❌ AU signing failed${NC}"
    exit 1
fi

sign_plugin "$VST3_PLUGIN" "VST3 Plugin"
if [ $? -ne 0 ]; then
    echo -e "${RED}❌ VST3 signing failed${NC}"
    exit 1
fi

echo ""
echo -e "${BLUE}📦 Copying plugins to package root...${NC}"

# Copy signed plugins to package root for installer
cp -R "$AU_PLUGIN" "release/pkg_root/Library/Audio/Plug-Ins/Components/"
cp -R "$VST3_PLUGIN" "release/pkg_root/Library/Audio/Plug-Ins/VST3/"

# Create README for the DMG
echo -e "${BLUE}📝 Creating README...${NC}"
cat > "$RELEASE_DIR/README.txt" << EOF
Eskilator v${VERSION}
====================

Thank you for downloading Eskilator!

INSTALLATION
------------

Simply double-click the Eskilator installer package (.pkg) and follow
the on-screen instructions. The installer will automatically place the
plugins in the correct locations.

After installation, restart your DAW.

SYSTEM REQUIREMENTS
-------------------
- macOS 10.13 or later
- 64-bit AU or VST3 compatible host

SUPPORT
-------
For issues and support, please visit:
https://github.com/dbbldz/eskilator

To support this project...

Buy my music on Bandcamp:
https://dubbeldutch.bandcamp.com

Follow me on Instagram:
https://www.instagram.com/dutchdubs

---
Built on: $(date)
Signed with: Developer ID Application
EOF

echo ""
echo -e "${BLUE}📦 Creating installer package (.pkg)...${NC}"

# Check if we have Developer ID Installer certificate for signing the package
if security find-identity -v -p basic | grep -q "Developer ID Installer"; then
    INSTALLER_CERT=$(security find-identity -v -p basic | grep "Developer ID Installer" | head -1 | sed 's/.*"\(.*\)"/\1/')
    echo -e "${BLUE}Found installer certificate: $INSTALLER_CERT${NC}"
    SIGN_PKG=(--sign "$INSTALLER_CERT")
else
    echo -e "${YELLOW}⚠️  No Developer ID Installer certificate found${NC}"
    echo -e "${YELLOW}Package will be created unsigned${NC}"
    SIGN_PKG=()
fi

# Create the package
pkgbuild --root "release/pkg_root" \
         --identifier "com.dubbeldutch.eskilator" \
         --version "$VERSION" \
         --install-location "/" \
         "${SIGN_PKG[@]}" \
         "release/$PKG_NAME"

if [ $? -ne 0 ]; then
    echo -e "${RED}❌ Failed to create package${NC}"
    exit 1
fi

echo -e "${GREEN}✅ Package created: release/$PKG_NAME${NC}"

echo ""
echo -e "${BLUE}📀 Creating DMG...${NC}"

# Create temporary DMG directory
DMG_DIR="release/dmg_temp"
mkdir -p "$DMG_DIR"

# Copy the package and README to DMG
cp "release/$PKG_NAME" "$DMG_DIR/"
cp "$RELEASE_DIR/README.txt" "$DMG_DIR/"

# Create the DMG
hdiutil create -volname "Eskilator v${VERSION}" \
               -srcfolder "$DMG_DIR" \
               -ov \
               -format UDZO \
               "release/$DMG_NAME"

if [ $? -ne 0 ]; then
    echo -e "${RED}❌ Failed to create DMG${NC}"
    exit 1
fi

echo -e "${GREEN}✅ DMG created: release/$DMG_NAME${NC}"

# Clean up temp directory
rm -rf "$DMG_DIR"

echo ""
echo -e "${GREEN}🎉 Release build complete!${NC}"
echo ""
echo -e "${BLUE}📀 Release DMG: ${NC}release/$DMG_NAME"
echo -e "${BLUE}📦 Installer PKG: ${NC}release/$PKG_NAME"
echo ""

# Calculate DMG size
DMG_SIZE=$(du -h "release/$DMG_NAME" | cut -f1)
echo -e "${BLUE}DMG size: ${NC}$DMG_SIZE"

echo ""
echo -e "${YELLOW}Next steps for distribution:${NC}"
echo -e "1. Test the installer on a clean system"
echo -e "2. Notarize the package with Apple (recommended):"
echo -e "   xcrun notarytool submit release/$DMG_NAME --keychain-profile \"AC_PASSWORD\" --wait"
echo -e "3. Staple the notarization:"
echo -e "   xcrun stapler staple release/$DMG_NAME"
echo ""
