## WIP
This plugin is a work in progress, with a bunch of known bugs. 

## Hyprlift

Hyprland plugin that creates an effect of a focused window being "lifted" (or as MaterialDesign refers to it -- elevated) over other windows. This is acheived by sizing down all other windows and (potentially) modifying shadow of a "focused" window. 
[official Hyprfocus](https://github.com/hyprwm/hyprland-plugins/tree/main/hyprfocus) plugin was used as a template.

## Install

No hyprpm support yet. 

If you want to try -- simplest way would be to use `nix build && hyprctl plugin load $(pwd)/result/lib/libhyprlift.so`

Or set up your own env as per [hyprland wiki](https://wiki.hypr.land/Plugins/Development/Getting-Started/)
