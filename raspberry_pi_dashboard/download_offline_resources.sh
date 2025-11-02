#!/bin/bash
# Download offline resources for Raspberry Pi Dashboard
# Run this on a computer with internet access, then transfer files to Pi

set -e  # Exit on error

echo "===================================="
echo "Downloading Offline Resources"
echo "===================================="
echo ""

# Navigate to script directory
cd "$(dirname "$0")"

# Create directories
echo "Creating directories..."
mkdir -p static/js
mkdir -p static/css
mkdir -p static/fonts

# Download Chart.js
echo ""
echo "[1/4] Downloading Chart.js..."
curl -L -o static/js/chart.min.js \
    https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js
echo "✓ Chart.js downloaded ($(wc -c < static/js/chart.min.js) bytes)"

# Download Bootstrap Icons CSS
echo ""
echo "[2/4] Downloading Bootstrap Icons CSS..."
curl -L -o static/css/bootstrap-icons.min.css \
    https://cdn.jsdelivr.net/npm/bootstrap-icons@1.11.0/font/bootstrap-icons.min.css
echo "✓ Bootstrap Icons CSS downloaded ($(wc -c < static/css/bootstrap-icons.min.css) bytes)"

# Download Bootstrap Icons fonts
echo ""
echo "[3/4] Downloading Bootstrap Icons WOFF2 font..."
curl -L -o static/fonts/bootstrap-icons.woff2 \
    https://cdn.jsdelivr.net/npm/bootstrap-icons@1.11.0/font/fonts/bootstrap-icons.woff2
echo "✓ WOFF2 font downloaded ($(wc -c < static/fonts/bootstrap-icons.woff2) bytes)"

echo ""
echo "[4/5] Downloading Bootstrap Icons WOFF font..."
curl -L -o static/fonts/bootstrap-icons.woff \
    https://cdn.jsdelivr.net/npm/bootstrap-icons@1.11.0/font/fonts/bootstrap-icons.woff
echo "✓ WOFF font downloaded ($(wc -c < static/fonts/bootstrap-icons.woff) bytes)"

echo ""
echo "[5/5] Downloading Socket.IO client..."
curl -L -o static/js/socket.io.min.js \
    https://cdn.socket.io/4.5.4/socket.io.min.js
echo "✓ Socket.IO downloaded ($(wc -c < static/js/socket.io.min.js) bytes)"

# Fix Bootstrap Icons CSS font paths
echo ""
echo "Fixing font paths in Bootstrap Icons CSS..."
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    sed -i '' 's|url("\./fonts/|url("/fonts/|g' static/css/bootstrap-icons.min.css
    sed -i '' 's|url("fonts/|url("/fonts/|g' static/css/bootstrap-icons.min.css
else
    # Linux
    sed -i 's|url("\./fonts/|url("/fonts/|g' static/css/bootstrap-icons.min.css
    sed -i 's|url("fonts/|url("/fonts/|g' static/css/bootstrap-icons.min.css
fi
echo "✓ Font paths updated"

echo ""
echo "===================================="
echo "✓ All resources downloaded!"
echo "===================================="
echo ""
echo "Files created:"
echo "  - static/js/chart.min.js"
echo "  - static/js/socket.io.min.js"
echo "  - static/css/bootstrap-icons.min.css"
echo "  - static/fonts/bootstrap-icons.woff2"
echo "  - static/fonts/bootstrap-icons.woff"
echo ""
echo "Next steps:"
echo "  1. Transfer these files to your Raspberry Pi"
echo "  2. Restart the Flask server"
echo "  3. Test at http://192.168.10.2:8080/pocket-lab-mockup.html"
echo ""
