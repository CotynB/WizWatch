"""
Master script to update UI after EEZ Studio re-export.

Runs all necessary patches in the correct order.
"""

import subprocess
import sys
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))


def run_script(script_name):
    """Run a Python script and return success status"""
    script_path = os.path.join(SCRIPT_DIR, script_name)
    print(f"\n{'='*60}")
    print(f"Running {script_name}...")
    print('='*60)

    try:
        result = subprocess.run([sys.executable, script_path], check=True)
        return True
    except subprocess.CalledProcessError as e:
        print(f"ERROR: {script_name} failed with exit code {e.returncode}")
        return False


def main():
    print("=" * 60)
    print("WizWatch UI Update - Post EEZ Studio Export")
    print("=" * 60)

    scripts = [
        "rename_to_cpp.py",     # First: Rename .c to .cpp for Arduino
        "patch_cpp_compat.py",  # Second: C++ compatibility fixes
        "patch_screens.py",     # Third: SD card image path replacements
    ]

    success = True
    for script in scripts:
        if not run_script(script):
            success = False
            break

    print("\n" + "=" * 60)
    if success:
        print("✓ All patches applied successfully!")
        print("\nNext steps:")
        print("  1. DELETE ui_wrapper.cpp (not needed anymore!)")
        print("  2. Compile and upload your sketch in Arduino IDE")
        print("  3. Run: python upload_to_sd.py COM3")
        print("     (to upload images to SD card)")
    else:
        print("✗ Patching failed - please check errors above")
    print("=" * 60)

    return 0 if success else 1


if __name__ == "__main__":
    sys.exit(main())
