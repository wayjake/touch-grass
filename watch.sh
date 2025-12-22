#!/bin/bash
# Watch for code changes and rebuild

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_CMD="arduino-cli compile --fqbn esp32:esp32:esp32s3 --output-dir $PROJECT_DIR/build $PROJECT_DIR/touch-grass.ino"

echo "Watching for changes in $PROJECT_DIR..."
echo "Press Ctrl+C to stop"

# Initial build
echo "Running initial build..."
$BUILD_CMD

# Watch for changes
inotifywait -m -r -e modify,create,delete,moved_to,close_write --include '\.(ino|h|cpp|c)$' "$PROJECT_DIR" 2>/dev/null | while read -r directory event filename; do
    echo ""
    echo "Change detected: $filename"
    echo "Rebuilding..."
    $BUILD_CMD
done
