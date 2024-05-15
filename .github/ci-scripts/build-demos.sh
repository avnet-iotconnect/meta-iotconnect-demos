#!/bin/bash

ssh-add -L
sudo chown user:user /src/ -R
cd /src/
ssh-keyscan -p 9418 -t rsa msc-git02.msc-ge.com >> ~/.ssh/known_hosts
git clone ssh://gitolite@msc-git02.msc-ge.com:9418/msc_ol99/msc-ldk
cd msc-ldk/
git checkout v1.11.0
pip3 install sphinx
sudo apt update && sudo apt install -y parted mtools
./setup.py --bsp=01047
cp -r /src/yocto-layers/ /src/msc-ldk/sources/meta-iotconnect-demos/
git clone git@github.com:avnet-iotconnect/meta-iotconnect.git /src/msc-ldk/sources/meta-iotconnect/-b kirkstone

source sources/yocto.git/oe-init-build-env build/01047
bitbake-layers add-layer ../../sources/meta-iotconnect/
bitbake-layers add-layer ../../sources/meta-iotconnect-demos/

cat conf/local.conf
bitbake core-image-base
