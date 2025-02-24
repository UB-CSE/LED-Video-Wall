{
  description = "LED Video Wall flake, providing extensible ways to deploy client and server updates.";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    arduino-nix.url = "github:bouk/arduino-nix";
    arduino-index = {
      url = "github:bouk/arduino-indexes";
      flake = false;
    };
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
      arduino-nix,
      arduino-index,
      ...
    }:
    let
      overlays = [
        (arduino-nix.overlay)
        (arduino-nix.mkArduinoPackageOverlay (arduino-index + "/index/package_index.json"))
        (arduino-nix.mkArduinoPackageOverlay (arduino-index + "/index/package_esp32_index.json"))
        (arduino-nix.mkArduinoLibraryOverlay (arduino-index + "/index/library_index.json"))
      ];
    in
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs { inherit system overlays; };

        arduino-cli-wrapped = pkgs.wrapArduinoCLI {
          libraries = with pkgs.arduinoLibraries; [
            (arduino-nix.latestVersion pkgs.arduinoLibraries."package_esp32_index")
          ];
        };

        hardwareShell = pkgs.mkShell {
          buildInputs = with pkgs; [
            arduino-ide
            arduino-cli
            esptool
            python3
          ];

          shellHook = ''
            arduino-cli core update-index
            arduino-cli core install esp32:esp32
            echo "ESP32 toolchain ready"
          '';
        };

        serverShell = pkgs.mkShell {
          buildInputs = with pkgs; [
            cmake
            ninja
            pkg-config

            gcc
            clang
            clang-tools

            # libraries
            opencv
            yaml-cpp
          ];

          shellHook = ''
            echo "C++ Development Environment Loaded"
            echo "OpenCV Version: ${pkgs.opencv.version}"
            echo "YAML-CPP Version: ${pkgs.yaml-cpp.version}"
          '';
        };

        clientFirmware = pkgs.stdenv.mkDerivation {
          pname = "led-firmware";
          version = "1.0";

          src = ./client;

          nativeBuildInputs = with pkgs; [
            arduino-core
            arduino-cli-wrapped
            python3
            curl
          ];

          buildInputs = [ ];

          buildPhase = ''
            echo "Compiling ESP32 firmware..."
            mkdir -p build
            cp -r $src build/client
            arduino-cli compile --fqbn esp32:esp32:esp32 --output-dir $out build/client
          '';

          installPhase = ''
            echo "Installing compiled .bin..."
            mv $out/*.bin $out/firmware.bin
          '';
        };
      in
      {
        nixConfig = {
          substituters = [ ]; # disable substituters
        };

        packages.default = clientFirmware;
        # TODO: make a package for server stuff when its done.

        devShells.default = serverShell;
        devShells.hardware = hardwareShell;

        apps.default = flake-utils.lib.mkApp {
          drv = pkgs.writeShellScriptBin "flashFirmware" ''
            set -e
            PORT="/dev/ttyUSB0"
            BIN_FILE="${clientFirmware}/firmware.bin"

            if [ ! -f "$BIN_FILE" ]; then
              echo "Firmware binary not found!"
              exit 1
            fi

            echo "Flashing firmware..."
            arduino-cli upload -p $PORT --fqbn esp32:esp32:wroom32 --input-file $BIN_FILE
            echo "Flashing complete!"
          '';
        };
      }
    );
}
