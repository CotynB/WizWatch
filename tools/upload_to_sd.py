"""
Upload .bin image files to ESP32 SD card via USB serial (text-based protocol).

Steps:
  1. Flash the SD_Uploader sketch to your ESP32
  2. Run: python upload_to_sd.py COM3
  3. Re-flash the WizWatch sketch when done

Requires: pip install pyserial
"""

import serial
import sys
import os
import time
import re
import struct

# LVGL v9 constants
LV_IMAGE_HEADER_MAGIC = 0x19
LV_COLOR_FORMAT_ARGB8888 = 0x10

# Images directory (relative to this script)
IMAGES_DIR = "../ui/WizWatch/src/ui/images"


def discover_images(script_dir):
    """Auto-discover all ui_image_*.c files and return (source_path, output_name) tuples."""
    images_path = os.path.join(script_dir, IMAGES_DIR)
    images = []

    if not os.path.isdir(images_path):
        print(f"WARNING: Images directory not found: {images_path}")
        return images

    for filename in os.listdir(images_path):
        if filename.startswith("ui_image_") and filename.endswith(".c"):
            # Extract name: ui_image_fond.c -> fond
            name = filename[9:-2]  # Strip "ui_image_" prefix and ".c" suffix
            src = os.path.join(IMAGES_DIR, filename)
            output = f"{name}.bin"
            images.append((src, output))

    return sorted(images, key=lambda x: x[1])  # Sort by output name for consistency

CHUNK_SIZE = 64  # raw bytes per chunk, sent as 128 hex chars


def parse_image_info(filepath):
    """Read w, h, stride from the lv_image_dsc_t at the end of a C file."""
    with open(filepath, "r") as f:
        content = f.read()

    w = int(re.search(r'\.w\s*=\s*(\d+)', content).group(1))
    h = int(re.search(r'\.h\s*=\s*(\d+)', content).group(1))
    stride = int(re.search(r'\.stride\s*=\s*(\d+)', content).group(1))
    return w, h, stride


def extract_bytes_from_c_file(filepath):
    with open(filepath, "r") as f:
        content = f.read()
    # Only match hex bytes inside the array (between { and })
    array_match = re.search(r'img_\w+_map\[\]\s*=\s*\{(.*?)\}', content, re.DOTALL)
    if array_match:
        array_content = array_match.group(1)
    else:
        array_content = content
    hex_values = re.findall(r'0x([0-9a-fA-F]{2})', array_content)
    return bytes(int(h, 16) for h in hex_values)


def make_lv_image_header(w, h, stride):
    word0 = LV_IMAGE_HEADER_MAGIC | (LV_COLOR_FORMAT_ARGB8888 << 8) | (0 << 16)
    word1 = w | (h << 16)
    word2 = stride | (0 << 16)
    return struct.pack("<III", word0, word1, word2)


def convert_image(src_path):
    w, h, stride = parse_image_info(src_path)
    print(f"  Dimensions: {w}x{h}, stride={stride}")
    pixel_data = extract_bytes_from_c_file(src_path)
    expected = stride * h
    print(f"  Data: {len(pixel_data)} bytes (expected {expected})")
    header = make_lv_image_header(w, h, stride)
    return header + pixel_data


def wait_for_line(ser, timeout=30):
    start = time.time()
    while time.time() - start < timeout:
        if ser.in_waiting:
            line = ser.readline().decode("utf-8", errors="ignore").strip()
            if line:
                return line
        time.sleep(0.01)
    return None


def upload_file(ser, filename, data):
    size = len(data)
    print(f"\nUploading {filename} ({size} bytes / {size / 1024 / 1024:.2f} MB)")

    cmd = f"FILE:{filename}:{size}\n"
    ser.write(cmd.encode())

    # Wait for READY
    while True:
        line = wait_for_line(ser, timeout=10)
        if line is None:
            print("  Timeout waiting for READY")
            return False
        print(f"  < {line}")
        if line == "READY":
            break
        if line.startswith("ERROR"):
            return False

    # Send data as hex-encoded text lines
    sent = 0
    while sent < size:
        end = min(sent + CHUNK_SIZE, size)
        chunk = data[sent:end]
        hex_str = chunk.hex()
        ser.write(f"DATA:{hex_str}\n".encode())
        ser.flush()
        sent = end

        # Wait for NEXT:<bytecount>
        while True:
            line = wait_for_line(ser, timeout=20)
            if line is None:
                print(f"\n  Timeout at {sent}/{size}")
                return False
            if line.startswith("NEXT:"):
                decoded_count = int(line.split(":")[1])
                if decoded_count != len(chunk):
                    print(f"\n  Data mismatch! Sent {len(chunk)} bytes, ESP decoded {decoded_count}")
                    return False
                break
            if line.startswith("ERROR"):
                print(f"\n  < {line}")
                return False

        pct = sent * 100 // size
        print(f"\r  Sent {sent}/{size} ({pct}%)", end="", flush=True)

    # Send END
    ser.write(b"END\n")
    print()

    # Wait for OK
    while True:
        line = wait_for_line(ser, timeout=15)
        if line is None:
            print("  Timeout waiting for confirmation")
            return False
        print(f"  < {line}")
        if line.startswith("OK:"):
            return True
        if line.startswith("ERROR"):
            return False


def main():
    if len(sys.argv) < 2:
        print(f"Usage: python {sys.argv[0]} <COM_PORT>")
        print(f"Example: python {sys.argv[0]} COM3")
        sys.exit(1)

    port = sys.argv[1]
    script_dir = os.path.dirname(os.path.abspath(__file__))

    print(f"Connecting to {port}...")
    ser = serial.Serial(port, 115200, timeout=1)

    # Toggle DTR/RTS to reset the ESP32
    ser.dtr = False
    ser.rts = False
    time.sleep(0.1)
    ser.dtr = True
    ser.rts = True
    time.sleep(3)

    # Wait for UPLOAD_READY
    print("Waiting for ESP32...")
    for attempt in range(3):
        if attempt > 0:
            print(f"  Retry {attempt}...")
            ser.dtr = False
            time.sleep(0.1)
            ser.dtr = True
            time.sleep(3)

        while True:
            line = wait_for_line(ser, timeout=10)
            if line is None:
                break
            print(f"  < {line}")
            if line == "UPLOAD_READY":
                break
        else:
            continue
        break
    else:
        print("Timeout - is SD_Uploader sketch flashed? Is the COM port correct?")
        ser.close()
        sys.exit(1)

    # Discover and upload each image
    images = discover_images(script_dir)
    if not images:
        print("No images found to upload!")
        ser.close()
        sys.exit(1)

    print(f"\nFound {len(images)} images to upload:")
    for src, name in images:
        print(f"  {name}")

    for src, name in images:
        src_path = os.path.join(script_dir, src)
        if not os.path.exists(src_path):
            print(f"ERROR: Source file not found: {src_path}")
            continue

        print(f"\nConverting {src}...")
        data = convert_image(src_path)

        if not upload_file(ser, name, data):
            print(f"Failed to upload {name}")
            ser.close()
            sys.exit(1)

    ser.write(b"DONE\n")
    line = wait_for_line(ser)
    if line:
        print(f"  < {line}")

    ser.close()
    print("\nAll files uploaded! You can now re-flash the WizWatch sketch.")


if __name__ == "__main__":
    main()
