# tek_square.py

import sys
import time

# Tektronix 4010/4014 Control Characters
GS = b'\x1d'  # Group Separator: Switches terminal to Vector Graphics (Graph Mode)
US = b'\x1f'  # Unit Separator: Switches terminal back to Alpha-Numeric (Text Mode)
FF = b'\x0c'  # Form Feed: Clears the vector display screen

def encode_tek_coordinates(x, y):
    """
    Converts 10-bit integer coordinates (0-1023) into 
    the 4-byte structural sequence required by Tektronix hardware.
    """
    # Enforce standard 4010 resolution boundaries
    x = max(0, min(int(x), 1023))
    y = max(0, min(int(y), 779)) # 4010 vertical limit is 779

    # Deconstruct coordinate bit masks
    high_y = 0x20 | ((y >> 5) & 0x1F)
    low_y  = 0x60 | (y & 0x1F)
    high_x = 0x20 | ((x >> 5) & 0x1F)
    low_x  = 0x40 | (x & 0x1F)

    return bytes([high_y, low_y, high_x, low_x])

def main():
    # 1. Clear the terminal screen
    sys.stdout.buffer.write(FF)
    sys.stdout.buffer.flush()
    time.sleep(0.5) # Give legacy hardware/emulators time to reset

    # 2. Trigger Vector Graphics Mode
    sys.stdout.buffer.write(GS)

    # 3. Define Vector Coordinates (A simple square box boundary)
    # The very first coordinate sent after GS acts as a "Move to" (dark vector)
    coordinates = [
        (200, 200),  # Move to starting point
        (600, 200),  # Draw line to bottom-right
        (600, 600),  # Draw line to top-right
        (200, 600),  # Draw line to top-left
        (200, 200)   # Close the box
    ]

    # Stream the coordinate data bytes smoothly
    for x, y in coordinates:
        packet = encode_tek_coordinates(x, y)
        sys.stdout.buffer.write(packet)
        sys.stdout.buffer.flush()
        time.sleep(0.1) # Simulates a slow hardware stream

    # 4. Safely fall back to normal text mode and position cursor
    sys.stdout.buffer.write(US)
    sys.stdout.buffer.flush()
    print("\nStream finished successfully.")

if __name__ == "__main__":
    main()
