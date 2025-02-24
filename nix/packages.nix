{ pkgs }:
let
  opencvGtk = pkgs.opencv.override {
    enableGtk2 = true;
    enableGtk3 = true;
  };

  serverBuild = pkgs.stdenv.mkDerivation {
    name = "led-wall-server";
    src = ../server;

    nativeBuildInputs = with pkgs; [
      cmake
      ninja
      pkg-config
    ];

    buildInputs = with pkgs; [
      opencvGtk
      gtk2
      gtk3
      yaml-cpp
      httplib
      libjpeg
      libpng
      xorg.libX11
      xorg.libXext
      xorg.libXrender
    ];

    configurePhase = ''
      cmake -B build -S . \
        -DCMAKE_BUILD_TYPE=Release \
        -GNinja
    '';

    buildPhase = ''
      cmake --build build
    '';

    installPhase = ''
      mkdir -p $out/bin
      cp build/* $out/bin/ 2>/dev/null || true
      cp *.jpg $out/bin/ 2>/dev/null || true
    '';

    shellHook = ''
      export PKG_CONFIG_PATH="${pkgs.opencv}/lib/pkgconfig:${pkgs.yaml-cpp}/lib/pkgconfig:$PKG_CONFIG_PATH"
      export CMAKE_PREFIX_PATH="${pkgs.opencv}/lib/cmake:${pkgs.yaml-cpp}/lib/cmake:$CMAKE_PREFIX_PATH"
      export LD_LIBRARY_PATH="${pkgs.opencv}/lib:${pkgs.yaml-cpp}/lib:$LD_LIBRARY_PATH"
    '';
  };

  clientFirmware = pkgs.stdenv.mkDerivation {
    pname = "led-firmware";
    version = "1.0";
    src = ../client;

    nativeBuildInputs = with pkgs; [
      arduino-cli-wrapped
      python3
      curl
    ];

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
  default = clientFirmware;
  server = serverBuild;
}
