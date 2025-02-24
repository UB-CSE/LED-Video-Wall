{
  pkgs,
  flake-utils,
  packages,
}:
{
  default = flake-utils.lib.mkApp {
    drv = pkgs.writeShellScriptBin "flashFirmware" ''
      set -e
      PORT="/dev/ttyUSB0"
      BIN_FILE="${packages.default}/firmware.bin"

      if [ ! -f "$BIN_FILE" ]; then
        echo "Firmware binary not found!"
        exit 1
      fi

      echo "Flashing firmware..."
      arduino-cli upload -p $PORT --fqbn esp32:esp32:wroom32 --input-file $BIN_FILE
      echo "Flashing complete!"
    '';
  };

  clean = flake-utils.lib.mkApp {
    drv = pkgs.writeShellScriptBin "clean" ''
      rm -rf build/
    '';
  };
}
