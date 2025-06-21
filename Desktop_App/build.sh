#!/bin/bash
# Unified build script for TermiCoach
# Builds both Linux native binary and Windows .exe

# Set text colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Make sure we're in the right directory
cd "$(dirname "$0")"

# Display header
echo -e "${BLUE}============================================${NC}"
echo -e "${GREEN}TermiCoach Unified Build Script${NC}"
echo -e "${BLUE}============================================${NC}"

# Function to check if a command is available
check_command() {
    if ! command -v $1 &> /dev/null; then
        echo -e "${RED}Error: $1 is not installed. Please install it to continue.${NC}"
        return 1
    fi
    return 0
}

# Build for Linux
build_linux() {
    echo -e "${BLUE}Building TermiCoach for Linux...${NC}"
    
    # Check for required commands
    check_command g++ || return 1
    check_command make || return 1
    
    # Clean any previous builds
    make clean
    
    # Build the application
    make
    
    # Check if the build was successful
    if [ -f "termicoach" ]; then
        echo -e "${GREEN}Linux build successful! The executable is 'termicoach'${NC}"
        return 0
    else
        echo -e "${RED}Linux build failed!${NC}"
        return 1
    fi
}

# Build for Windows
build_windows() {
    echo -e "${BLUE}Building TermiCoach for Windows...${NC}"
    
    # Check for required commands
    check_command x86_64-w64-mingw32-g++ || { 
        echo -e "${RED}Error: MinGW cross-compiler (x86_64-w64-mingw32-g++) not found.${NC}"
        echo -e "${RED}Install it with: sudo apt-get install mingw-w64${NC}"
        return 1
    }
    
    # Clean previous windows builds
    make -f Makefile.win clean
    
    # Remove previous dist directory
    rm -rf dist-win
    
    # Create dist directory
    mkdir -p dist-win
    
    # Build the application
    make -f Makefile.win
    
    # Check if the build was successful
    if [ -f "termicoach.exe" ]; then
        # Copy necessary files to dist folder
        cp termicoach.exe dist-win/
        cp sqlite3.dll dist-win/
        
        # Create a README.txt for Windows users
        cat > dist-win/README.txt << EOL
TermiCoach for Windows
======================

This is the Windows version of TermiCoach, a terminal-based workout coaching application.

Files included:
- termicoach.exe - The main application
- sqlite3.dll - Required SQL database library

To run the application:
1. Double-click on termicoach.exe
2. Make sure both files remain in the same folder

Note: This is a console application that will run in a command prompt window.
EOL

        echo -e "${GREEN}Windows build successful! Distribution package created in 'dist-win' folder.${NC}"
        return 0
    else
        echo -e "${RED}Windows build failed!${NC}"
        return 1
    fi
}

# Main build process
linux_build_status=0
windows_build_status=0

# Process command-line arguments
if [ "$1" = "linux" ]; then
    build_linux
    exit $?
elif [ "$1" = "windows" ]; then
    build_windows
    exit $?
elif [ "$1" = "help" ] || [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    echo "Usage: $0 [option]"
    echo "Options:"
    echo "  linux    - Build only Linux binary"
    echo "  windows  - Build only Windows binary"
    echo "  (none)   - Build both Linux and Windows binaries"
    echo "  help     - Show this help message"
    exit 0
else
    # Build both
    echo -e "${BLUE}Building binaries for both Linux and Windows...${NC}"
    echo ""
    
    # First Linux
    build_linux
    linux_build_status=$?
    echo ""
    
    # Then Windows
    build_windows
    windows_build_status=$?
    echo ""
    
    # Summary
    echo -e "${BLUE}============================================${NC}"
    echo -e "${GREEN}Build Summary:${NC}"
    if [ $linux_build_status -eq 0 ]; then
        echo -e "Linux build:   ${GREEN}SUCCESS${NC}"
    else
        echo -e "Linux build:   ${RED}FAILED${NC}"
    fi
    
    if [ $windows_build_status -eq 0 ]; then
        echo -e "Windows build: ${GREEN}SUCCESS${NC}"
    else
        echo -e "Windows build: ${RED}FAILED${NC}"
    fi
    echo -e "${BLUE}============================================${NC}"
    
    # Overall status
    if [ $linux_build_status -eq 0 ] && [ $windows_build_status -eq 0 ]; then
        echo -e "${GREEN}All builds completed successfully!${NC}"
        exit 0
    else
        echo -e "${RED}Some builds failed. Please check the logs above.${NC}"
        exit 1
    fi
fi
