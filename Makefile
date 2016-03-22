all:
	gcc -g -o wmemulator wmemulator.c wiimote.c input.c wm_crypto.c wm_reports.c wm_print.c -lbluetooth -lSDL -lpthread -lm `pkg-config --cflags dbus-1` -ldbus-1 -Wall
	gcc -o bdaddr bdaddr.c oui.c -lbluetooth
	gcc -o sdptool sdptool.c -lbluetooth
	gcc -o packedtest packedtest.c
