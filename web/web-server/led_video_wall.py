#!/usr/bin/env python3
# led_video_wall.py
# Simple helper class for controlling the LEDVW server via /tmp/led-cmd

class LedVideoWall:
    CMD_FILE = "/tmp/led-cmd"

    # -----------------------------
    # Internal helper
    # -----------------------------
    @staticmethod
    def appendToFile(command: str) -> bool:
        """
        Write a command to the LED command file.
        Returns True if successful, False if the file could not be opened or written.
        """
        try:
            with open(LedVideoWall.CMD_FILE, "a") as fp:
                fp.write(command + "\n")
            return True
        except (FileNotFoundError, OSError):
            return False

    # -----------------------------
    # BASIC COMMANDS
    # -----------------------------
    @staticmethod
    def add_image(element_id, filepath, x, y):
        return LedVideoWall.appendToFile(f"add image {element_id} {filepath} {x} {y}")

    @staticmethod
    def move(element_id, x, y):
        return LedVideoWall.appendToFile(f"move {element_id} {x} {y}")

    @staticmethod
    def remove(element_id):
        return LedVideoWall.appendToFile(f"remove {element_id}")

    # -----------------------------
    # TEXT COMMANDS
    # -----------------------------
    @staticmethod
    def set_text(element_id, text):
        return LedVideoWall.appendToFile(f"set_text {element_id} {text}")

    @staticmethod
    def set_font_size(element_id, size):
        return LedVideoWall.appendToFile(f"set_font_size {element_id} {size}")

    @staticmethod
    def set_font(element_id, font_path):
        return LedVideoWall.appendToFile(f"set_font {element_id} {font_path}")

    @staticmethod
    def set_font_color(element_id, b, g, r):
        return LedVideoWall.appendToFile(f"set_font_color {element_id} {b} {g} {r}")

    # -----------------------------
    # CONTROL COMMANDS
    # -----------------------------
    @staticmethod
    def pause():
        return LedVideoWall.appendToFile("pause")

    @staticmethod
    def resume():
        return LedVideoWall.appendToFile("resume")

    @staticmethod
    def quit():
        return LedVideoWall.appendToFile("quit")
