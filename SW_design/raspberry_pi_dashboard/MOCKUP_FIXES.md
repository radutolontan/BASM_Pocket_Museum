# Mockup Fixes Summary

## Issues to Fix

### 1. Custom SVG Icons
Replace Bootstrap icons with custom SVG icons in yellow squares:

```javascript
const MEASUREMENT_ICONS = {
    temperature: `<svg viewBox="0 0 24 24" fill="currentColor"><path d="M17 4v8a5 5 0 11-10 0V4a3 3 0 116 0zm-5 14a3 3 0 003-3h-6a3 3 0 003 3z"/></svg>`,
    pressure: `<svg viewBox="0 0 24 24" fill="currentColor"><circle cx="12" cy="12" r="9" stroke="currentColor" stroke-width="2" fill="none"/><path d="M12 6v6l4 4"/></svg>`,
    acceleration: `<svg viewBox="0 0 24 24" fill="currentColor"><path d="M4 12h16M4 12l4-4M4 12l4 4M20 12l-4-4M20 12l-4 4"/></svg>`,
    // ... etc
};
```

### 2. Color Definitions
Add color for each measurement type:

```javascript
const MEASUREMENT_COLORS = {
    temperature: '#bd2026',      // RED
    pressure: '#375f83',         // BLUE
    acceleration: '#10b981',     // GREEN (new)
    gyro: '#f97316',            // ORANGE (new)
    magnetometer: '#f8c01c',    // YELLOW
    volume: '#d782a0',          // PINK/MAGENTA
    ambientLight: '#ffffff',    // WHITE
    spectrum: '#8b5cf6'         // PURPLE (for spectrum)
};
```

### 3. Debug Logging
Add debugLog() calls throughout:

```javascript
function initializeChart(displayId, measurementKey, measurement) {
    debugLog(`Initializing chart for ${displayId}...`);

    const canvas = document.getElementById(`${displayId}-chart`);
    if (!canvas) {
        debugLog(`ERROR: Canvas element not found: ${displayId}-chart`, 'error');
        return;
    }

    debugLog(`Canvas found, creating Chart.js instance...`);
    // ... rest of code
}
```

### 4. Chart Initialization Fix
Ensure charts initialize properly:

```javascript
// Wait for canvas to be in DOM before initializing
setTimeout(() => {
    initializeChart(displayId, measurementKey, measurement);
}, 200); // Increased from 100ms
```

## Test Instructions

1. **Reload page** - Check debug console at bottom
2. **Look for**:
   - "Chart.js loaded successfully" (green)
   - "Initializing chart for display-xxx..."
   - Any RED error messages
3. **Open Browser DevTools** (F12) → Console tab
4. **Take screenshot** of both consoles and send to me

## Debugging Steps

If charts still don't show:
1. Check browser console for errors
2. Check if Chart.js CDN is accessible
3. Verify canvas elements are being created
4. Check CSS display properties

## Files to Update

- `static/js/pocket-lab-mockup.js` - Add icons, colors, debug logging
- `static/css/pocket-lab-mockup.css` - Add color classes for measurement cards
