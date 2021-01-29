#!/bin/sh

# download bluez-4.101 dist
wget https://www.kernel.org/pub/linux/bluetooth/bluez-4.101.tar.xz
tar -xf ./bluez-4.101.tar.xz
rm ./bluez-4.101.tar.xz

# apply patch (to disable sdp server) and build bluez
cd bluez-4.101
mkdir dist
DIST=$(pwd)/dist
cp ../bluez-disable-sdp.patch .
patch -p0 < bluez-disable-sdp.patch
./configure --prefix=$DIST --with-systemdunitdir=$DIST/system --disable-service --disable-audio --disable-input --disable-serial
make && make install

# build bluez-plugin
cd ../bluez-plugin
make
cp ./wmemu.so $DIST/lib/bluetooth/plugins
cd ..

# build emulator
CUSTOM_BUILD=1
make