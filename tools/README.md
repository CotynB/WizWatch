# WizWatch UI Tools

Scripts for processing EEZ Studio generated files.

## Workflow After EEZ Studio Update

When you modify and re-export your EEZ Studio project:

### 1. Export from EEZ Studio
- Make your changes in EEZ Studio
- Build/Export the project (generates files in `ui/WizWatch/src/ui/`)

### 2. Run Update Script
```bash
cd tools
python update_ui.py
```

This automatically runs:
- ✓ `patch_cpp_compat.py` - Fixes C++ compilation issues
- ✓ `patch_screens.py` - Replaces embedded images with SD card paths

### 3. Compile & Upload
- Open Arduino IDE
- Compile and upload WizWatch.ino to your ESP32

### 4. Upload Images to SD Card
```bash
python upload_to_sd.py COM3
```
(Replace COM3 with your actual port)

---

## Individual Scripts

### `update_ui.py` ⭐ (Use this one!)
Master script that runs all patches in the correct order.

### `patch_cpp_compat.py`
Patches EEZ-generated files for C++ compatibility:
- Adds `<eez-framework.h>` includes
- Adds `extern "C"` blocks
- Adds C++ casts for LVGL functions

### `patch_screens.py`
Replaces embedded image references with SD card paths:
- `&img_fond` → `"S:fond.bin"`
- Removes `images.h` includes

### `upload_to_sd.py`
Uploads converted images to ESP32 SD card via serial.

---

## Quick Reference

**After every EEZ Studio export:**
```bash
python update_ui.py
```

**To upload images:**
```bash
python upload_to_sd.py COM3
```

That's it! ✨
