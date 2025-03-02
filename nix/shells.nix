{ pkgs }:
let
  arduino-cli-wrapped = pkgs.wrapArduinoCLI {
    libraries = with pkgs.arduinoLibraries; [
      (arduino-nix.latestVersion pkgs.arduinoLibraries."package_esp32_index")
    ];
  };
in
{
  default = pkgs.mkShell {
    buildInputs = pkgs.packages.server.buildInputs ++ pkgs.packages.server.nativeBuildInputs;
    shellHook = pkgs.packages.server.shellHook;
  };

  hardware = pkgs.mkShell {
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
}
