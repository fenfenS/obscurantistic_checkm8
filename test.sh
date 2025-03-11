cc -Wall -Wextra -Wpedantic checkm8.c -o cm8 $(pkg-config --libs --cflags libusb-1.0) -lcrypto -pthread -ldl -Os -g
