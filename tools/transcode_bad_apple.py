import cv2
import numpy as np

# Open the video
cap = cv2.VideoCapture('bad_apple.mp4')
if not cap.isOpened():
    print("Error: Could not open bad_apple.mp4!")
    exit(1)

fps = cap.get(cv2.CAP_PROP_FPS)
total_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
duration = total_frames / fps
print(f"Video loaded: {fps:.2f} FPS, {total_frames} total frames, {duration:.1f} seconds.")

# Target specs
TARGET_FPS = 20
TARGET_WIDTH = 128
TARGET_HEIGHT = 96

frame_time_step = 1.0 / TARGET_FPS
current_time = 0.0

out_bytes = bytearray()
saved_count = 0

print("Processing video...")
while True:
    cap.set(cv2.CAP_PROP_POS_MSEC, current_time * 1000.0)
    ret, frame = cap.read()
    if not ret:
        break
    
    # Convert to grayscale
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    
    # Resize
    resized = cv2.resize(gray, (TARGET_WIDTH, TARGET_HEIGHT), interpolation=cv2.INTER_AREA)
    
    # Threshold to binary (0 or 255)
    _, thresh = cv2.threshold(resized, 127, 255, cv2.THRESH_BINARY)
    
    # Pack 1-bit pixels (8 pixels per byte)
    for y in range(TARGET_HEIGHT):
        row = thresh[y]
        for i in range(0, TARGET_WIDTH, 8):
            byte_val = 0
            for bit_idx in range(8):
                pixel_val = row[i + bit_idx]
                bit = 1 if pixel_val > 127 else 0
                byte_val = (byte_val << 1) | bit
            out_bytes.append(byte_val)
                
    saved_count += 1
    current_time += frame_time_step
    
    if saved_count % 100 == 0:
        print(f"Processed {saved_count} frames...")

cap.release()

# Save binary file
with open('bad_apple.bin', 'wb') as f:
    f.write(out_bytes)

print(f"Done! Saved {saved_count} frames to bad_apple.bin")
print(f"File size: {len(out_bytes)} bytes ({len(out_bytes)/1024:.2f} KB)")
