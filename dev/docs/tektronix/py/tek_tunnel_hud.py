# tek_tunnel_hud.py

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

def get_ring_points(z, cam_x, cam_y, tunnel_w=500, tunnel_h=350):
    """
    Calculates and projects the 4 corner points of a tunnel ring at depth Z.
    """
    half_w, half_h = tunnel_w // 2, tunnel_h // 2
    corners_3d = [
        (-half_w - cam_x, -half_h - cam_y, z),  # Top-Left (0)
        ( half_w - cam_x, -half_h - cam_y, z),  # Top-Right (1)
        ( half_w - cam_x,  half_h - cam_y, z),  # Bottom-Right (2)
        (-half_w - cam_x,  half_h - cam_y, z),  # Bottom-Left (3)
    ]
    
    points_2d = []
    for x3d, y3d, z3d in corners_3d:
        pt = project_3d_to_2d(x3d, y3d, z3d)
        points_2d.append(pt)
    return points_2d

def draw_hud(f):
    """
    Draws an aviation flight crosshair HUD in the center of the screen.
    """
    cx, cy = 512, 390
    
    # Central reticle ring (Approximated octagonal shape for vector speed)
    hud_ring = [
        (cx-15, cy-5), (cx-5, cy-15), (cx+5, cy-15), (cx+15, cy-5),
        (cx+15, cy+5), (cx+5, cy+15), (cx-5, cy+15), (cx-15, cy+5), (cx-15, cy-5)
    ]
    
    f.write(GS)
    for px, py in hud_ring:
        f.write(encode_tek_coordinates(px, py))
        
    # Left wing indicator line
    f.write(GS)
    f.write(encode_tek_coordinates(cx - 50, cy))
    f.write(encode_tek_coordinates(cx - 20, cy))
    
    # Right wing indicator line
    f.write(GS)
    f.write(encode_tek_coordinates(cx + 20, cy))
    f.write(encode_tek_coordinates(cx + 50, cy))
    
    # Pitch ladder top tick
    f.write(GS)
    f.write(encode_tek_coordinates(cx, cy + 20))
    f.write(encode_tek_coordinates(cx, cy + 30))

def main():
    # Simulation Settings
    speed = 30
    max_depth = 1100
    ring_spacing = 220
    
    # Initialize Z depth for the rectangular rings
    segments = list(range(ring_spacing, max_depth, ring_spacing))
    time_elapsed = 0.0
    
    # Order sequence to loop and close the box path (0 -> 1 -> 2 -> 3 -> 0)
    draw_order = (0, 1, 2, 3, 0)
    
    try:
        print("Starting Tektronix Tunnel Stream with HUD... Press CTRL+C to stop.", file=sys.stderr)
        time.sleep(1)

        while True:
            # 1. Clear screen (Form Feed) to begin drawing the new animation frame
            sys.stdout.buffer.write(FF)
            
            time_elapsed += 0.12
            # Camera drift values for this specific frame
            cam_x = math.sin(time_elapsed * 1.0) * 45
            cam_y = math.cos(time_elapsed * 0.7) * 30

            # Update and cycle ring depths
            new_segments = []
            for z in segments:
                z -= speed
                if z <= 0:
                    z += max_depth
                new_segments.append(z)
            segments = sorted(new_segments, reverse=True) # Sort back-to-front rendering

            # 2. Draw the integrated tunnel layout
            for i in range(len(segments)):
                z_curr = segments[i]
                pts_curr = get_ring_points(z_curr, cam_x, cam_y)
                
                # Check if current ring is fully visible on screen
                if any(p is None for p in pts_curr):
                    continue

                # FIXED: The loop now explicitly reads from the draw_order tuple
                sys.stdout.buffer.write(GS)
                for idx in draw_order:
                    px, py = pts_curr[idx]
                    sys.stdout.buffer.write(encode_tek_coordinates(px, py))

                # Draw longitudinal walls extending to the next consecutive ring depth frame
                if i < len(segments) - 1:
                    z_next = segments[i+1]
                    pts_next = get_ring_points(z_next, cam_x, cam_y)
                    
                    if not any(p is None for p in pts_next):
                        # Connect the 4 corners safely (Top-Left, Top-Right, Bottom-Right, Bottom-Left)
                        for c in range(4):
                            sys.stdout.buffer.write(GS)
                            sys.stdout.buffer.write(encode_tek_coordinates(pts_curr[c][0], pts_curr[c][1]))
                            sys.stdout.buffer.write(encode_tek_coordinates(pts_next[c][0], pts_next[c][1]))

            # 3. Overlay the UI Graphics HUD
            draw_hud(sys.stdout.buffer)

            # Switch back to text alpha mode safely before flushing the stream buffer
            sys.stdout.buffer.write(US)
            sys.stdout.buffer.flush()
            
            # Pacing delay for screen drawing stability
            time.sleep(0.04)

    except KeyboardInterrupt:
        sys.stdout.buffer.write(US)
        sys.stdout.buffer.flush()
        print("\nStream terminated safely.", file=sys.stderr)

if __name__ == "__main__":
    main()
