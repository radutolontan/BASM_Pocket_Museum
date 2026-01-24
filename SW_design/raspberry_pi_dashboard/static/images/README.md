# Pocket Lab Icon

## Required Image

Please place your Pocket Lab icon PNG image in this directory with the filename:

**`pocket-lab-icon.png`**

## Specifications

- **Format**: PNG with transparency (recommended)
- **Size**: Any size (will be displayed at 64x64 pixels)
- **Aspect ratio**: Square is recommended but not required
- **Usage**: This icon appears on device selection cards for each Pocket Lab

## Current Path

The dashboard looks for the image at:
```
/images/pocket-lab-icon.png
```

Which corresponds to:
```
raspberry_pi_dashboard/static/images/pocket-lab-icon.png
```

## If Image is Missing

If the image file is not found, browsers will show a broken image icon. To use the default Bootstrap icon instead, you can modify line 444 in `static/js/pocket-lab-live.js`:

```javascript
// Current (custom PNG):
<img src="/images/pocket-lab-icon.png" alt="Pocket Lab" style="width: 64px; height: 64px; object-fit: contain;" />

// Alternative (Bootstrap icon):
<i class="bi bi-cpu-fill"></i>
```
