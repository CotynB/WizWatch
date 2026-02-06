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

IMAGE_REPLACEMENTS = {
    "&img_fond": '"S:fond.bin"',
    "&img_full": '"S:full.bin"',
    "&img_half_full": '"S:half_full.bin"',
    "&img_empty": '"S:empty.bin"',
}

OBJECTS = ["fond", "battery_full", "battery_half_full", "battery_empty", "time_lbl"]


def extract_lv_calls(block):
    """Extract all lv_* function calls on 'obj' from a code block."""
    calls = {}
    # Match lines like: lv_something(obj, ...);
    for match in re.finditer(r'(lv_\w+)\(obj,\s*(.*?)\);', block):
        func_name = match.group(1)
        args = match.group(2).strip()
        calls[func_name] = args
    return calls


def parse_screens_c():
    """Parse screens.c to extract all lv_* calls per object."""
    with open(SCREENS_C, "r") as f:
        content = f.read()

    props = {}

    for obj_name in OBJECTS:
        pattern = rf'objects\.{re.escape(obj_name)}\s*=\s*obj;(.*?)(?=objects\.\w+\s*=|$)'
        match = re.search(pattern, content, re.DOTALL)
        if not match:
            continue
        block = match.group(1)
        props[obj_name] = extract_lv_calls(block)

    # Screen size
    screen_match = re.search(
        r'objects\.main\s*=\s*obj;(.*?)(?=\{\s*lv_obj_t \*parent_obj)',
        content, re.DOTALL
    )
    if screen_match:
        props['_screen'] = extract_lv_calls(screen_match.group(1))

    return props


def patch_ui_cpp(props):
    """Update ui.cpp with extracted properties."""
    with open(UI_CPP, "r") as f:
        content = f.read()

    # Update screen-level calls
    if '_screen' in props:
        for func, args in props['_screen'].items():
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
    for old, new in IMAGE_REPLACEMENTS.items():
        content = content.replace(old, new)

    # Remove image includes
    for inc in ['#include "ui/WizWatch/src/ui/images.h"', '#include "images.h"']:
        content = content.replace(inc, "")

    content = re.sub(r'\n{3,}', '\n\n', content)

    with open(UI_CPP, "w") as f:
        f.write(content)


def main():
    print("Reading screens.c...")
    props = parse_screens_c()

    for name, calls in props.items():
        if name.startswith('_'):
            print(f"  Screen: {calls}")
        else:
            print(f"  {name}:")
            for func, args in calls.items():
                print(f"    {func}(obj, {args})")

    print("\nPatching ui.cpp...")
    patch_ui_cpp(props)
    print("Done!")


if __name__ == "__main__":
    main()
