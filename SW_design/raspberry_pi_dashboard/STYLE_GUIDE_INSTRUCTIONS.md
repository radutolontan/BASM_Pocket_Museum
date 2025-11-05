# BASM Dashboard Style Guide - Viewing Instructions

## Quick Start

### Method 1: View Directly (Recommended for Development)

If you already have the Flask application running:

1. **Start the dashboard server:**
   ```bash
   cd /home/user/BASM_Pocket_Museum/raspberry_pi_dashboard
   python3 app.py
   ```

2. **Open your browser and navigate to:**
   ```
   http://localhost:8080/style-guide.html
   ```

3. **Done!** The style guide should now be visible.

---

### Method 2: View Without Running Server (Static HTML)

If you want to view the style guide without running the Flask server:

1. **Navigate to the static directory:**
   ```bash
   cd /home/user/BASM_Pocket_Museum/raspberry_pi_dashboard/static
   ```

2. **Start a simple HTTP server:**
   ```bash
   # Python 3
   python3 -m http.server 8000

   # Or using Python 2
   python -m SimpleHTTPServer 8000
   ```

3. **Open your browser and navigate to:**
   ```
   http://localhost:8000/style-guide.html
   ```

---

### Method 3: Open in Browser (May have limited functionality)

You can also open the file directly, though some features may not work:

1. **Navigate to the file:**
   ```bash
   /home/user/BASM_Pocket_Museum/raspberry_pi_dashboard/static/style-guide.html
   ```

2. **Open in your browser:**
   - Right-click → Open with → Your Browser
   - Or drag and drop into browser window

**Note:** Some external resources (fonts, icons) may not load properly with this method.

---

## What You'll See

The style guide includes the following sections:

### 1. **Color Palette**
   - **Brand Colors:** Your four accent colors
     - Sunshine Yellow (#f8c01c)
     - Ocean Blue (#375f83)
     - Rose Pink (#d782a0)
     - Cardinal Red (#bd2026)
   - **Light Theme Colors:** Background, text, borders
   - **Dark Theme Colors:** Alternative dark mode palette
   - **Interactive:** Click on any hex code to copy it to clipboard!

### 2. **Typography**
   - All heading levels (H1-H6) using Work Sans font
   - Body text variations (large, regular, small, caption)
   - Font weights, sizes, and line heights
   - Shows how text looks in both light and dark themes

### 3. **Text Widths**
   - Demonstrates different content widths:
     - Full Width (100%) - for tables and data displays
     - Wide (80ch) - for technical content
     - Reading Width (70ch) - optimal for articles
     - Comfortable (60ch) - for blog posts
     - Narrow (50ch) - for mobile layouts
   - Each has example text so you can see readability

### 4. **Cards & Border Radius**
   - Six border radius options:
     - None (0px) - sharp corners
     - Small (4px) - subtle rounding
     - Medium (8px) - balanced
     - Large (12px) - soft appearance
     - X-Large (16px) - prominent
     - XX-Large (24px) - bold
   - Card examples using all four brand colors
   - Hover effects and shadows

### 5. **Buttons**
   - **Primary Buttons:** Solid color buttons for each brand color
   - **Outline Buttons:** Transparent with colored borders
   - **Button Sizes:** Small, Medium, Large, Extra Large
   - **Button States:** Normal, Disabled, With Icons
   - All buttons have hover and active states

### 6. **Form Elements**
   - Text inputs
   - Select dropdowns
   - Text areas
   - Checkboxes (for sensor selection)
   - Radio buttons (for display type)
   - All styled consistently with focus states

### 7. **Component Examples**
   - **Device Card:** How ESP32 devices will appear
   - **Question Card:** Student notebook question layout
   - **Sensor Display Card:** Live sensor readings with charts
   - These show real dashboard components in action

---

## Using the Style Guide

### Theme Switcher

Click the **theme toggle button** in the top-right corner to switch between:
- **Light Theme** (default) - White background, dark text
- **Dark Theme** - Dark background, light text

Your preference is saved in browser localStorage and will persist between visits.

### Navigation

- Use the **Table of Contents** at the top to jump to specific sections
- Smooth scrolling is enabled for easy navigation
- All sections are clearly labeled and organized

### Interactive Features

1. **Copy Hex Codes:**
   - Click on any color hex code to copy it to your clipboard
   - The text will briefly change to "Copied!" to confirm

2. **Hover Effects:**
   - Hover over buttons, cards, and interactive elements to see animations
   - All components have smooth transitions

3. **Responsive:**
   - Resize your browser window to see how components adapt
   - Works on desktop, tablet, and mobile sizes

---

## Customization Guide

### Changing Colors

To modify the brand colors, edit `/static/css/style-guide.css`:

```css
:root {
    /* Brand Colors */
    --color-yellow: #f8c01c;  /* Change to your yellow */
    --color-blue: #375f83;    /* Change to your blue */
    --color-pink: #d782a0;    /* Change to your pink */
    --color-red: #bd2026;     /* Change to your red */
}
```

### Changing Border Radius

All border radius values are defined as CSS variables:

```css
:root {
    --radius-none: 0;
    --radius-sm: 4px;     /* Increase for rounder corners */
    --radius-md: 8px;     /* Default for most cards */
    --radius-lg: 12px;    /* For larger components */
    --radius-xl: 16px;    /* Extra rounded */
    --radius-2xl: 24px;   /* Very rounded */
}
```

### Changing Font Sizes

Typography sizes are also variables:

```css
h1 { font-size: 3rem; }      /* 48px */
h2 { font-size: 2.25rem; }   /* 36px */
h3 { font-size: 1.75rem; }   /* 28px */
/* etc. */
```

### Adding New Components

To add a new component example:

1. **Add HTML in** `style-guide.html`:
   ```html
   <div class="your-component-demo">
       <!-- Your component markup -->
   </div>
   ```

2. **Add CSS in** `style-guide.css`:
   ```css
   .your-component-demo {
       /* Your styles */
   }
   ```

3. **Refresh browser** to see changes

---

## Applying Styles to the Dashboard

Once you've reviewed and approved the styles:

1. **Copy CSS variables** from `style-guide.css` to `styles.css`
2. **Use the color variables** in your dashboard components:
   ```css
   .device-card {
       background: var(--bg-primary);
       border: 2px solid var(--color-blue);
       border-radius: var(--radius-md);
   }
   ```
3. **Apply button classes** directly:
   ```html
   <button class="btn btn-primary-yellow">Submit</button>
   ```

---

## Browser Compatibility

The style guide works best in modern browsers:
- ✅ Chrome/Edge (v90+)
- ✅ Firefox (v88+)
- ✅ Safari (v14+)
- ✅ Opera (v76+)

Features used:
- CSS Custom Properties (CSS Variables)
- CSS Grid
- Flexbox
- Modern font loading
- LocalStorage for theme persistence

---

## Troubleshooting

### Fonts Not Loading

If Work Sans font doesn't appear:

1. **Check internet connection** (font loaded from Google Fonts)
2. **Verify the font link** in `style-guide.html`:
   ```html
   <link href="https://fonts.googleapis.com/css2?family=Work+Sans:wght@300;400;500;600;700;800&display=swap" rel="stylesheet">
   ```
3. **Check browser console** for errors (F12 → Console)

### Icons Not Showing

If Bootstrap Icons don't appear:

1. **Check the icon link** in `style-guide.html`:
   ```html
   <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bootstrap-icons@1.11.0/font/bootstrap-icons.css">
   ```
2. **Verify internet connection**

### Theme Not Switching

If the theme toggle doesn't work:

1. **Check browser console** for JavaScript errors
2. **Verify** `style-guide.js` is loaded:
   ```html
   <script src="/js/style-guide.js"></script>
   ```
3. **Clear browser cache** and reload

### Styles Look Wrong

If the styling appears broken:

1. **Verify CSS file is loaded:**
   ```html
   <link rel="stylesheet" href="/css/style-guide.css">
   ```
2. **Check browser console** for 404 errors
3. **Clear browser cache** (Ctrl+Shift+R or Cmd+Shift+R)

---

## Next Steps After Review

Once you've reviewed the style guide:

1. **Provide feedback:**
   - Which border radius do you prefer? (recommend: medium 8px or large 12px)
   - Which text width feels best for reading?
   - Do the brand colors look correct?
   - Any adjustments needed for light/dark themes?

2. **Apply to dashboard:**
   - Update `styles.css` with approved variables
   - Implement components using style guide examples
   - Ensure consistency across all tabs

3. **Test on devices:**
   - Desktop browsers
   - Tablets
   - Mobile phones
   - Raspberry Pi display (if applicable)

---

## Contact

If you encounter any issues or have questions:
- Check the troubleshooting section above
- Review browser console for errors (F12 → Console)
- Take a screenshot of any issues for easier debugging

---

## File Locations

For reference, the style guide files are located at:

```
raspberry_pi_dashboard/
├── static/
│   ├── style-guide.html           # Main style guide page
│   ├── css/
│   │   └── style-guide.css        # Style guide styles
│   └── js/
│       └── style-guide.js         # Theme switcher logic
└── STYLE_GUIDE_INSTRUCTIONS.md    # This file
```

---

**Enjoy exploring the BASM Dashboard Style Guide!** 🎨
