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
echo "[1/9] Downloading Chart.js..."
curl -L -o static/js/chart.min.js \
    https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js
echo "✓ Chart.js downloaded ($(wc -c < static/js/chart.min.js) bytes)"

# Download Bootstrap Icons CSS
echo ""
echo "[2/9] Downloading Bootstrap Icons CSS..."
curl -L -o static/css/bootstrap-icons.min.css \
    https://cdn.jsdelivr.net/npm/bootstrap-icons@1.11.0/font/bootstrap-icons.min.css
echo "✓ Bootstrap Icons CSS downloaded ($(wc -c < static/css/bootstrap-icons.min.css) bytes)"

# Download Bootstrap Icons fonts
echo ""
echo "[3/9] Downloading Bootstrap Icons WOFF2 font..."
curl -L -o static/fonts/bootstrap-icons.woff2 \
    https://cdn.jsdelivr.net/npm/bootstrap-icons@1.11.0/font/fonts/bootstrap-icons.woff2
echo "✓ WOFF2 font downloaded ($(wc -c < static/fonts/bootstrap-icons.woff2) bytes)"

echo ""
echo "[4/9] Downloading Bootstrap Icons WOFF font..."
curl -L -o static/fonts/bootstrap-icons.woff \
    https://cdn.jsdelivr.net/npm/bootstrap-icons@1.11.0/font/fonts/bootstrap-icons.woff
echo "✓ WOFF font downloaded ($(wc -c < static/fonts/bootstrap-icons.woff) bytes)"

echo ""
echo "[5/9] Downloading Socket.IO client..."
curl -L -o static/js/socket.io.min.js \
    https://cdn.socket.io/4.5.4/socket.io.min.js
echo "✓ Socket.IO downloaded ($(wc -c < static/js/socket.io.min.js) bytes)"

# Download Work Sans fonts (weights 300-800)
echo ""
echo "[6/9] Downloading Work Sans Regular (400)..."
curl -L -o static/fonts/WorkSans-Regular.woff2 \
    "https://fonts.gstatic.com/s/worksans/v18/QGY_z_wNahGAdqQ43RhVcIgYT2Xz5u32K0nWNigDp6_cOyA.woff2"
echo "✓ Work Sans Regular downloaded"

echo ""
echo "[7/9] Downloading Work Sans Medium (500)..."
curl -L -o static/fonts/WorkSans-Medium.woff2 \
    "https://fonts.gstatic.com/s/worksans/v18/QGY_z_wNahGAdqQ43RhVcIgYT2Xz5u32K3vWNigDp6_cOyA.woff2"
echo "✓ Work Sans Medium downloaded"

echo ""
echo "[8/9] Downloading Work Sans SemiBold (600)..."
curl -L -o static/fonts/WorkSans-SemiBold.woff2 \
    "https://fonts.gstatic.com/s/worksans/v18/QGY_z_wNahGAdqQ43RhVcIgYT2Xz5u32K5fQNigDp6_cOyA.woff2"
echo "✓ Work Sans SemiBold downloaded"

echo ""
echo "[9/9] Downloading Work Sans Bold (700)..."
curl -L -o static/fonts/WorkSans-Bold.woff2 \
    "https://fonts.gstatic.com/s/worksans/v18/QGY_z_wNahGAdqQ43RhVcIgYT2Xz5u32K67QNigDp6_cOyA.woff2"
echo "✓ Work Sans Bold downloaded"

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

# Create Work Sans CSS file
echo ""
echo "Creating Work Sans CSS file..."
cat > static/css/work-sans.css << 'EOF'
/* Work Sans Font - Local */
@font-face {
  font-family: 'Work Sans';
  font-style: normal;
  font-weight: 400;
  font-display: swap;
  src: url('/fonts/WorkSans-Regular.woff2') format('woff2');
}

@font-face {
  font-family: 'Work Sans';
  font-style: normal;
  font-weight: 500;
  font-display: swap;
  src: url('/fonts/WorkSans-Medium.woff2') format('woff2');
}

@font-face {
  font-family: 'Work Sans';
  font-style: normal;
  font-weight: 600;
  font-display: swap;
  src: url('/fonts/WorkSans-SemiBold.woff2') format('woff2');
}

@font-face {
  font-family: 'Work Sans';
  font-style: normal;
  font-weight: 700;
  font-display: swap;
  src: url('/fonts/WorkSans-Bold.woff2') format('woff2');
}
EOF
echo "✓ Work Sans CSS created"

echo ""
echo "===================================="
echo "✓ All resources downloaded!"
echo "===================================="
echo ""
echo "Files created:"
echo "  - static/js/chart.min.js"
echo "  - static/js/socket.io.min.js"
echo "  - static/css/bootstrap-icons.min.css"
echo "  - static/css/work-sans.css"
echo "  - static/fonts/bootstrap-icons.woff2"
echo "  - static/fonts/bootstrap-icons.woff"
echo "  - static/fonts/WorkSans-Regular.woff2"
echo "  - static/fonts/WorkSans-Medium.woff2"
echo "  - static/fonts/WorkSans-SemiBold.woff2"
echo "  - static/fonts/WorkSans-Bold.woff2"
echo ""
echo "Next steps:"
echo "  1. Transfer these files to your Raspberry Pi"
echo "  2. Restart the Flask server"
echo "  3. Test at http://192.168.10.2:8080/pocket-lab-live.html"
echo ""
