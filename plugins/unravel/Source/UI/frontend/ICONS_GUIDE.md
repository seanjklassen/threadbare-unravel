# Icon Update Guide

This guide explains how to update icons in the Unravel plugin UI.

## Quick Start

1. **Find the icon you want to update** in `src/icons.js`
2. **Copy your new SVG path data** (the `d` attribute from your SVG)
3. **Replace the path data** in the corresponding icon function
4. **Update dimensions** if needed (width, height, viewBox)

## Icon Organization

Icons are organized in `src/icons.js` with clear labels:

### UI Control Icons
- `chevronIcon()` - Dropdown arrow in preset pill
- `logoIcon()` - Threadbare brand logo
- `freezeIcon()` - Infinity symbol for freeze/loop button
- `settingsIcon()` - Sliders icon (shown when settings closed)
- `closeIcon()` - X icon (shown when settings open)

### Parameter Icons (Settings Drawer)
- `decayIcon()` - Decay parameter
- `distanceIcon()` - ER Pre-Delay parameter
- `sizeIcon()` - Size parameter
- `toneIcon()` - Tone parameter
- `driftIcon()` - Drift parameter
- `ghostIcon()` - Ghost parameter
- `duckIcon()` - Duck parameter
- `blendIcon()` - Mix/Blend parameter
- `outputIcon()` - Output parameter

## How to Update an Icon

### Step 1: Get Your SVG Code

When you have a new SVG icon, you'll typically get something like:

```svg
<svg width="24" height="24" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
  <path d="M12 2L2 7L12 12L22 7L12 2Z" fill="currentColor"/>
</svg>
```

### Step 2: Extract the Path Data

Copy the `d` attribute value from the `<path>` element:
```
M12 2L2 7L12 12L22 7L12 2Z
```

### Step 3: Update the Icon Function

Open `src/icons.js` and find the icon you want to update. For example, to update the freeze icon:

```javascript
export function freezeIcon() {
  return createIcon(
    'M12 2L2 7L12 12L22 7L12 2Z',  // ← Replace this with your new path data
    20,                              // ← Update width if needed
    10,                              // ← Update height if needed
    '0 0 20 10',                     // ← Update viewBox if needed
    'icon-freeze'
  )
}
```

### Step 4: Update Dimensions (if needed)

If your new icon has different dimensions:
- Update the `width` and `height` parameters
- Update the `viewBox` to match your SVG's viewBox
- The viewBox format is: `"minX minY width height"`

### Step 5: Multiple Paths

If your SVG has multiple `<path>` elements, you can combine them:

```javascript
export function myIcon() {
  // For multiple paths, combine them in a single path string
  // or use multiple path elements (requires modifying createIcon)
  return createIcon(
    'M12 2L2 7L12 12L22 7L12 2Z M12 14L2 19L12 24L22 19L12 14Z',
    24,
    24,
    '0 0 24 24'
  )
}
```

## Icon Locations in HTML

Icons are currently embedded directly in `index.html`. After updating `icons.js`, you'll need to update the HTML:

### Main UI Icons (in `index.html`)

- **Chevron**: Lines 30-41 (inside `.preset-pill .chevron`)
- **Logo**: Lines 46-61 (inside `.logo-glyph`)
- **Freeze**: Lines 84-97 (inside `.btn-freeze`)
- **Settings**: Lines 100-113 (inside `.btn-settings .icon-settings`)
- **Close**: Lines 114-124 (inside `.btn-settings .icon-close`)

### Parameter Icons (in `index.html`)

- **Decay**: Lines 136-138
- **Distance**: Lines 153-155
- **Size**: Lines 170-172
- **Tone**: Lines 187-189
- **Drift**: Lines 204-206
- **Ghost**: Lines 221-223
- **Duck**: Lines 238-240
- **Blend**: Lines 255-257
- **Output**: Lines 272-274

## Best Practices

1. **Keep `fill="currentColor"`**: This allows icons to inherit text color from CSS
2. **Maintain viewBox**: Preserves icon proportions at any size
3. **Test at different sizes**: Icons scale via CSS, so ensure they look good at various sizes
4. **Consistent stroke width**: If using strokes, keep them consistent across icons
5. **Optimize paths**: Remove unnecessary path commands to reduce file size

## Example: Updating the Freeze Icon

Let's say you have a new infinity symbol SVG:

```svg
<svg viewBox="0 0 24 24" fill="none">
  <path d="M18.178 16.088L17.989 16.1915L17.8 16.088C16.717 15.345 15.5 14.5 15.5 12C15.5 9.5 16.717 8.655 17.8 7.912L17.989 7.8085L18.178 7.912C19.261 8.655 20.5 9.5 20.5 12C20.5 14.5 19.261 15.345 18.178 16.088ZM5.822 16.088C4.739 15.345 3.5 14.5 3.5 12C3.5 9.5 4.739 8.655 5.822 7.912L6.011 7.8085L6.2 7.912C7.283 8.655 8.5 9.5 8.5 12C8.5 14.5 7.283 15.345 6.2 16.088L6.011 16.1915L5.822 16.088Z" fill="currentColor"/>
</svg>
```

Update `freezeIcon()` in `icons.js`:

```javascript
export function freezeIcon() {
  return createIcon(
    'M18.178 16.088L17.989 16.1915L17.8 16.088C16.717 15.345 15.5 14.5 15.5 12C15.5 9.5 16.717 8.655 17.8 7.912L17.989 7.8085L18.178 7.912C19.261 8.655 20.5 9.5 20.5 12C20.5 14.5 19.261 15.345 18.178 16.088ZM5.822 16.088C4.739 15.345 3.5 14.5 3.5 12C3.5 9.5 4.739 8.655 5.822 7.912L6.011 7.8085L6.2 7.912C7.283 8.655 8.5 9.5 8.5 12C8.5 14.5 7.283 15.345 6.2 16.088L6.011 16.1915L5.822 16.088Z',
    20,
    10,
    '0 0 24 24',
    'icon-freeze'
  )
}
```

Then update the HTML at lines 84-97 with the new SVG code.

## Future: Dynamic Icon Loading

Currently, icons are embedded in HTML. A future enhancement could dynamically load icons from `icons.js` using JavaScript, eliminating the need to update HTML manually.
