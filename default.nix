{ pkgs, ... }:

pkgs.stdenv.mkDerivation rec {
  pname = "sinter";
  version = "0.0.1";

  name = "${pname}-${version}";
  src = builtins.fetchGit {
    name = "sinter";
    url = ./.;
  };

  buildInputs = builtins.attrValues {
    inherit (pkgs) fuse3 libcap;
  };
  nativeBuildInputs = builtins.attrValues {
    inherit (pkgs) meson ninja moreutils coreutils;
  };
  builder = builtins.toFile "builder.sh" ''
    source $stdenv/setup
    cd $src
    meson setup /build
    meson compile -C /build
    cp -r /build $out
  '';

}
