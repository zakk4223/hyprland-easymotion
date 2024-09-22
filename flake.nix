# based off of split-monitor-workspaces flake.nix
# https://github.com/Duckonaut/split-monitor-workspaces/blob/main/flake.nix
{
  inputs = {
    hyprland.url = "github:hyprwm/Hyprland";
    nix-filter.url = "github:numtide/nix-filter";
  };

  outputs = {
    self,
    hyprland,
    nix-filter,
    ...
  }: let
    inherit (hyprland.inputs) nixpkgs;
    forHyprlandSystems = fn: nixpkgs.lib.genAttrs (builtins.attrNames hyprland.packages) (system: fn system nixpkgs.legacyPackages.${system});
  in {
    packages = forHyprlandSystems (system: pkgs: rec {
      hyprland-easymotion = pkgs.gcc13Stdenv.mkDerivation {
        pname = "hyprland-easymotion";
        version = "1.0.0";
        src = nix-filter.lib {
          root = ./.;
          include = [
            "src"
            "include"
            ./Makefile
          ];
        };

        # allow overriding xwayland support
        BUILT_WITH_NOXWAYLAND = false;

        nativeBuildInputs = with pkgs; [ninja pkg-config];

        buildInputs = with pkgs;
          [
            hyprland.packages.${system}.hyprland.dev
            pango
            cairo
          ]
          ++ hyprland.packages.${system}.hyprland.buildInputs;

        meta = with pkgs.lib; {
          homepage = "https://github.com/zakk4223/hyprland-easymotion";
          description = "Easymotion, for hyprland";
          license = licenses.bsd3;
          platforms = platforms.linux;
        };
      };

      default = hyprland-easymotion;
    });

    devShells = forHyprlandSystems (system: pkgs: {
      default = pkgs.mkShell {
        name = "hyprland-easymotion";

        nativeBuildInputs = with pkgs; [
          clang-tools_16
          bear
        ];

        inputsFrom = [self.packages.${system}.hyprland-easymotion];
      };
    });
  };
}
