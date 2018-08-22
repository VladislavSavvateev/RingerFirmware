#include <Arduino.h>

#include "pics.cpp"
#include "str_defines.cpp"

#include "SSD1306.h"
#include <EEPROM.h>

#include "TimeLib.h"
#include <Wire.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>

#define RINGER_PIN D4
#define DS1307_CTRL_ID 0x68

SSD1306 lcd(0x3C, D3, D5);

WiFiServer server(1488);
WiFiClient client;

uint8_t clearAllRings();
uint8_t enableRelay();
uint8_t disableRelay();
void drawLogMessage(String message);
void drawMainUI();
void drawHeaderTitle(String title);
void drawWifiIcon(bool visible);
void drawActiveBellIcon(bool visible);
void drawClock();
void drawInfos();
uint16_t getNextRing();
uint8_t getCountOfRings();
void getDateDs1307();
void setDateDs1307();
byte decToBcd(byte val);
byte bcdToDec(byte val);

void turnRinger(bool enabled);

uint16_t nextRing = 0x0000;
int currentDay = 0;
int lastSecond = 0;

void setup() {
	ESP.eraseConfig();

	turnRinger(false);

	// lcd init
	lcd.init();
	lcd.flipScreenVertically();
	lcd.setContrast(255);
	lcd.drawXbm(56, 20, 16, 24, pic_bell);
	lcd.setFont(ArialMT_Plain_16);
	lcd.drawString((128 - lcd.getStringWidth(TITLE)) / 2, 2, TITLE);
	lcd.setFont(ArialMT_Plain_10);
	lcd.display();

	delay(1000);

	// time init
	drawLogMessage(INIT_TIME);
	getDateDs1307();

	// eeprom init
	drawLogMessage(INIT_EEPROM);
	EEPROM.begin(1024);	

	// wifi init
	drawLogMessage(INIT_WIFI);
	WiFi.softAP(WIFI_SSID, WIFI_PASSWORD, 9, false, 16);

	// server init
	drawLogMessage(INIT_SERVER);
	server.begin();
	server.setNoDelay(true);

	drawMainUI();

	nextRing = getNextRing();
	currentDay = day();
	lastSecond = second();
}


void loop() {
	if (server.hasClient()) {
		drawWifiIcon(true);

		if (client.connected()) client.stopAll();
		client = server.available();
	}

	if (client.connected()) {
		if (client.available() > 0) {
			switch (client.read()) {
				case 1: {
					client.write(0x01);
					
					// get all rings
					int offset = 0;
					for (int i = 0; i < 7; i++) {
						uint8_t count = EEPROM.read(offset++);
						client.write(count);	// count of rings
						for (int k = 0; k < count; k++) {
							client.write(EEPROM.read(offset++));	// enabled
							client.write(EEPROM.read(offset++));	// hour
							client.write(EEPROM.read(offset++));	// minute
						}
					}
					break;
				} case 2:
					client.write(clearAllRings());
					break;
				case 3:
					client.write(enableRelay());
					break;
				case 4:
					client.write(disableRelay());
					break;
				case 5: {
					// set date
					while (client.available() < 7) yield();
					int hour = client.read();
					int minute = client.read();
					int second = client.read();
					int day = client.read();
					int month = client.read();
					int year = (client.read() << 8) + client.read();
					setTime(hour, minute, second, day, month, year);
					setDateDs1307();
					nextRing = getNextRing();

					client.write(0x01);
					break;
				}
				case 6: {
					int offset = 0;
					for (int i = 0; i < 7; i++) {
						while (client.available() < 1) yield();

						uint8_t count = client.read();
						EEPROM.write(offset++, count);

						while (client.available() < count * 3) yield();
						for (int k = 0; k < count; k++) {
							EEPROM.write(offset++, client.read());
							EEPROM.write(offset++, client.read());
							EEPROM.write(offset++, client.read());
						}
					}
					EEPROM.commit();
					nextRing = getNextRing();

					client.write(0x01);
					break;
				}
			}
			client.flush();
		}
	} else drawWifiIcon(false);

	drawClock();
	drawInfos();
	lcd.display();

	if ((nextRing >> 8) == hour() && (nextRing & 255) == minute()) {
		turnRinger(true);
		unsigned long now = millis();
		while (millis() - now < 5000) yield();
		turnRinger(false);
		nextRing = getNextRing();
	}
	if (currentDay != day()) {
		currentDay = day();
		nextRing = getNextRing();
	}
}
#pragma region COMMANDS
uint8_t clearAllRings() {
	for (int i = 0; i < 7; i++) 
		EEPROM.write(i, 0x00);
	EEPROM.commit();
	nextRing = getNextRing();

	return 0x01;
}
uint8_t enableRelay() {
	turnRinger(true);
	drawActiveBellIcon(true);

	return 0x01;
}
uint8_t disableRelay() {
	turnRinger(false);
	drawActiveBellIcon(false);

	return 0x01;
}
#pragma endregion
#pragma region DRAW METHODS
void drawLogMessage(String message) {
	lcd.setColor(BLACK);
	lcd.fillRect(0, 54, 128, 10);
	lcd.setColor(WHITE);

	lcd.drawString((128 - lcd.getStringWidth(message)) / 2, 54, message);
	lcd.display();
}
void drawMainUI() {
	lcd.clear();

	// draw dividers
	lcd.drawLine(0, 13, 128, 13);
	lcd.drawLine(0, 54, 128, 54); 

	drawHeaderTitle(TITLE_MAIN);
	drawClock();
	lcd.display();
}
void drawHeaderTitle(String title) {
	unsigned short stringWidth = lcd.getStringWidth(title); 
	lcd.drawString(37 + (91 - stringWidth) / 2, 0, title);
	lcd.drawLine(37, 5, 33 + (91 - stringWidth) / 2, 5);
	lcd.drawLine(41 + (91 - stringWidth) / 2 + stringWidth, 5, 128, 5);
}
void drawWifiIcon(bool visible) {
	if (visible) lcd.drawXbm(120, 56, 8, 8, pic_wifi);
	else {
		lcd.setColor(BLACK);
		lcd.fillRect(120, 56, 8, 8);
		lcd.setColor(WHITE);
	}
	lcd.display();
}
void drawActiveBellIcon(bool visible) {
	if (visible) lcd.drawXbm(112, 56, 8, 8, pic_active_bell);
	else {
		lcd.setColor(BLACK);
		lcd.fillRect(112, 56, 8, 8);
		lcd.setColor(WHITE);
	}
	lcd.display();
}
bool showDivider = true;
void drawClock() {
	lcd.setColor(BLACK);
	lcd.fillRect(0, 0, 37, 13);
	lcd.setColor(WHITE);

	int h = hour();
	int m = minute();

	lcd.drawString(0, 0, String(h / 10));
	lcd.drawString(8, 0, String(h % 10));
	if (showDivider) lcd.drawString(16, 0, ":");
	lcd.drawString(21, 0, String(m / 10));
	lcd.drawString(29, 0, String(m % 10));

	if (lastSecond != second()) {
		lastSecond = second();
		showDivider = !showDivider;
	}
}
void drawInfos() {
	lcd.setColor(BLACK);
	lcd.fillRect(0, 14, 128, 40);
	lcd.setColor(WHITE);

	int w = weekday(now()) - 1;
	lcd.drawString(0, 17, "D: " + String(w == 0 ? 7 : w));
	lcd.drawString(0, 27, "T: " + String(getCountOfRings()));

	lcd.setFont(ArialMT_Plain_16);
	String t;
	if (nextRing == 0xFFFF) t = "--:--";
	else {
		int h = nextRing >> 8;
		int m = nextRing & 255;
		t = (h < 10 ? "0" + String(h) : String(h)) + ":" + (m < 10 ? "0" + String(m) : String(m));
	}
	lcd.drawString(128 - lcd.getStringWidth(t), 38, t);

	int remainingMinutes = ((nextRing >> 8) * 60 + (nextRing & 255)) - (hour() * 60 + minute());
	lcd.setFont(ArialMT_Plain_24);
	if (nextRing == 0xFFFF) t = "--:--";
	else {
		int h = remainingMinutes / 60;
		int m = remainingMinutes % 60;
		t = (h < 10 ? "0" + String(h) : String(h)) + ":" + (m < 10 ? "0" + String(m) : String(m));
	}
	lcd.drawString(128 - lcd.getStringWidth(t), 16, t);
	lcd.setFont(ArialMT_Plain_10);
}
#pragma endregion
#pragma region RING METHODS
uint8_t getCountOfRings() {
	int w = weekday(now()) - 2;
	if (w == -1) w = 6; 
	int offset = 0;
	for (int i = 0; i < w; i++) 
		offset += EEPROM.read(offset) * 3 + 1;
	return EEPROM.read(offset);
}
uint16_t getNextRing() {
	int w = weekday(now()) - 2;
	if (w == -1) w = 6; 
	int offset = 0;
	for (int i = 0; i < w; i++) 
		offset += EEPROM.read(offset) * 3 + 1;
	int total = EEPROM.read(offset++);
	for (int i = 0; i < total; i++) {
		if (EEPROM.read(offset++) == 0x00) 
			offset += 2;
		else {
			uint8_t h = EEPROM.read(offset++);
			uint8_t m = EEPROM.read(offset++);
			int h_n = hour();
			int m_n = minute();

			if (h_n < h || (h_n == 23 && h == 0)) return (h << 8) + m;
			else if (h == h_n && m_n < m) return (h << 8) + m;	
		}
	}

	return 0xFFFF;
}
#pragma endregion
#pragma region CONTROL METHODS
void turnRinger(bool enabled) {
	pinMode(RINGER_PIN, enabled ? OUTPUT : INPUT);
}
#pragma endregion
#pragma region DS1307 METHODS
void getDateDs1307() {
	Wire.beginTransmission(DS1307_CTRL_ID); 
	Wire.write(byte(0x00));
	Wire.endTransmission();
	Wire.requestFrom(DS1307_CTRL_ID, 7);

	int second = bcdToDec(Wire.read() & 0x7f);
	int minute = bcdToDec(Wire.read());
	int hour = bcdToDec(Wire.read() & 0x3f);
	int dayOfWeek = bcdToDec(Wire.read());
	int dayOfMonth = bcdToDec(Wire.read());
	int month = bcdToDec(Wire.read());
	int year = 2000 + bcdToDec(Wire.read());

  setTime(hour, minute, second, dayOfMonth, month, year);
}
void setDateDs1307() { 
  Wire.beginTransmission(DS1307_CTRL_ID);
  Wire.write(byte(0x00));
  Wire.write(decToBcd(second()));  // 0 to bit 7 starts the clock
  Wire.write(decToBcd(minute()));
  Wire.write(decToBcd(hour()));	// If you want 12 hour am/pm you need to set
  // bit 6 (also need to change readDateDs1307)

  Wire.write(decToBcd(weekday(now())));
  Wire.write(decToBcd(day()));
  Wire.write(decToBcd(month()));
  Wire.write(decToBcd(year() % 100));
  Wire.endTransmission();
}

byte decToBcd(byte val) {
  return ( (val / 10 * 16) + (val % 10) );
}
byte bcdToDec(byte val) {
  return ( (val / 16 * 10) + (val % 16) );
}
#pragma endregion