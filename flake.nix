{
  description = "Easymotion, for hyprland";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
    hyprland.url = "git+https://github.com/hyprwm/Hyprland?submodules=1";
  };

  outputs = {
    self,
    nixpkgs,
    ...
  } @ inputs: let
    forAllSystems = function:
      nixpkgs.lib.genAttrs [
        "x86_64-linux"
      ] (system: function nixpkgs.legacyPackages.${system});
  in {
    packages = forAllSystems (pkgs: {
      default = self.packages.${pkgs.system}.hyprland-easymotion;
      hyprland-easymotion = pkgs.stdenvNoCC.mkDerivation rec {
        name = "hyprland-easymotion";
        pname = name;
        src = ./.;
        nativeBuildInputs = inputs.hyprland.packages.${pkgs.system}.hyprland.nativeBuildInputs ++ [inputs.hyprland.packages.${pkgs.system}.hyprland pkgs.gcc13];
        buildInputs = inputs.hyprland.packages.${pkgs.system}.hyprland.buildInputs;

        dontUseCmakeConfigure = true;
        dontUseMesonConfigure = true;
        dontUseNinjaBuild = true;
        dontUseNinjaInstall = true;

        installPhase = ''
        runHook preInstall

        mkdir -p "$out/lib"
        cp -r out/* "$out/lib/lib${name}.so"
        runHook postInstall
        '';
      };
    });
  };
}
