#!/bin/bash

# Script to prepare dependencies for x86Emulator
# Run this script before building the project

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
EXTERNAL_DIR="$PROJECT_DIR/external"

# Colors for console output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to clone or update a Git repository
clone_or_update_repo() {
    local repo_url="$1"
    local target_dir="$2"
    local branch="$3"
    
    if [ -d "$target_dir/.git" ]; then
        echo -e "${BLUE}Updating $target_dir...${NC}"
        cd "$target_dir"
        git fetch
        git checkout "$branch"
        git pull
        cd - > /dev/null
    else
        echo -e "${BLUE}Cloning $repo_url to $target_dir...${NC}"
        mkdir -p "$(dirname "$target_dir")"
        git clone --depth 1 --branch "$branch" "$repo_url" "$target_dir"
    fi
}

# Function to patch a repository
apply_patch() {
    local target_dir="$1"
    local patch_file="$2"
    
    if [ -f "$patch_file" ]; then
        echo -e "${BLUE}Applying patch $patch_file to $target_dir...${NC}"
        cd "$target_dir"
        git apply --check "$patch_file" 2>/dev/null || echo -e "${YELLOW}Patch already applied or conflicts. Skipping.${NC}"
        git apply "$patch_file" 2>/dev/null || true
        cd - > /dev/null
    else
        echo -e "${YELLOW}Patch file $patch_file not found. Skipping.${NC}"
    fi
}

# Create external directory if it doesn't exist
mkdir -p "$EXTERNAL_DIR"

# Prepare 86Box
echo -e "${GREEN}Preparing 86Box...${NC}"
clone_or_update_repo "https://github.com/86Box/86Box.git" "$EXTERNAL_DIR/86box" "master"
apply_patch "$EXTERNAL_DIR/86box" "$SCRIPT_DIR/patches/86box_integration.patch"

# Prepare MAME CPU components
echo -e "${GREEN}Preparing MAME CPU components...${NC}"
clone_or_update_repo "https://github.com/mamedev/mame.git" "$EXTERNAL_DIR/mame" "master"
apply_patch "$EXTERNAL_DIR/mame" "$SCRIPT_DIR/patches/mame_integration.patch"

# Remove WinUAE preparation
# echo -e "${GREEN}Preparing WinUAE components...${NC}"
# clone_or_update_repo "https://github.com/tonioni/WinUAE.git" "$EXTERNAL_DIR/winuae" "master"
# apply_patch "$EXTERNAL_DIR/winuae" "$SCRIPT_DIR/patches/winuae_integration.patch"

echo -e "${GREEN}All dependencies prepared successfully!${NC}"