# hyprEasymotion

Plugin to enable 'easymotion' navigation. Inspired by Xmonad easymotion (which is in turn inspired by vim-easymotion)

https://github.com/zakk4223/hyprland-easymotion/assets/22642/9382b23e-efbd-466c-b071-127a53f0c48b

# Configuration

Easymotion is basically a single dispatcher that brings up window labels and then allows you to execute a user-defined command when one of those labels is typed.

`bind = SUPER, z, easymotion, action:hyprctl dispatch focuswindow address:{}`

This bind will bring up easymotion with SUPER-z. Once you select a window the window
will focus. If you want to change the command, the selected window's address is substituted where "{}" occurs.

You can configure the appearance of the labels. Defaults are as follows:

```
plugin {
  easymotion {
    #font size of the text
    textsize=15

    #color of the text, takes standard hyprland color format
    textcolor=rgba(ffffffff)

    #background color of the label box. alpha is respected
    bgcolor=rgba(000000ff)

    #font to use for the label. This is passed directly to the pango font description
    textfont=Sans

    #padding around the text (inside the label box) size in pixels, adjusted for
    #monitor scaling. This is the same format as hyprland's gapsin/gapsout workspace layout rule
    #example: textpadding=2 5 5 2 (spaces not commas)
    textpadding=0

    #size of the border around the label box. A border size of zero disables border rendering.
    bordersize=0

    #color of the border. takes the same format as hyprland's border (so it can be a gradient)
    bordercolor=rgba(ffffffff)

    #rounded corners? Same as hyprland's 'decoration:rounding' config
    rounding=0

    #which keys to use for labeling windows
    motionkeys=abcdefghijklmnopqrstuvwxyz1234567890
  }
}
```

Every one of these variables is also settable via the dispatcher, so you can create multiple dispatchers that look different based on function.

`bind = SUPER, z, easymotion, bgcolor:rgba(ff0000ff),bordersize:5,action:hyprctl dispatch closewindow address:{}`

# Installing

## Hyprpm, Hyprland's official plugin manager (recommended)

1. Run `hyprpm add https://github.com/zakk4223/hyprland-easymotion` and wait for hyprpm to build the plugin.
2. Run `hyprpm enable hyprEasymotion`

## NixOS with home—manager

<details>
<summary><b>Here is an example config</b></summary>

```nix
# flake.nix
{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

    home-manager = {
      url = "github:nix-community/home-manager";
      inputs.nixpkgs.follows = "nixpkgs";
    };

    hyprland.url = "github:hyprwm/Hyprland";

    hypreasymotion = {
      url = "github:zakk4223/hyprland-easymotion";
      inputs.hyprland.follows = "hyprland";
    };
  };

  outputs = { nixpkgs, home-manager, hyprland, hypreasymotion, ... }:
    let
      system = "x86_64-linux";
      pkgs = nixpkgs.legacyPackages.${system};
    in
    {
      homeConfigurations."user@hostname" = home-manager.lib.homeManagerConfiguration {
        pkgs = nixpkgs.legacyPackages.x86_64-linux;

        modules = [
          hyprland.homeManagerModules.default
          {
            wayland.windowManager.hyprland = {
              enable = true;
              package = hyprland.packages."${pkgs.system}".hyprland;
              plugins = [
                hypreasymotion.packages.${pkgs.system}.hypreasymotion
              ];
              extraConfig = ''
                plugin {
                    easymotion {
                    }
                }
              '' + ''
                # your othor config
              '';
            };
          }
          # ...
        ];
      };
    };
}
```

</details>

# TODO

- [ ] Blur?
- [ ] Allow multi-letter labels?
- [ ] Fixed/static label box sizing
- [ ] Location of label in window (edges etc)
- [ ] Auto label placement that tries to avoid being occluded
