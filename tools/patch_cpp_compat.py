"""
Patches EEZ Studio generated files for C++ compatibility.

Run this after every EEZ Studio re-export (after renaming .c to .cpp).
"""

import os
import re

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
UI_DIR = os.path.join(SCRIPT_DIR, "..", "ui", "WizWatch", "src", "ui")

UI_H = os.path.join(UI_DIR, "ui.h")
UI_CPP = os.path.join(UI_DIR, "ui.cpp")  # Now .cpp after rename
STRUCTS_H = os.path.join(UI_DIR, "structs.h")
SCREENS_CPP = os.path.join(UI_DIR, "screens.cpp")  # Now .cpp after rename


def patch_ui_h():
    """Add eez-framework.h include to ui.h"""
    with open(UI_H, "r") as f:
        content = f.read()

    # Replace the specific include line
    original = '#if defined(EEZ_FOR_LVGL)\n#include <eez/flow/lvgl_api.h>\n#endif'
    replacement = '#if defined(EEZ_FOR_LVGL)\n#include <eez-framework.h>\n#endif'

    if original in content:
        content = content.replace(original, replacement)
        with open(UI_H, "w") as f:
            f.write(content)
        return True
    return False


def patch_structs_h():
    """Add eez-framework.h include to structs.h"""
    with open(STRUCTS_H, "r") as f:
        content = f.read()

    # Add eez-framework.h include after EEZ_FOR_LVGL check
    pattern = r'(#if defined\(EEZ_FOR_LVGL\)\s*\n)(#include <eez/flow/flow\.h>)'
    replacement = r'\1#include <eez-framework.h>\n\2'

    new_content = re.sub(pattern, replacement, content)

    if new_content != content:
        with open(STRUCTS_H, "w") as f:
            f.write(new_content)
        return True
    return False


def patch_ui_cpp():
    """Add extern C blocks around global arrays and functions in ui.cpp"""
    with open(UI_CPP, "r") as f:
        content = f.read()

    original = content

    # 1. Add eez-framework.h include
    content = content.replace(
        '#if defined(EEZ_FOR_LVGL)\n#include <eez/core/vars.h>\n#endif',
        '#if defined(EEZ_FOR_LVGL)\n#include <eez-framework.h>\n#include <eez/core/vars.h>\n#endif'
    )

    # 2. Cast native variable function pointers to void*
    # Find patterns like: { TYPE, get_func, set_func }
    content = re.sub(
        r'\{\s*NATIVE_VAR_TYPE_(\w+),\s*(\w+),\s*(\w+)\s*\}',
        r'{ NATIVE_VAR_TYPE_\1, (void*)\2, (void*)\3 }',
        content
    )

    # 3. Wrap native_vars and actions arrays in extern "C"
    pattern1 = r'(};\s*\n)(native_var_t native_vars\[\] = \{.*?\};\s*\n\s*\nActionExecFunc actions\[\] = \{.*?\};)'
    replacement1 = r'\1\n#ifdef __cplusplus\nextern "C" {\n#endif\n\n\2\n\n#ifdef __cplusplus\n}\n#endif'
    content = re.sub(pattern1, replacement1, content, flags=re.DOTALL)

    # 3. Wrap ui_init and ui_tick in extern "C" (EEZ_FOR_LVGL section)
    pattern2 = r'(#if defined\(EEZ_FOR_LVGL\)\s*\n)(void ui_init\(\).*?void ui_tick\(\).*?\})\s*\n(#else)'
    replacement2 = r'\1\n#ifdef __cplusplus\nextern "C" {\n#endif\n\n\2\n\n#ifdef __cplusplus\n}\n#endif\n\n\3'
    content = re.sub(pattern2, replacement2, content, flags=re.DOTALL)

    if content != original:
        with open(UI_CPP, "w") as f:
            f.write(content)
        return True
    return False


def patch_screens_cpp():
    """Add C++ cast to lv_event_get_target() calls"""
    with open(SCREENS_CPP, "r") as f:
        content = f.read()

    # Add cast to lv_event_get_target
    pattern = r'lv_obj_t \*(\w+) = lv_event_get_target\(e\);'
    replacement = r'lv_obj_t *\1 = (lv_obj_t *)lv_event_get_target(e);'

    new_content = re.sub(pattern, replacement, content)

    if new_content != content:
        with open(SCREENS_CPP, "w") as f:
            f.write(new_content)
        return True
    return False


def main():
    print("Patching EEZ Studio files for C++ compatibility...")

    results = {
        "ui.h": patch_ui_h(),
        "structs.h": patch_structs_h(),
        "ui.cpp": patch_ui_cpp(),
        "screens.cpp": patch_screens_cpp()
    }

    print("\nResults:")
    for file, patched in results.items():
        status = "✓ Patched" if patched else "○ No changes needed"
        print(f"  {status}: {file}")

    print("\nDone!")


if __name__ == "__main__":
    main()
