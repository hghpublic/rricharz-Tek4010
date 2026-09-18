# # tek_sequence_03.py

import sys
import time
import math

# Tektronix 4010/4014 Core Protocol Control Characters
ESC = b'\x1b'  # Escape character: Initiates system commands
FF  = b'\x0c'  # Form Feed character
GS  = b'\x1d'  # Group Separator: Switches to Graphic/Vector Mode
US  = b'\x1f'  # Unit Separator: Restores Alpha/Text Mode

# ANSI Terminal Layer Control Sequences
HIDE_CURSOR = b'\x1b[?25l'  # ANSI DECTCEM: Hides text cursor/caret
SHOW_CURSOR = b'\x1b[?25h'  # ANSI DECTCEM: Shows text cursor/caret


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
    """Sends the mandatory ESC + FF sequence to trigger full screen erasure."""
    sys.stdout.buffer.write(ESC + FF)
    sys.stdout.buffer.flush()
    time.sleep(0.3) 

def draw_shape(points):
    """Streams a list of (x, y) tuples as a closed continuous vector path."""
    sys.stdout.buffer.write(GS)
    for x, y in points:
        sys.stdout.buffer.write(encode_tek_coordinates(x, y))
    sys.stdout.buffer.write(US)
    sys.stdout.buffer.flush()

def main():
    center_x, center_y = 512, 390
    
    # 1. HIDE THE TEXT CURSOR BEFORE DRAWING
    sys.stdout.buffer.write(HIDE_CURSOR)
    sys.stdout.buffer.flush()
    
    print("Starting Tektronix sequence stream (Cursor Hidden)...", file=sys.stderr)
    time.sleep(0.5)

    try:
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
        t_radius = 180  
        triangle_points = []
        for i in range(4):  # 4 steps closes back to vertex 0
            angle = (2 * math.pi * i / 3) + (math.pi / 2)
            tx = center_x + t_radius * math.cos(angle)
            ty = center_y + t_radius * math.sin(angle)
            triangle_points.append((tx, ty))
        draw_shape(triangle_points)

        print("Sequence completed.", file=sys.stderr)

    finally:
        # 2. SHOW THE TEXT CURSOR AGAIN AT THE END
        # Wrapped in a finally block so it recovers even if you press Ctrl+C
        sys.stdout.buffer.write(SHOW_CURSOR)
        sys.stdout.buffer.flush()

if __name__ == "__main__":
    main()
