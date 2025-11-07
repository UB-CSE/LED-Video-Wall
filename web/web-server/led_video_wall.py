#!/usr/bin/env python3
# led_video_wall.py
# Simple helper class for controlling the LEDVW server via /tmp/led-cmd

class LedVideoWall:
    CMD_FILE = "/tmp/led-cmd"

    # -----------------------------
    # BASIC COMMANDS
    # -----------------------------
    @staticmethod
    def add_image(element_id, filepath, x, y):
        with open(LedVideoWall.CMD_FILE, "a") as fp:
            fp.write(f"add image {element_id} {filepath} {x} {y}\n")

    @staticmethod
    def move(element_id, x, y):
        with open(LedVideoWall.CMD_FILE, "a") as fp:
            fp.write(f"move {element_id} {x} {y}\n")

    @staticmethod
    def remove(element_id):
        with open(LedVideoWall.CMD_FILE, "a") as fp:
            fp.write(f"remove {element_id}\n")
    
    @staticmethod
    def scale(element_id, scale):
        with open(LedVideoWall.CMD_FILE, "a") as fp:
            fp.write(f"scale {element_id} {scale}\n")

    # -----------------------------
    # TEXT COMMANDS
    # -----------------------------
    @staticmethod
    def set_text(element_id, text):
        with open(LedVideoWall.CMD_FILE, "a") as fp:
            fp.write(f"set_text {element_id} {text}\n")

    @staticmethod
    def set_font_size(element_id, size):
        with open(LedVideoWall.CMD_FILE, "a") as fp:
            fp.write(f"set_font_size {element_id} {size}\n")

    @staticmethod
    def set_font(element_id, font_path):
        with open(LedVideoWall.CMD_FILE, "a") as fp:
            fp.write(f"set_font {element_id} {font_path}\n")

    @staticmethod
    def set_font_color(element_id, b, g, r):
        with open(LedVideoWall.CMD_FILE, "a") as fp:
            fp.write(f"set_font_color {element_id} {b} {g} {r}\n")

    # -----------------------------
    # CONTROL COMMANDS
    # -----------------------------
    @staticmethod
    def pause():
        with open(LedVideoWall.CMD_FILE, "a") as fp:
            fp.write("pause\n")

    @staticmethod
    def resume():
        with open(LedVideoWall.CMD_FILE, "a") as fp:
            fp.write("resume\n")

    @staticmethod
    def quit():
        with open(LedVideoWall.CMD_FILE, "a") as fp:
            fp.write("quit\n")
