#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}🔨 Building and Installing Eskilator Plugin...${NC}"
echo ""

# Check if build directory exists and has CMake cache, create/configure if not
if [ ! -d "build" ] || [ ! -f "build/CMakeCache.txt" ]; then
    echo -e "${YELLOW}📁 Creating/configuring build directory...${NC}"
    mkdir -p build
    cd build
    cmake ..
    cd ..
fi

# Kill any existing standalone instances before building
echo -e "${YELLOW}🔄 Killing existing standalone instances...${NC}"
pkill -f "Eskilator" 2>/dev/null || true
sleep 1

# Build the plugin
echo -e "${BLUE}🔨 Building plugin...${NC}"

# Check if a specific target was passed
if [ "$1" = "standalone" ]; then
    echo -e "${YELLOW}🎯 Building standalone only (fast UI testing)...${NC}"
    cmake --build build --config Release --target Eskilator_Standalone
    BUILD_TARGET="standalone"
elif [ "$1" = "all" ] || [ -z "$1" ]; then
    echo -e "${YELLOW}🎯 Building all targets (AU, VST3, Standalone)...${NC}"
    cmake --build build --config Release
    BUILD_TARGET="all"
else
    echo -e "${YELLOW}🎯 Building custom target: $1...${NC}"
    cmake --build build --config Release --target "$1"
    BUILD_TARGET="$1"
fi

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✅ Build successful!${NC}"

    # If building standalone, open it and exit
    if [ "$BUILD_TARGET" = "standalone" ]; then
        echo ""
        echo -e "${BLUE}🚀 Opening standalone app for UI testing...${NC}"

        # Kill any existing instances
        pkill -f "Eskilator" 2>/dev/null
        sleep 1

        # Open the standalone app
        if [ -d "build/Eskilator_artefacts/Standalone/Eskilator.app" ]; then
            open "build/Eskilator_artefacts/Standalone/Eskilator.app"
            echo -e "${GREEN}✅ Standalone app opened!${NC}"
        else
            echo -e "${RED}❌ Standalone app not found${NC}"
        fi

        echo ""
        echo -e "${YELLOW}💡 Use './build_and_install.sh all' to build and install all plugins${NC}"
        exit 0
    fi
else
    echo -e "${RED}❌ Build failed!${NC}"
    exit 1
fi

echo ""
echo -e "${BLUE}🔐 Code signing plugins...${NC}"

# Function to code sign a plugin
code_sign_plugin() {
    local plugin_path="$1"
    local plugin_type="$2"
    
    if [ ! -d "$plugin_path" ]; then
        echo -e "${RED}❌ $plugin_type not found at: $plugin_path${NC}"
        return 1
    fi
    
    echo -e "${BLUE}   Code signing $plugin_type...${NC}"
    
    # Check if we have a valid certificate
    if security find-identity -v -p codesigning | grep -q "Developer ID Application"; then
        echo -e "${BLUE}     Using Developer ID Application certificate${NC}"
        codesign --force --deep --sign "Developer ID Application" "$plugin_path"
    elif security find-identity -v -p codesigning | grep -q "Mac Developer"; then
        echo -e "${BLUE}     Using Mac Developer certificate${NC}"
        codesign --force --deep --sign "Mac Developer" "$plugin_path"
    else
        echo -e "${YELLOW}     No valid code signing certificate found, using proper ad-hoc signing${NC}"
        # First sign the executable inside the bundle
        local executable_path=""
        if [[ "$plugin_type" == "AU Plugin" ]]; then
            executable_path="$plugin_path/Contents/MacOS/Eskilator"
        elif [[ "$plugin_type" == "VST3 Plugin" ]]; then
            executable_path="$plugin_path/Contents/MacOS/Eskilator"
        fi
        
        if [ -f "$executable_path" ]; then
            echo -e "${BLUE}       Signing executable: $executable_path${NC}"
            codesign --force --sign - --timestamp --options runtime "$executable_path"
        fi
        
        # Then sign the entire bundle
        codesign --force --deep --sign - --timestamp --options runtime "$plugin_path"
    fi
    
    # Verify the signature
    if codesign --verify --verbose "$plugin_path" 2>&1 | grep -q "valid on disk"; then
        echo -e "${GREEN}     ✅ $plugin_type code signed successfully${NC}"
    else
        echo -e "${RED}     ❌ $plugin_type code signing verification failed${NC}"
        return 1
    fi
}

# Find the actual plugin locations (could be in Debug or Release)
AU_PLUGIN=""
VST3_PLUGIN=""

# Check the actual CMake build output location first
if [ -d "build/Eskilator_artefacts/AU/Eskilator.component" ]; then
    AU_PLUGIN="build/Eskilator_artefacts/AU/Eskilator.component"
elif [ -d "build/Eskilator_artefacts/Release/AU/Eskilator.component" ]; then
    AU_PLUGIN="build/Eskilator_artefacts/Release/AU/Eskilator.component"
elif [ -d "build/Eskilator_artefacts/Debug/AU/Eskilator.component" ]; then
    AU_PLUGIN="build/Eskilator_artefacts/Debug/AU/Eskilator.component"
fi

if [ -d "build/Eskilator_artefacts/VST3/Eskilator.vst3" ]; then
    VST3_PLUGIN="build/Eskilator_artefacts/VST3/Eskilator.vst3"
elif [ -d "build/Eskilator_artefacts/Release/VST3/Eskilator.vst3" ]; then
    VST3_PLUGIN="build/Eskilator_artefacts/Release/VST3/Eskilator.vst3"
elif [ -d "build/Eskilator_artefacts/Debug/VST3/Eskilator.vst3" ]; then
    VST3_PLUGIN="build/Eskilator_artefacts/Debug/VST3/Eskilator.vst3"
fi

# Code sign AU plugin
if [ -n "$AU_PLUGIN" ]; then
    code_sign_plugin "$AU_PLUGIN" "AU Plugin"
fi

# Code sign VST3 plugin
if [ -n "$VST3_PLUGIN" ]; then
    code_sign_plugin "$VST3_PLUGIN" "VST3 Plugin"
fi

echo ""
echo -e "${BLUE}📁 Installing plugins...${NC}"

# Use user directories instead of system directories (no sudo required)
AU_INSTALL_DIR="$HOME/Library/Audio/Plug-Ins/Components"
VST3_INSTALL_DIR="$HOME/Library/Audio/Plug-Ins/VST3"

# Create plugins directories if they don't exist
mkdir -p "$AU_INSTALL_DIR"
mkdir -p "$VST3_INSTALL_DIR"

# Copy AU plugin
if [ -n "$AU_PLUGIN" ]; then
    echo -e "${BLUE}📋 Copying AU plugin...${NC}"
    cp -R "$AU_PLUGIN" "$AU_INSTALL_DIR/"
else
    echo -e "${RED}❌ AU plugin not found${NC}"
fi

# Copy VST3 plugin
if [ -n "$VST3_PLUGIN" ]; then
    echo -e "${BLUE}📋 Copying VST3 plugin...${NC}"
    cp -R "$VST3_PLUGIN" "$VST3_INSTALL_DIR/"
else
    echo -e "${RED}❌ VST3 plugin not found${NC}"
fi

# Set proper permissions (no sudo needed for user directories)
echo -e "${BLUE}🔐 Setting permissions...${NC}"
chmod -R 755 "$AU_INSTALL_DIR/Eskilator.component" 2>/dev/null || true
chmod -R 755 "$VST3_INSTALL_DIR/Eskilator.vst3" 2>/dev/null || true

echo ""
echo -e "${GREEN}✅ Installation complete!${NC}"
echo ""
echo -e "${YELLOW}💡 You may need to restart your DAW for the new plugins to appear${NC}"
echo ""
echo -e "${BLUE}📍 Plugins installed to:${NC}"
echo "   AU: $AU_INSTALL_DIR/Eskilator.component"
echo "   VST3: $VST3_INSTALL_DIR/Eskilator.vst3"
