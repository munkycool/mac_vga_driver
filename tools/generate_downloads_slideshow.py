#!/usr/bin/env python3

from __future__ import annotations

import argparse
import base64
import json
import re
import os
import subprocess
import sys
from pathlib import Path

import cv2
import numpy as np


TARGET_WIDTH = 320
TARGET_HEIGHT = 240
TARGET_FRAME_BYTES = TARGET_WIDTH * TARGET_HEIGHT

PALETTE = np.array([
    (0, 0, 0),
    (255, 0, 0),
    (0, 255, 0),
    (255, 255, 0),
    (0, 0, 255),
    (255, 0, 255),
    (0, 255, 255),
    (255, 255, 255),
], dtype=np.float32)

SUPPORTED_EXTENSIONS = {
    ".bmp",
    ".gif",
    ".jpeg",
    ".jpg",
    ".png",
    ".svg",
    ".tif",
    ".tiff",
    ".webp",
}


def discover_images(input_dir: Path) -> list[Path]:
    if not input_dir.is_dir():
        return []
    return sorted(
        [path for path in input_dir.iterdir() if path.is_file() and path.suffix.lower() in SUPPORTED_EXTENSIONS],
        key=lambda path: path.name.lower(),
    )


def load_via_convert(path: Path) -> np.ndarray:
    command = [
        "convert",
        f"{path}[0]",
        "-auto-orient",
        "-background",
        "black",
        "-alpha",
        "remove",
        "-alpha",
        "off",
        "-resize",
        f"{TARGET_WIDTH}x{TARGET_HEIGHT}",
        "-gravity",
        "center",
        "-extent",
        f"{TARGET_WIDTH}x{TARGET_HEIGHT}",
        "png:-",
    ]
    completed = subprocess.run(command, check=True, capture_output=True)
    decoded = cv2.imdecode(np.frombuffer(completed.stdout, dtype=np.uint8), cv2.IMREAD_COLOR)
    if decoded is None:
        raise RuntimeError(f"convert output could not be decoded for {path}")
    return decoded


def load_svg(path: Path) -> np.ndarray:
    text = path.read_text(errors="ignore")
    match = re.search(r"data:image/[^;]+;base64,([^\"]+)", text, re.IGNORECASE | re.DOTALL)
    if match is None:
        raise RuntimeError(f"no embedded raster image found in {path}")

    png_bytes = base64.b64decode(re.sub(r"\s+", "", match.group(1)))
    decoded = cv2.imdecode(np.frombuffer(png_bytes, dtype=np.uint8), cv2.IMREAD_COLOR)
    if decoded is None:
        raise RuntimeError(f"embedded PNG could not be decoded for {path}")
    return decoded


def load_image(path: Path) -> np.ndarray:
    if path.suffix.lower() == ".svg":
        return load_svg(path)

    try:
        return load_via_convert(path)
    except Exception:
        image = cv2.imread(str(path), cv2.IMREAD_COLOR)
        if image is None:
            raise
        resized = cv2.resize(image, (TARGET_WIDTH, TARGET_HEIGHT), interpolation=cv2.INTER_AREA)
        return resized


def dither_color_image(image_bgr: np.ndarray) -> np.ndarray:
    rgb = cv2.cvtColor(image_bgr, cv2.COLOR_BGR2RGB).astype(np.float32)
    height, width, _ = rgb.shape
    output = np.zeros((height, width), dtype=np.uint8)

    for y in range(height):
        for x in range(width):
            old_pixel = rgb[y, x]
            old_pixel = np.clip(old_pixel, 0.0, 255.0)

            deltas = PALETTE - old_pixel
            distances = np.sum(deltas * deltas, axis=1)
            best_index = int(np.argmin(distances))
            output[y, x] = best_index

            new_pixel = PALETTE[best_index]
            error = old_pixel - new_pixel

            if x + 1 < width:
                rgb[y, x + 1] += error * (7.0 / 16.0)
            if y + 1 < height:
                if x > 0:
                    rgb[y + 1, x - 1] += error * (3.0 / 16.0)
                rgb[y + 1, x] += error * (5.0 / 16.0)
                if x + 1 < width:
                    rgb[y + 1, x + 1] += error * (1.0 / 16.0)

    return output


def convert_to_frame(image_bgr: np.ndarray) -> bytes:
    dithered = dither_color_image(image_bgr)
    frame = bytearray(TARGET_FRAME_BYTES)

    for y in range(TARGET_HEIGHT):
        row_offset = y * TARGET_WIDTH
        for x in range(TARGET_WIDTH):
            pixel = (int(dithered[y, x]) & 7) | 0x08
            frame[row_offset + x] = (pixel << 4) | pixel

    return bytes(frame)


def write_generated_source(output_file: Path, frames: list[tuple[str, bytes]]) -> None:
    output_file.parent.mkdir(parents=True, exist_ok=True)

    lines = []
    lines.append("// Auto-generated from the local Downloads folder.\n")
    lines.append("#include \"common.h\"\n\n")
    lines.append(f"const size_t downloads_image_count = {len(frames)};\n")

    array_size = len(frames) if frames else 1
    lines.append(
        f"const uint8_t downloads_images[{array_size}][DOWNLOADS_FRAME_BYTES] = {{\n"
    )

    if frames:
        for frame_index, (name, frame_bytes) in enumerate(frames):
            lines.append(f"    // {name}\n")
            lines.append("    {\n")
            for offset in range(0, len(frame_bytes), 12):
                chunk = frame_bytes[offset : offset + 12]
                line = ", ".join(f"0x{byte:02X}" for byte in chunk)
                lines.append(f"        {line},\n")
            lines.append("    }")
            lines.append(",\n" if frame_index + 1 < len(frames) else "\n")
    else:
        lines.append("    {\n")
        lines.append("        0x88,\n")
        lines.append("    }\n")

    lines.append("};\n")
    new_content = "".join(lines)

    if output_file.exists():
        try:
            existing_content = output_file.read_text(encoding="utf-8")
            if existing_content == new_content:
                return
        except Exception:
            pass

    output_file.write_text(new_content, encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate slideshow frame data from Downloads images.")
    parser.add_argument("--input-dir", required=True, type=Path)
    parser.add_argument("--output-file", required=True, type=Path)
    args = parser.parse_args()

    images = discover_images(args.input_dir)
    frames: list[tuple[str, bytes]] = []

    cache_file = args.output_file.parent / ".slideshow_cache.json"
    cache = {}
    if cache_file.exists():
        try:
            cache = json.loads(cache_file.read_text(encoding="utf-8"))
        except Exception:
            pass

    new_cache = {}

    for path in images:
        try:
            stat = path.stat()
            mtime = stat.st_mtime
            size = stat.st_size
            
            cache_key = path.name
            cached_entry = cache.get(cache_key)
            if (cached_entry and 
                cached_entry.get("mtime") == mtime and 
                cached_entry.get("size") == size and 
                "frame_hex" in cached_entry):
                frame_bytes = bytes.fromhex(cached_entry["frame_hex"])
            else:
                image_bgr = load_image(path)
                frame_bytes = convert_to_frame(image_bgr)
            
            frames.append((path.name, frame_bytes))
            new_cache[cache_key] = {
                "mtime": mtime,
                "size": size,
                "frame_hex": frame_bytes.hex()
            }
        except Exception as exc:
            print(f"Skipping {path}: {exc}", file=sys.stderr)

    try:
        cache_file.parent.mkdir(parents=True, exist_ok=True)
        cache_file.write_text(json.dumps(new_cache, indent=2), encoding="utf-8")
    except Exception as exc:
        print(f"Warning: failed to write cache file: {exc}", file=sys.stderr)

    write_generated_source(args.output_file, frames)
    print(f"Generated {len(frames)} slideshow frames from {args.input_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())