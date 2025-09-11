#!/bin/bash
set -e

# Ensure xmake is in PATH
export PATH="/root/.local/bin:${PATH}"
export XMAKE_ROOT=y

# Get version from git or use default
VERSION=$(git describe --tags --long 2>/dev/null || echo "1.0-dev")
export ACCELERATORCSS_VERSION="$VERSION"
echo "Setting version to \"$ACCELERATORCSS_VERSION\""

# Configure & build native plugin
xmake f -y -p linux -a x86_64 --mode=debug
xmake -y

# Prepare folders
mkdir -p build/package/addons/metamod
mkdir -p build/package/addons/AcceleratorCSS
mkdir -p build/package/addons/AcceleratorCSS/bin/linuxsteamrt64
mkdir -p build/package/addons/counterstrikesharp/plugins
mkdir -p build/package/addons/counterstrikesharp/shared/0Harmony

# Copy configs
cp configs/addons/metamod/AcceleratorCSS.vdf build/package/addons/metamod
cp configs/addons/AcceleratorCSS/config.json build/package/addons/AcceleratorCSS
cp build/linux/x86_64/debug/libAcceleratorCSS.so \
   build/package/addons/AcceleratorCSS/bin/linuxsteamrt64/AcceleratorCSS.so
cp managed/0Harmony.dll \
   build/package/addons/counterstrikesharp/shared/0Harmony/0Harmony.dll

# Build managed
dotnet publish managed/AcceleratorCSS_CSS/AcceleratorCSS_CSS.csproj -c Release -o build/package/addons/counterstrikesharp/plugins/AcceleratorCSS_CSS