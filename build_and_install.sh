#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}🔨 Building and Installing VST Plugin...${NC}"
echo ""

# Check if build directory exists, create if not
if [ ! -d "build" ]; then
    echo -e "${YELLOW}📁 Creating build directory...${NC}"
    mkdir build
    cd build
    cmake ..
    cd ..
fi

# Build the plugin
echo -e "${BLUE}🔨 Building plugin...${NC}"
cmake --build build --target HelloWorldVST3_AU

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✅ Build successful!${NC}"
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
        echo -e "${YELLOW}     No valid code signing certificate found, using ad-hoc signing${NC}"
        codesign --force --deep --sign - "$plugin_path"
    fi
    
    # Verify the signature
    if codesign --verify --verbose "$plugin_path" 2>&1 | grep -q "valid on disk"; then
        echo -e "${GREEN}     ✅ $plugin_type code signed successfully${NC}"
    else
        echo -e "${RED}     ❌ $plugin_type code signing verification failed${NC}"
        return 1
    fi
}

# Code sign AU plugin
code_sign_plugin "build/HelloWorldVST3_artefacts/AU/Beat Generator v2.component" "AU Plugin"

# Code sign VST3 plugin
code_sign_plugin "build/HelloWorldVST3_artefacts/VST3/Beat Generator v2.vst3" "VST3 Plugin"

echo ""
echo -e "${BLUE}📁 Installing plugins...${NC}"

# Create plugins directories if they don't exist
sudo mkdir -p "/Library/Audio/Plug-Ins/Components"
sudo mkdir -p "/Library/Audio/Plug-Ins/VST3"

# Copy AU plugin
echo -e "${BLUE}📋 Copying AU plugin...${NC}"
sudo cp -R "build/HelloWorldVST3_artefacts/AU/Beat Generator v2.component" "/Library/Audio/Plug-Ins/Components/"

# Copy VST3 plugin
echo -e "${BLUE}📋 Copying VST3 plugin...${NC}"
sudo cp -R "build/HelloWorldVST3_artefacts/VST3/Beat Generator v2.vst3" "/Library/Audio/Plug-Ins/VST3/"

# Set proper permissions
echo -e "${BLUE}🔐 Setting permissions...${NC}"
sudo chmod -R 755 "/Library/Audio/Plug-Ins/Components/Beat Generator v2.component"
sudo chmod -R 755 "/Library/Audio/Plug-Ins/VST3/Beat Generator v2.vst3"

echo ""
echo -e "${GREEN}✅ Installation complete!${NC}"
echo ""
echo -e "${YELLOW}💡 You may need to restart your DAW for the new plugins to appear${NC}"
echo ""
echo -e "${BLUE}📍 Plugins installed to:${NC}"
echo "   AU: /Library/Audio/Plug-Ins/Components/Beat Generator v2.component"
echo "   VST3: /Library/Audio/Plug-Ins/VST3/Beat Generator v2.vst3"
