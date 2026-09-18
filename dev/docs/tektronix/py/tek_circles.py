# tek_circles.py

import math

# Tektronix 4010/4014 Control Bytes
GS = b'\x1d'  # Group Separator: Switches to Graphic/Vector Mode
US = b'\x1f'  # Unit Separator: Switches back to Alpha/Text Mode
FF = b'\x0c'  # Form Feed: Clears the display screen

def encode_tek_coordinates(x, y):
    """
    Converts 10-bit integer coordinates (0-1023 X, 0-779 Y) into 
    the 4-byte structural sequence required by Tektronix protocols.
    """
    x = max(0, min(int(x), 1023))
    y = max(0, min(int(y), 779))

    high_y = 0x20 | ((y >> 5) & 0x1F)
    low_y  = 0x60 | (y & 0x1F)
    high_x = 0x20 | ((x >> 5) & 0x1F)
    low_x  = 0x40 | (x & 0x1F)

    return bytes([high_y, low_y, high_x, low_x])

def generate_tek_plt(filename):
    # Core display configurations
    center_x, center_y = 512, 390  # Center of the 4010 screen (1024x780 max)
    max_radius = 370               # Max radius to stay perfectly inside boundaries
    num_circles = 100
    points_per_circle = 64         # Line segments per circle for smooth geometry

    with open(filename, 'wb') as f:
        # 1. Clear terminal screen
        f.write(FF)
        
        # Calculate radius step for 100 perfectly spaced concentric rings
        radius_step = max_radius / num_circles

        for i in range(1, num_circles + 1):
            radius = i * radius_step
            
            # 2. Drop into Vector Mode for the start of every distinct circle
            f.write(GS)
            
            for step in range(points_per_circle + 1):
                # Calculate angle in radians
                angle = (2 * math.pi * step) / points_per_circle
                
                # Plot X and Y coordinates
                x = center_x + radius * math.cos(angle)
                y = center_y + radius * math.sin(angle)
                
                # Write 4-byte coordinate sequence
                # Note: The very first coordinate after the GS command above
                # acts automatically as a "dark vector" (Move cursor without drawing).
                f.write(encode_tek_coordinates(x, y))
        
        # 3. Exit back to regular alpha text mode safely at the end of file
        f.write(US)

if __name__ == "__main__":
    output_file = "tek_circles.plt"
    generate_tek_plt(output_file)
    print(f"Successfully generated '{output_file}' containing 100 concentric circles.")
