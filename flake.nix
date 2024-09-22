{
  inputs = {
    hyprland.url = "github:hyprwm/Hyprland";
    nix-filter.url = "github:numtide/nix-filter";
  };
  outputs = { self, hyprland, nix-filter, ... }:
    let
      inherit (hyprland.inputs) nixpkgs;
      forHyprlandSystems = fn: nixpkgs.lib.genAttrs (builtins.attrNames hyprland.packages) (system: fn system nixpkgs.legacyPackages.${system});
    in
    {
      packages = forHyprlandSystems
        (system: pkgs: rec {
          hyprfocus = pkgs.gcc13Stdenv.mkDerivation {
            pname = "hyprland-easymotion";
            version = "0.1";
            src = nix-filter.lib {
              root = ./.;
              include = [
                "src"
                ./Makefile
              ];
            };


            nativeBuildInputs = with pkgs; [ pkg-config ];

            buildInputs = with pkgs; [
              hyprland.packages.${system}.hyprland.dev
            ]
            ++ hyprland.packages.${system}.hyprland.buildInputs;

            installPhase = ''
              mkdir -p $out/lib
              install ./hyprland-easymotion.so $out/lib/libhyprland-easymotion.so
            '';

            meta = with pkgs.lib; {
              homepage = "https://github.com/zakk4223/hyprland-easymotion";
              description = "Easymotion, for hyprland";
              license = licenses.bsd3;
              platforms = platforms.linux;
            };


          };
          default = hyprland-easymotion;
        });
    };
}
