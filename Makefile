all:
	gcc -o wmemulator agent.c wmemulator.c wiimote.c input.c wm_crypto.c wm_reports.c -lbluetooth -lSDL -lpthread -lm `pkg-config --cflags dbus-1` -ldbus-1 -Wall
