#!/usr/bin/env python3
# led_video_wall.py
# Simple helper class for controlling the LEDVW server via /tmp/led-cmd

class LedVideoWall:
    CMD_FILE = "/tmp/led-cmd"

    # -----------------------------
    # Internal helper
    # -----------------------------
    @staticmethod
    def appendToFile(ledvw_port, command: str) -> bool:
        """
        Write a command to the LED command file.
        Returns True if successful, False if the file could not be opened or written.
        """
        try:
            with open(LedVideoWall.CMD_FILE + "-" + str(ledvw_port), "a") as fp:
                fp.write(command + "\n")
            return True
        except (FileNotFoundError, OSError):
            return False

    # -----------------------------
    # BASIC COMMANDS
    # -----------------------------
    @staticmethod
    def add_image(ledvw_port, element_id, filepath, x, y):
        return LedVideoWall.appendToFile(ledvw_port, f"add image {element_id} {filepath} {x} {y}")

    @staticmethod
    def move(ledvw_port, element_id, x, y):
        return LedVideoWall.appendToFile(ledvw_port, f"move {element_id} {x} {y}")

    @staticmethod
    def remove(ledvw_port, element_id):
        return LedVideoWall.appendToFile(ledvw_port, f"remove {element_id}")

    # -----------------------------
    # TEXT COMMANDS
    # -----------------------------
    @staticmethod
    def set_text(ledvw_port, element_id, text):
        return LedVideoWall.appendToFile(ledvw_port, f"set_text {element_id} {text}")

    @staticmethod
    def set_font_size(ledvw_port, element_id, size):
        return LedVideoWall.appendToFile(ledvw_port, f"set_font_size {element_id} {size}")

    @staticmethod
    def set_font(ledvw_port, element_id, font_path):
        return LedVideoWall.appendToFile(ledvw_port, f"set_font {element_id} {font_path}")

    @staticmethod
    def set_font_color(ledvw_port, element_id, b, g, r):
        return LedVideoWall.appendToFile(ledvw_port, f"set_font_color {element_id} {b} {g} {r}")

    # -----------------------------
    # CONTROL COMMANDS
    # -----------------------------
    @staticmethod
    def pause(ledvw_port):
        return LedVideoWall.appendToFile(ledvw_port, "pause")

    @staticmethod
    def resume(ledvw_port):
        return LedVideoWall.appendToFile(ledvw_port, "resume")

    @staticmethod
    def quit(ledvw_port):
        return LedVideoWall.appendToFile(ledvw_port, "quit")
