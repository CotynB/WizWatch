"""
Patches screens.c after EEZ Studio export.

Replaces embedded image references (&img_*) with SD card paths ("S:*.bin")
so images are loaded from SD card instead of being compiled into firmware.

Usage: python patch_screens.py
Run this after every EEZ Studio re-export.
"""

import re
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SCREENS_CPP = os.path.join(SCRIPT_DIR, "..", "ui", "WizWatch", "src", "ui", "screens.cpp")  # Now .cpp after rename
IMAGES_DIR = os.path.join(SCRIPT_DIR, "..", "ui", "WizWatch", "src", "ui", "images")


def discover_image_replacements():
    """Auto-discover all ui_image_*.cpp files and build replacement dict."""
    replacements = {}

    if not os.path.isdir(IMAGES_DIR):
        print(f"WARNING: Images directory not found: {IMAGES_DIR}")
        return replacements

    seen = set()
    for filename in os.listdir(IMAGES_DIR):
        if filename.startswith("ui_image_") and (filename.endswith(".cpp") or filename.endswith(".c")):
            # Extract name: ui_image_fond.cpp -> fond or ui_image_fond.c -> fond
            ext_len = 4 if filename.endswith(".cpp") else 2
            name = filename[9:-ext_len]  # Strip "ui_image_" prefix and extension
            if name not in seen:
                seen.add(name)
                replacements[f"&img_{name}"] = f'"S:{name}.bin"'

    return replacements


def patch_screens_cpp(image_replacements):
    """Patch screens.cpp to use SD card image paths."""
    with open(SCREENS_CPP, "r") as f:
        content = f.read()

    original = content

    # Replace image references with SD card paths
    for old, new in image_replacements.items():
        content = content.replace(old, new)

    # Remove images.h include since we're loading from SD
    content = content.replace('#include "images.h"\n', '')

    # Remove lv_image_set_scale() calls on SD-loaded images.
    # LVGL's BIN decoder in streaming mode (LV_BIN_DECODER_RAM_LOAD=0)
    # cannot render transformed/scaled images from files.
    content = re.sub(r'\n\s*lv_image_set_scale\(obj, \d+\);', '', content)

    if content != original:
        with open(SCREENS_CPP, "w") as f:
            f.write(content)
        return True
    return False


def main():
    print("Discovering images...")
    image_replacements = discover_image_replacements()
    print(f"  Found {len(image_replacements)} images:")
    for old, new in sorted(image_replacements.items()):
        print(f"    {old} -> {new}")

    print("\nPatching screens.cpp...")
    if patch_screens_cpp(image_replacements):
        print("  Patched successfully!")
    else:
        print("  No changes needed (already patched or no matches)")

    print("Done!")


if __name__ == "__main__":
    main()
