"""
Renames EEZ Studio generated .c files to .cpp for C++ compilation.

This is simpler than the wrapper approach and lets Arduino compile them directly.
"""

import os
import shutil

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
UI_DIR = os.path.join(SCRIPT_DIR, "..", "ui", "WizWatch", "src", "ui")


def rename_c_to_cpp():
    """Rename .c files to .cpp in the UI directory"""
    renamed = []

    # Main UI directory
    for filename in os.listdir(UI_DIR):
        if filename.endswith(".c") and not filename.startswith("."):
            old_path = os.path.join(UI_DIR, filename)
            new_path = os.path.join(UI_DIR, filename[:-2] + ".cpp")

            if os.path.exists(new_path):
                os.remove(new_path)

            shutil.move(old_path, new_path)
            renamed.append(filename + " → " + filename[:-2] + ".cpp")

    # Images subdirectory
    images_dir = os.path.join(UI_DIR, "images")
    if os.path.exists(images_dir):
        for filename in os.listdir(images_dir):
            if filename.endswith(".c") and not filename.startswith("."):
                old_path = os.path.join(images_dir, filename)
                new_path = os.path.join(images_dir, filename[:-2] + ".cpp")

                if os.path.exists(new_path):
                    os.remove(new_path)

                shutil.move(old_path, new_path)
                renamed.append("images/" + filename + " → " + filename[:-2] + ".cpp")

    return renamed


def main():
    print("Renaming .c files to .cpp for Arduino compatibility...")
    print()

    renamed = rename_c_to_cpp()

    if renamed:
        print(f"Renamed {len(renamed)} files:")
        for item in renamed:
            print(f"  ✓ {item}")
    else:
        print("  ○ No .c files found (already renamed?)")

    print("\nDone!")


if __name__ == "__main__":
    main()
