#include <Arduino.h>

PROGMEM const unsigned char pic_bell[] = {
	B00000000,B00000000,
	B10000000,B00000000,
	B00000000,B00000000,
	B11000000,B00000001,
	B00110000,B00000110,
	B01001000,B00001000,
	B00100100,B00010000,
	B00010100,B00010000,
	B00010100,B00010000,
	B00010100,B00010000,
	B00010100,B00010000,
	B00010100,B00010000,
	B00010100,B00010000,
	B00010100,B00010000,
	B00010100,B00010000,
	B00010100,B00010000,
	B11010110,B00110111,
	B00000001,B01000000,
	B11111110,B00111111,
	B00000000,B00000000,
	B11000000,B00000001,
	B11000000,B00000001,
	B10000000,B00000000,
	B00000000,B00000000
};

PROGMEM const unsigned char pic_wifi[] = {
	B00000000,
	B00111100,
	B01000010,
	B10011001,
	B10100101,
	B00000000,
	B00011000,
	B00000000
};

PROGMEM const unsigned char pic_active_bell[] = {
	B00000000,
	B00011000,
	B00100100,
	B01011010,
	B01011010,
	B00100100,
	B00011000,
	B00000000
};