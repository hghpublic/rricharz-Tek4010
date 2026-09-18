# tek_tunnel.py

import sys
import time
import math

# Tektronix Control Characters
GS = b'\x1d'  # Group Separator: Switches to Graphic/Vector Mode
US = b'\x1f'  # Unit Separator: Restores Alpha/Text Mode
FF = b'\x0c'  # Form Feed: Clears the vector display screen

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

def project_3d_to_2d(x, y, z, width=1024, height=780, fov=350):
    """
    Transforms 3D coordinates into 2D Tektronix screen boundaries 
    using perspective division.
    """
    if z <= 0:
        return None
    screen_x = int((x * fov) / z) + (width // 2)
    screen_y = int((y * fov) / z) + (height // 2)
    return screen_x, screen_y

def main():
    # Simulation Settings
    tunnel_w, tunnel_h = 500, 350
    speed = 25
    max_depth = 1000
    ring_spacing = 200
    
    # Initialize Z depth for the rectangular rings
    segments = list(range(ring_spacing, max_depth, ring_spacing))
    time_elapsed = 0.0
    
    try:
        print("Starting Tektronix Tunnel Stream... Press CTRL+C to stop.", file=sys.stderr)
        time.sleep(1)

        while True:
            # 1. Clear screen (Form Feed) to draw a brand new frame
            sys.stdout.buffer.write(FF)
            
            time_elapsed += 0.15
            # Simulates camera drift/flight swaying over time
            cam_x = math.sin(time_elapsed * 1.0) * 40
            cam_y = math.cos(time_elapsed * 0.7) * 30

            new_segments = []
            for z in segments:
                # Move ring closer along the Z axis
                z -= speed
                if z <= 0:
                    z += max_depth
                new_segments.append(z)

                # Define 3D corner coordinates of the rectangular ring
                half_w, half_h = tunnel_w // 2, tunnel_h // 2
                corners_3d = [
                    (-half_w - cam_x, -half_h - cam_y, z),  # Top-Left
                    ( half_w - cam_x, -half_h - cam_y, z),  # Top-Right
                    ( half_w - cam_x,  half_h - cam_y, z),  # Bottom-Right
                    (-half_w - cam_x,  half_h - cam_y, z),  # Bottom-Left
                ]

                # Project the corners into 2D pixel space
                points_2d = []
                for x3d, y3d, z3d in corners_3d:
                    pt = project_3d_to_2d(x3d, y3d, z3d)
                    if pt:
                        points_2d.append(pt)

                # If all 4 corners project onto the screen, draw the frame
                if len(points_2d) == 4:
                    # Switch into vector graphics engine mode
                    sys.stdout.buffer.write(GS)
                    
                    # Draw order path: 0 -> 1 -> 2 -> 3 -> 0 (Closes the box)
                    draw_order = [0, 1, 2, 3, 0]
                    for idx in draw_order:
                        px, py = points_2d[idx]
                        sys.stdout.buffer.write(encode_tek_coordinates(px, py))

            segments = new_segments
            
            # Switch back to text alpha mode safely before flushing the stream buffer
            sys.stdout.buffer.write(US)
            sys.stdout.buffer.flush()
            
            # Control frame rate pacing (gives terminal architectures time to process)
            time.sleep(0.05)

    except KeyboardInterrupt:
        # Gracefully handle emergency breaks and restore cursor
        sys.stdout.buffer.write(US)
        sys.stdout.buffer.flush()
        print("\nStream terminated safely.", file=sys.stderr)

if __name__ == "__main__":
    main()
