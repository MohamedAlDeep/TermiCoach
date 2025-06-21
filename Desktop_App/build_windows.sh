#!/bin/bash
# Build script for Windows executable
echo "Building TermiCoach for Windows..."

# Make sure we're in the right directory
cd "$(dirname "$0")"

# Clean any previous builds
make -f Makefile.win clean
rm -rf dist

# Create dist directory 
mkdir -p dist

# Build the application
make -f Makefile.win

# Check if the build was successful
if [ -f "termicoach.exe" ]; then
    echo "Build successful!"
    
    # Copy necessary files to dist folder
    cp termicoach.exe dist/
    cp sqlite3.dll dist/
    
    # Create a README.txt for Windows users
    cat > dist/README.txt << EOL
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

    echo "Distribution package created in the 'dist' folder."
    echo "To use on Windows, copy the entire 'dist' folder to the Windows machine."
else
    echo "Build failed!"
fi
