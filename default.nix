{ lib
, stdenv
, meson
, ninja
, hyprland
, version ? "git"
}:
stdenv.mkDerivation rec {
  pname = "hypreasymotion";
  inherit version;
  src = ./.;

  nativeBuildInputs = lib.filter (input: !lib.elem input [ meson ninja ]) hyprland.nativeBuildInputs;

  buildInputs = [ hyprland ] ++ hyprland.buildInputs;

  buildPhase = ''
    make all
  '';

  installPhase = ''
    mkdir -p $out/lib/
    cp ${pname}.so $out/lib/lib${pname}.so
  '';

  meta = with lib; {
    homepage = "https://github.com/zakk4223/hyprland-easymotion";
    description = "Easymotion, for hyprland";
    license = licenses.bsd3;
    platforms = platforms.linux;
  };
}
