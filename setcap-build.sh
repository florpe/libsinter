#!/usr/bin/env bash

set -euxo pipefail

sudo umount mnt || true
sudo rm -rf run &&
    nix-build -E 'with import <nixpkgs> {} ; callPackage ./default.nix {}' &&
    cp -R result/ run &&
    sudo setcap CAP_SYS_ADMIN+ep ./run/sinter
./run/sinter -h
