"""
Patches ui.cpp after EEZ Studio re-export.

Reads screens.c to extract ALL lv_* function calls for each object,
then updates ui.cpp with those values and replaces image references
with SD card file paths.

Usage: python patch_ui.py
Run this after every EEZ Studio re-export.
"""

import re
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SCREENS_C = os.path.join(SCRIPT_DIR, "..", "ui", "WizWatch", "src", "ui", "screens.c")
UI_CPP = os.path.join(SCRIPT_DIR, "..", "ui.cpp")
IMAGES_DIR = os.path.join(SCRIPT_DIR, "..", "ui", "WizWatch", "src", "ui", "images")


def discover_image_replacements():
    """Auto-discover all ui_image_*.c files and build replacement dict."""
    replacements = {}

    if not os.path.isdir(IMAGES_DIR):
        print(f"WARNING: Images directory not found: {IMAGES_DIR}")
        return replacements

    for filename in os.listdir(IMAGES_DIR):
        if filename.startswith("ui_image_") and filename.endswith(".c"):
            # Extract name: ui_image_fond.c -> fond
            name = filename[9:-2]  # Strip "ui_image_" prefix and ".c" suffix
            replacements[f"&img_{name}"] = f'"S:{name}.bin"'

    return replacements


def extract_lv_calls(block):
    """Extract all lv_* function calls on 'obj' from a code block."""
    calls = {}
    # Match lines like: lv_something(obj, ...);
    for match in re.finditer(r'(lv_\w+)\(obj,\s*(.*?)\);', block):
        func_name = match.group(1)
        args = match.group(2).strip()
        calls[func_name] = args
    return calls


def discover_objects(content):
    """Auto-discover all object names from the object_names[] array in screens.c."""
    match = re.search(r'object_names\[\]\s*=\s*\{([^}]+)\}', content)
    if not match:
        return []

    # Extract quoted strings: "main", "fond", etc.
    names = re.findall(r'"([^"]+)"', match.group(1))

    # Filter out screen names (they appear in screen_names[] too)
    screen_match = re.search(r'screen_names\[\]\s*=\s*\{([^}]+)\}', content)
    screen_names = []
    if screen_match:
        screen_names = [s.lower() for s in re.findall(r'"([^"]+)"', screen_match.group(1))]

    # Return objects that aren't screens
    return [n for n in names if n.lower() not in screen_names]


def discover_screens(content):
    """Auto-discover all screen names from screen_names[] array."""
    match = re.search(r'screen_names\[\]\s*=\s*\{([^}]+)\}', content)
    if not match:
        return []
    return [s.lower() for s in re.findall(r'"([^"]+)"', match.group(1))]


def parse_screens_c():
    """Parse screens.c to extract all lv_* calls per object."""
    with open(SCREENS_C, "r") as f:
        content = f.read()

    props = {}

    # Auto-discover objects
    objects = discover_objects(content)
    screens = discover_screens(content)

    print(f"  Found {len(screens)} screens: {', '.join(screens)}")
    print(f"  Found {len(objects)} objects: {', '.join(objects)}")

    for obj_name in objects:
        pattern = rf'objects\.{re.escape(obj_name)}\s*=\s*obj;(.*?)(?=objects\.\w+\s*=|\n\s*\}})'
        match = re.search(pattern, content, re.DOTALL)
        if not match:
            continue
        block = match.group(1)
        props[obj_name] = extract_lv_calls(block)

    # Extract screen-level properties for each screen
    for screen_name in screens:
        screen_match = re.search(
            rf'objects\.{re.escape(screen_name)}\s*=\s*obj;(.*?)(?=\{{\s*lv_obj_t \*parent_obj)',
            content, re.DOTALL
        )
        if screen_match:
            props[f'_screen_{screen_name}'] = extract_lv_calls(screen_match.group(1))

    return props


def patch_ui_cpp(props, image_replacements):
    """Update ui.cpp with extracted properties."""
    with open(UI_CPP, "r") as f:
        content = f.read()

    # Update screen-level calls for all screens
    for key, calls in props.items():
        if key.startswith('_screen_'):
            for func, args in calls.items():
                pattern = rf'{re.escape(func)}\(scr,\s*.*?\)'
                replacement = f'{func}(scr, {args})'
                content = re.sub(pattern, replacement, content)

    for obj_name, calls in props.items():
        if obj_name.startswith('_'):
            continue

        # Find the block for this object in ui.cpp
        block_pattern = rf'(ui_objects\.{re.escape(obj_name)}\s*=\s*obj;.*?)(?=\n  \}})'
        match = re.search(block_pattern, content, re.DOTALL)
        if not match:
            print(f"  WARNING: Could not find block for {obj_name} in ui.cpp")
            continue

        block = match.group(1)
        new_block = block

        for func, args in calls.items():
            # Skip lv_image_set_src — we handle that separately with SD paths
            if func == 'lv_image_set_src':
                continue

            pattern = rf'{re.escape(func)}\(obj,\s*.*?\)'
            replacement = f'{func}(obj, {args})'

            if re.search(pattern, new_block):
                # Update existing call
                new_block = re.sub(pattern, replacement, new_block)
            else:
                # Add new call before the closing of the block
                # Insert before the last line
                new_block = new_block.rstrip()
                new_block += f'\n    {func}(obj, {args});'

        content = content.replace(block, new_block)

    # Replace image references with SD card paths
    for old, new in image_replacements.items():
        content = content.replace(old, new)

    # Remove image includes
    for inc in ['#include "ui/WizWatch/src/ui/images.h"', '#include "images.h"']:
        content = content.replace(inc, "")

    content = re.sub(r'\n{3,}', '\n\n', content)

    with open(UI_CPP, "w") as f:
        f.write(content)


def main():
    print("Discovering images...")
    image_replacements = discover_image_replacements()
    print(f"  Found {len(image_replacements)} images:")
    for old, new in sorted(image_replacements.items()):
        print(f"    {old} -> {new}")

    print("\nReading screens.c...")
    props = parse_screens_c()

    for name, calls in props.items():
        if name.startswith('_screen_'):
            screen_name = name[8:]  # Strip '_screen_' prefix
            print(f"  Screen '{screen_name}': {len(calls)} calls")
        elif calls:
            print(f"  {name}: {len(calls)} lv_* calls")

    print("\nPatching ui.cpp...")
    patch_ui_cpp(props, image_replacements)
    print("Done!")


if __name__ == "__main__":
    main()
