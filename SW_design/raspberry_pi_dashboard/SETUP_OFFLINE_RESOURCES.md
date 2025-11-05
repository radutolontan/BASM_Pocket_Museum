# Setup Offline Resources for Raspberry Pi Dashboard

The Raspberry Pi dashboard requires Chart.js and Bootstrap Icons to function properly. Since the Pi may not have internet access when running as an access point, you need to download these files locally.

## Quick Setup (On a computer with internet access)

### Option 1: Download Script (Linux/Mac)

Run this script on a computer with internet access, then transfer the files to your Raspberry Pi:

```bash
#!/bin/bash
cd raspberry_pi_dashboard/static

# Create directories if they don't exist
mkdir -p js
mkdir -p css
mkdir -p fonts

# Download Chart.js
echo "Downloading Chart.js..."
curl -o js/chart.min.js https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js

# Download Socket.IO
echo "Downloading Socket.IO..."
curl -o js/socket.io.min.js https://cdn.socket.io/4.5.4/socket.io.min.js

# Download Bootstrap Icons CSS
echo "Downloading Bootstrap Icons CSS..."
curl -o css/bootstrap-icons.min.css https://cdn.jsdelivr.net/npm/bootstrap-icons@1.11.0/font/bootstrap-icons.min.css

# Download Bootstrap Icons fonts
echo "Downloading Bootstrap Icons fonts..."
curl -o fonts/bootstrap-icons.woff https://cdn.jsdelivr.net/npm/bootstrap-icons@1.11.0/font/fonts/bootstrap-icons.woff
curl -o fonts/bootstrap-icons.woff2 https://cdn.jsdelivr.net/npm/bootstrap-icons@1.11.0/font/fonts/bootstrap-icons.woff2

echo "Done! Now transfer the 'static' folder to your Raspberry Pi."
```

### Option 2: Manual Download (Windows/any browser)

1. **Download Chart.js:**
   - Visit: https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js
   - Right-click → Save As → `chart.min.js`
   - Place in: `raspberry_pi_dashboard/static/js/chart.min.js`

2. **Download Socket.IO:**
   - Visit: https://cdn.socket.io/4.5.4/socket.io.min.js
   - Right-click → Save As → `socket.io.min.js`
   - Place in: `raspberry_pi_dashboard/static/js/socket.io.min.js`

3. **Download Bootstrap Icons CSS:**
   - Visit: https://cdn.jsdelivr.net/npm/bootstrap-icons@1.11.0/font/bootstrap-icons.min.css
   - Right-click → Save As → `bootstrap-icons.min.css`
   - Place in: `raspberry_pi_dashboard/static/css/bootstrap-icons.min.css`

4. **Download Bootstrap Icons Fonts:**
   - Visit: https://cdn.jsdelivr.net/npm/bootstrap-icons@1.11.0/font/fonts/bootstrap-icons.woff2
   - Right-click → Save As → `bootstrap-icons.woff2`
   - Place in: `raspberry_pi_dashboard/static/fonts/bootstrap-icons.woff2`

   - Visit: https://cdn.jsdelivr.net/npm/bootstrap-icons@1.11.0/font/fonts/bootstrap-icons.woff
   - Right-click → Save As → `bootstrap-icons.woff`
   - Place in: `raspberry_pi_dashboard/static/fonts/bootstrap-icons.woff`

4. **Update Bootstrap Icons CSS:**
   - Open `raspberry_pi_dashboard/static/css/bootstrap-icons.min.css`
   - Find the `@font-face` section
   - Change the font URL from:
     ```css
     url("./fonts/bootstrap-icons.woff2?...")
     ```
     to:
     ```css
     url("/fonts/bootstrap-icons.woff2")
     ```

### Option 3: Transfer from this repository (if you have the files locally)

If you've already cloned this repository on a machine with internet access and want to download the files there:

```bash
# On your local machine (not the Pi)
cd /path/to/BASM_Pocket_Museum
bash raspberry_pi_dashboard/download_offline_resources.sh

# Then sync to Raspberry Pi
rsync -avz raspberry_pi_dashboard/static/ pi@192.168.10.2:/home/pi/BASM_Pocket_Museum/raspberry_pi_dashboard/static/
```

## Verification

After transferring the files, verify they exist on the Raspberry Pi:

```bash
ls -lh raspberry_pi_dashboard/static/js/chart.min.js
ls -lh raspberry_pi_dashboard/static/js/socket.io.min.js
ls -lh raspberry_pi_dashboard/static/css/bootstrap-icons.min.css
ls -lh raspberry_pi_dashboard/static/fonts/bootstrap-icons.woff*
```

Expected output:
```
-rw-r--r-- 1 pi pi 261K Nov 2 14:00 raspberry_pi_dashboard/static/js/chart.min.js
-rw-r--r-- 1 pi pi  59K Nov 2 14:00 raspberry_pi_dashboard/static/js/socket.io.min.js
-rw-r--r-- 1 pi pi  72K Nov 2 14:00 raspberry_pi_dashboard/static/css/bootstrap-icons.min.css
-rw-r--r-- 1 pi pi 124K Nov 2 14:00 raspberry_pi_dashboard/static/fonts/bootstrap-icons.woff2
-rw-r--r-- 1 pi pi 165K Nov 2 14:00 raspberry_pi_dashboard/static/fonts/bootstrap-icons.woff
```

## Testing

1. Restart the Flask server (if running)
2. Navigate to: http://192.168.10.2:8080/pocket-lab-mockup.html
3. Check the debug console (bottom of page) for:
   - ✅ "Chart.js loaded successfully" (green)
   - ✅ "Pocket Lab Mockup loaded successfully" (green)
4. The page should load in under 2 seconds

## Troubleshooting

**Problem:** Still getting "Chart.js not loaded" error

**Solution:**
- Verify the file exists at the correct path
- Check Flask is serving static files correctly
- Open browser console (F12) and check for 404 errors
- Try accessing directly: http://192.168.10.2:8080/js/chart.min.js

**Problem:** Icons not showing (X symbols instead)

**Solution:**
- Check Bootstrap Icons CSS and fonts are downloaded
- Verify font paths in the CSS file point to `/fonts/` (absolute path)
- Clear browser cache
