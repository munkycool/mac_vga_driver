import cv2
import numpy as np

# Open the video
cap = cv2.VideoCapture('mario_movie.mp4')
if not cap.isOpened():
    print("Error: Could not open mario_movie.mp4! Put it in this folder.")
    exit(1)

fps = cap.get(cv2.CAP_PROP_FPS)
total_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
duration = total_frames / fps
print(f"Video loaded: {fps:.2f} FPS, {total_frames} total frames, {duration:.1f} seconds.")

# Target specs
TARGET_FPS = 10
TARGET_WIDTH = 128
TARGET_HEIGHT = 96

frame_time_step = 1.0 / TARGET_FPS
current_time = 0.0

PALETTE = [
    (0, 0, 0),       # 0: Black
    (255, 0, 0),     # 1: Red
    (0, 255, 0),     # 2: Green
    (255, 255, 0),   # 3: Yellow
    (0, 0, 255),     # 4: Blue
    (255, 0, 255),   # 5: Magenta
    (0, 255, 255),   # 6: Cyan
    (255, 255, 255)  # 7: White
]

def dither_color_image(img):
    # Convert BGR (OpenCV) to RGB float
    rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB).astype(float)
    h, w, _ = rgb.shape
    out_indices = np.zeros((h, w), dtype=np.uint8)
    
    for y in range(h):
        for x in range(w):
            old_r, old_g, old_b = rgb[y, x]
            
            # Clip pixel values
            if old_r < 0.0: old_r = 0.0
            elif old_r > 255.0: old_r = 255.0
            
            if old_g < 0.0: old_g = 0.0
            elif old_g > 255.0: old_g = 255.0
            
            if old_b < 0.0: old_b = 0.0
            elif old_b > 255.0: old_b = 255.0
            
            # Find closest color in PALETTE
            min_dist = 1e9
            best_idx = 0
            for idx in range(8):
                pr, pg, pb = PALETTE[idx]
                dist = (pr - old_r)**2 + (pg - old_g)**2 + (pb - old_b)**2
                if dist < min_dist:
                    min_dist = dist
                    best_idx = idx
            
            out_indices[y, x] = best_idx
            new_r, new_g, new_b = PALETTE[best_idx]
            
            err_r = old_r - new_r
            err_g = old_g - new_g
            err_b = old_b - new_b
            
            # Distribute error (Floyd-Steinberg)
            if x + 1 < w:
                rgb[y, x + 1, 0] += err_r * 7 / 16
                rgb[y, x + 1, 1] += err_g * 7 / 16
                rgb[y, x + 1, 2] += err_b * 7 / 16
            if y + 1 < h:
                if x > 0:
                    rgb[y + 1, x - 1, 0] += err_r * 3 / 16
                    rgb[y + 1, x - 1, 1] += err_g * 3 / 16
                    rgb[y + 1, x - 1, 2] += err_b * 3 / 16
                rgb[y + 1, x, 0] += err_r * 5 / 16
                rgb[y + 1, x, 1] += err_g * 5 / 16
                rgb[y + 1, x, 2] += err_b * 5 / 16
                if x + 1 < w:
                    rgb[y + 1, x + 1, 0] += err_r * 1 / 16
                    rgb[y + 1, x + 1, 1] += err_g * 1 / 16
                    rgb[y + 1, x + 1, 2] += err_b * 1 / 16
                    
    return out_indices

def pack_3bit_row(row_indices):
    row_bytes = bytearray()
    for i in range(0, len(row_indices), 8):
        chunk = row_indices[i:i+8]
        val = 0
        for pixel in chunk:
            val = (val << 3) | (int(pixel) & 7)
        row_bytes.append((val >> 16) & 255)
        row_bytes.append((val >> 8) & 255)
        row_bytes.append(val & 255)
    return row_bytes

out_bytes = bytearray()
saved_count = 0

print("Processing video...")
while True:
    cap.set(cv2.CAP_PROP_POS_MSEC, current_time * 1000.0)
    ret, frame = cap.read()
    if not ret:
        break
    
    resized = cv2.resize(frame, (TARGET_WIDTH, TARGET_HEIGHT), interpolation=cv2.INTER_AREA)
    dithered_indices = dither_color_image(resized)
    
    for y in range(TARGET_HEIGHT):
        row_bytes = pack_3bit_row(dithered_indices[y])
        out_bytes.extend(row_bytes)
                
    saved_count += 1
    current_time += frame_time_step
    
    if saved_count % 100 == 0:
        print(f"Processed {saved_count} frames...")

cap.release()

# Save binary file
with open('mario_movie.bin', 'wb') as f:
    f.write(out_bytes)

print(f"Done! Saved {saved_count} frames to mario_movie.bin")
print(f"File size: {len(out_bytes)} bytes ({len(out_bytes)/1024/1024:.2f} MB)")
