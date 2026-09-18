# tek_sequence.py

import sys
import time
import math

# Tektronix 4010/4014 Control Characters
GS = b'\x1d'  # Group Separator: Switches to Graphic/Vector Mode
US = b'\x1f'  # Unit Separator: Restores Alpha/Text Mode
FF = b'\x0c'  # Form Feed: Clears the display screen

def encode_tek_coordinates(x, y):
    """
    Converts 10-bit integer coordinates (0-1023 X, 0-779 Y) into 
    the 4-byte structural sequence required by Tektronix hardware.
    """
    x = max(0, min(int(x), 1023))
    y = max(0, min(int(y), 779))

    high_y = 0x20 | ((y >> 5) & 0x1F)
    low_y  = 0x60 | (y & 0x1F)
    high_x = 0x20 | ((x >> 5) & 0x1F)
    low_x  = 0x40 | (x & 0x1F)

    return bytes([high_y, low_y, high_x, low_x])

def clear_screen():
    """Clears the terminal and gives legacy/emulated storage tubes time to reset."""
    sys.stdout.buffer.write(FF)
    sys.stdout.buffer.flush()
    time.sleep(0.2) 

def draw_shape(points):
    """Streams a list of (x, y) tuples as a closed continuous vector path."""
    sys.stdout.buffer.write(GS)
    for x, y in points:
        sys.stdout.buffer.write(encode_tek_coordinates(x, y))
    sys.stdout.buffer.write(US)
    sys.stdout.buffer.flush()

def main():
    center_x, center_y = 512, 390
    
    print("Starting Tektronix sequence stream...", file=sys.stderr)
    time.sleep(0.5)

    # --- Step 1 & 2: Clear and Draw a Square ---
    clear_screen()
    size = 150
    square_points = [
        (center_x - size, center_y - size), # Bottom-Left
        (center_x + size, center_y - size), # Bottom-Right
        (center_x + size, center_y + size), # Top-Right
        (center_x - size, center_y + size), # Top-Left
        (center_x - size, center_y - size)  # Close path
    ]
    draw_shape(square_points)
    
    # --- Step 3: Wait 1 second ---
    time.sleep(1.0)

    # --- Step 4 & 5: Clear and Draw a Circle ---
    clear_screen()
    radius = 160
    steps = 32
    circle_points = []
    for i in range(steps + 1):
        angle = (2 * math.pi * i) / steps
        cx = center_x + radius * math.cos(angle)
        cy = center_y + radius * math.sin(angle)
        circle_points.append((cx, cy))
    draw_shape(circle_points)

    # --- Step 6: Wait 1 second ---
    time.sleep(1.0)

    # --- Step 7 & 8: Clear and Draw a Triangle ---
    clear_screen()
    t_radius = 180  # Distance from center to vertex points
    triangle_points = []
    # 3 vertices spaced out by 120 degrees starting pointing straight up
    for i in range(4):  # 4 loops to close back to index 0
        angle = (2 * math.pi * i / 3) + (math.pi / 2)
        tx = center_x + t_radius * math.cos(angle)
        ty = center_y + t_radius * math.sin(angle)
        triangle_points.append((tx, ty))
    draw_shape(triangle_points)

    print("Sequence completed.", file=sys.stderr)

if __name__ == "__main__":
    main()
