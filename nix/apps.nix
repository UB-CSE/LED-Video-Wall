{
  pkgs,
  flake-utils,
  packages,
}:
{
  default = flake-utils.lib.mkApp {
    drv = pkgs.writeShellScriptBin "flashFirmware" ''
      echo "tbd!"
    '';
  };
}
