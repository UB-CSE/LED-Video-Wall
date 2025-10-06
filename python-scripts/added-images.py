#!/usr/bin/env python3
import time

cmd_file="/tmp/led-cmd"

try:
    with open(cmd_file,"a") as fp:
        fp.write("add image 2 #insert filepath here# 0 0\n")
    time.sleep(1)
    with open(cmd_file, "a") as fp:
        fp.write("add image 3 #insert filepath here#  0 0\n")
    while True:
        with open(cmd_file, "a") as fp:
            fp.write("move 3 1 1\n")
        time.sleep(1)
        
except KeyboardInterrupt:
    print("\nUser manually stopped")