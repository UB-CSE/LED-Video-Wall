{ arduino-nix, arduino-index }:
[
  (arduino-nix.overlay)
  (arduino-nix.mkArduinoPackageOverlay (arduino-index + "/index/package_index.json"))
  (arduino-nix.mkArduinoPackageOverlay (arduino-index + "/index/package_esp32_index.json"))
  (arduino-nix.mkArduinoLibraryOverlay (arduino-index + "/index/library_index.json"))
]
