ifeq ($(origin CUSTOM_BUILD),undefined)
LBLUETOOTH=-lbluetooth
CFLAGS=
else
BLUEZ_DIST=./bluez-4.101/dist
LBLUETOOTH=-L"$(BLUEZ_DIST)/lib" -I"$(BLUEZ_DIST)/include" -Wl,-rpath="$(BLUEZ_DIST)/lib"
CFLAGS=-D SDP_SERVER
endif
LDBUS=`pkg-config --cflags dbus-1` -ldbus-1

all: wmemulator packedtest
clean:
	rm wmemulator packedtest
wmemulator: wmemulator.c wiimote.c input.c wm_crypto.c wm_reports.c wm_print.c sdp.c oui.c bdaddr.c device_setup.c
	gcc $(CFLAGS) -o wmemulator wmemulator.c wiimote.c input.c wm_crypto.c wm_reports.c wm_print.c sdp.c oui.c bdaddr.c device_setup.c $(LBLUETOOTH) -lSDL -lpthread -lm $(LDBUS) -Wall
packedtest: packedtest.c
	gcc -o packedtest packedtest.c
