{
  description = "Easymotion, for hyprland";

  inputs = {
    hyprland.url = "github:hyprwm/Hyprland";
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
  };

  outputs = {
    self,
    nixpkgs,
    hyprland,
    ...
  } @ inputs: let
    forAllSystems = function:
      nixpkgs.lib.genAttrs [
        "x86_64-linux"
      ] (system: function nixpkgs.legacyPackages.${system});
  in {
    packages = forAllSystems (pkgs: {
      default = self.packages.${pkgs.system}.hyprland-easymotion;
      hyprland-easymotion = hyprland.packages.${pkgs.system}.hyprland.stdenv.mkDerivation rec {
        name = "hyprland-easymotion";
        pname = name;
        src = ./.;

        dontUseNinjaBuild = true;
        dontUseNinjaInstall = true;
        dontUseCmakeConfigure = true;
        dontUseMesonConfigure = true;

        nativeBuildInputs = [
          hyprland.packages.${pkgs.system}.hyprland.dev
        ] ++ inputs.hyprland.packages.${pkgs.system}.hyprland.nativeBuildInputs;
        buildInputs = inputs.hyprland.packages.${pkgs.system}.hyprland.buildInputs;

        installPhase = ''
          runHook preInstall

          mkdir -p "$out/lib"
          cp -r ./hypreasymotion.so "$out/lib/libhyprland-easymotion.so"
          runHook postInstall
        '';
      };
    });
  };
}
