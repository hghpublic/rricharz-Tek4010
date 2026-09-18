# tek_hello.py


import sys
import time

# Tektronix 4010/4014 Control Sequences
ESC = '\x1b'
FF  = '\x0c'  # Form Feed / Page Clear
US  = '\x1f'  # Unit Separator / Enter Alpha (Text) Mode

def send_stream(data):
    """Sends raw bytes immediately to stdout without newline buffering."""
    sys.stdout.write(data)
    sys.stdout.flush()

def main():
    # (1) Clear the screen & home the cursor
    # The canonical Tektronix PAGE sequence is ESC + FF
    send_stream(ESC + FF)
    
    # (2) Wait 1 second
    time.sleep(1)
    
    # (3) Force Alpha Mode and write 'Hello, world!' in the vector font
    send_stream(US + 'Hello, world!')
    
    # (4) Wait 3 seconds
    time.sleep(3)
    
    # (5) Clear the screen
    send_stream(ESC + FF)

if __name__ == '__main__':
    main()
