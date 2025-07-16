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

    #enable blur. The bgcolor alpha must be at least semi-transparent.
    blur=0

    #Set blur alpha value. Blur must be enabled (float value)
    blurA=1.0

    #Set xray. Blur must be enabled
    xray=0

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

    #what to do if a window is fullscreen
    #none: nothing. (easymotion label won't be displayed on that window)
    #toggle: take the window out of fullscreen entirely.
    #maximize: convert the window to maximized.
    #windows are restored to fullscreen after easymotion is exited/selected
    fullscreen_action=none

    #which keys to use for labeling windows
    motionkeys=abcdefghijklmnopqrstuvwxyz1234567890

    #if a monitor has a focused special workspace, only put easymotion labels on the windows in the special workspace
    only_special = true
  }
}
```

Every one of these variables is also settable via the dispatcher, so you can create multiple dispatchers that look different based on function.

`bind = SUPER, z, easymotion, bgcolor:rgba(ff0000ff),bordersize:5,action:hyprctl dispatch closewindow address:{}`

# Installing

## Hyprpm, Hyprland's official plugin manager (recommended)
1. Run `hyprpm add https://github.com/zakk4223/hyprland-easymotion` and wait for hyprpm to build the plugin.
2. Run `hyprpm enable hyprEasymotion`

## NixOS (Flakes)
Please note, you should *also have hyprland as a flake input*.
Add this repo to your flake inputs:
```nix
inputs = {
  hyprland.url = "github:hyprwm/Hyprland";

  hyprland-easymotion = {
    url = "github:zakk4223/hyprland-easymotion";
    inputs.hyprland.follows = "hyprland";
  };
  # ...
};
outputs = { self, hyprland, hyprland-easymotion, ... } @ inputs:
  # ...
```
Add the plugin to your Hyprland Home Manager config:
```nix
wayland.windowManager.hyprland = {
  plugins = [
    inputs.hyprland-easymotion.packages.${pkgs.system}.hyprland-easymotion
  ];
  # ...
};
```
# TODO
- [x] Blur?
- [ ] Allow multi-letter labels?
- [ ] Fixed/static label box sizing
- [ ] Location of label in window (edges etc)
- [ ] Auto label placement that tries to avoid being occluded
