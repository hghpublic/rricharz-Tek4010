# tek_interactive.py

import sys
import math
import curses

# Tektronix 4010/4014 Control Bytes
GS = b'\x1d'  # Group Separator: Switches to Graphic Vector Mode
US = b'\x1f'  # Unit Separator: Switches to Alpha Text Mode
FF = b'\x0c'  # Form Feed: Clears the display vector screen

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

def draw_frame(circle_x, circle_y):
    """
    Renders the outer bounding rectangle and the moving circle inside it.
    """
    # Define box boundaries
    box_x1, box_y1 = 100, 100
    box_x2, box_y2 = 924, 679
    
    # 1. Clear Screen
    sys.stdout.buffer.write(FF)
    
    # 2. Draw Outer Rectangle (0-to-4 path sequence)
    rect_points = [
        (box_x1, box_y1), (box_x2, box_y1), 
        (box_x2, box_y2), (box_x1, box_y2), (box_x1, box_y1)
    ]
    sys.stdout.buffer.write(GS)
    for px, py in rect_points:
        sys.stdout.buffer.write(encode_tek_coordinates(px, py))
        
    # 3. Draw Moving Circle (Approximated 24-point polygon for speed)
    radius = 35
    circle_steps = 24
    
    sys.stdout.buffer.write(GS)
    for step in range(circle_steps + 1):
        angle = (2 * math.pi * step) / circle_steps
        cx = circle_x + radius * math.cos(angle)
        cy = circle_y + radius * math.sin(angle)
        sys.stdout.buffer.write(encode_tek_coordinates(cx, cy))
        
    # 4. Safe return to Text mode
    sys.stdout.buffer.write(US)
    sys.stdout.buffer.flush()

def interactive_loop(stdscr):
    # Configure curses window settings
    curses.curs_set(0)  # Hide cursor representation
    stdscr.nodelay(True)  # Make getch non-blocking so rendering loop stays active
    stdscr.keypad(True)  # Enable detection of Special Arrow keys
    
    # Initial starting positions for the circle
    x, y = 512, 390
    step_size = 15
    
    # Render the initial starting view
    draw_frame(x, y)
    
    while True:
        try:
            # Capture keyboard buffer inputs
            key = stdscr.getch()
            
            if key == ord('q') or key == 27:  # Exit on 'q' or ESC key
                break
                
            moved = False
            # Check key definitions and enforce bounding box limits
            if key == curses.KEY_LEFT and x > 150:
                x -= step_size
                moved = True
            elif key == curses.KEY_RIGHT and x < 874:
                x += step_size
                moved = True
            elif key == curses.KEY_UP and y < 629:
                y += step_size
                moved = True
            elif key == curses.KEY_DOWN and y > 150:
                y -= step_size
                moved = True
                
            # Only refresh data line vectors if an action occurs
            if moved:
                draw_frame(x, y)
                
            # Prevent 100% CPU thread starvation usage
            curses.napms(10)
            
        except KeyboardInterrupt:
            break

def main():
    # Run the interactive script wrapped inside curses protection shield
    curses.wrapper(interactive_loop)
    
    # Ensure terminal returns to Alpha Text Mode permanently upon close
    sys.stdout.buffer.write(US)
    sys.stdout.buffer.flush()
    print("\nProgram exited cleanly.")

if __name__ == "__main__":
    main()
