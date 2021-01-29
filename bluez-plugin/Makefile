BLUEZ_DIR=../bluez-4.101

all:
	gcc -Wall -DHAVE_CONFIG_H -g -c -fvisibility=hidden -fPIC -o wmemu.o wmemu.c -I"$(BLUEZ_DIR)" `pkg-config --cflags --libs glib-2.0` `pkg-config --cflags dbus-1` -ldbus-1
	gcc -Wl,-E -shared -o wmemu.so wmemu.o -ldbus-1

