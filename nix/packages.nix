{ pkgs }:
let
  serverBuild = pkgs.stdenv.mkDerivation {
    name = "led-wall-server";
    src = ../server;

    nativeBuildInputs = with pkgs; [
      pkg-config
      gnumake
    ];

    buildInputs = with pkgs; [
      freetype
      opencv
      yaml-cpp
      libjpeg
      libpng
      xorg.libX11
      xorg.libXext
      xorg.libXrender
    ];
  };
in
{
  default = serverBuild;
  server = serverBuild;
}
