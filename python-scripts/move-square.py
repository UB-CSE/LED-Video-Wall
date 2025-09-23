#!/usr/bin/env python3
import time

try:
    while True:

        with open("/tmp/led-cmd", "w") as f:
            f.write("move 1 10 0\n")  
            time.sleep(1)  
        
        with open("/tmp/led-cmd", "w") as f:
           f.write("move 1 10 10\n")
           time.sleep(1)

        with open("/tmp/led-cmd", "w") as f:
           f.write("move 1 0 10\n")
           time.sleep(1)

        with open("/tmp/led-cmd", "w") as f:
           f.write("move 1 0 0\n")
           time.sleep(1)

except KeyboardInterrupt:
    print("\nUser manually stopped")
    
