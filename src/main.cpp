// AWTRIX Controller
// Copyright (C) 2020
// by Blueforcer & Mazze2000

#include <LittleFS.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <FastLED.h>
#include <FastLED_NeoMatrix.h>
#include <Fonts/TomThumb.h>
#include <Wire.h>
#include "SoftwareSerial.h"
#include <WiFiManager.h>
#include <Wire.h>
#include <DFMiniMp3.h>
#include <BME280_t.h>
#include <LightDependentResistor.h>

#define I2C_SDA D3
#define I2C_SCL D1

// instantiate temp sensor
BME280<> BMESensor;
// Adafruit_BMP280 BMPSensor; // use I2C interface
// Adafruit_HTU21DF htu = Adafruit_HTU21DF();

WiFiClient wifi;
WiFiManager wifiManager;
PubSubClient mqtt(wifi);
ESP8266WebServer web(80);

// Matrix Settings
CRGB leds[256];
FastLED_NeoMatrix *matrix;

//Taster_mid
int tasterPin[] = {D0, D4, D8};
int tasterCount = 3;


/// LDR Config
#define LDR_PIN A0
#define LDR_RESISTOR 1000 //ohms
#define LDR_PHOTOCELL LightDependentResistor::GL5516
LightDependentResistor ldr(LDR_PIN, LDR_RESISTOR, LDR_PHOTOCELL);

class Mp3Notify; 
SoftwareSerial mySoftwareSerial(D7, D5); // RX, TX
typedef DFMiniMp3<SoftwareSerial, Mp3Notify> DfMp3; 
DfMp3 dfmp3(mySoftwareSerial);

class Mp3Notify
{

};

void showText(char* text) {
	matrix->clear();
	matrix->setCursor(7, 6);
	for (int x = 32; x >= -90; x--)
	{
		matrix->clear();
		matrix->setCursor(x, 6);
		matrix->print(text);
		matrix->setTextColor(matrix->Color(0, 255, 50));
		matrix->show();
		delay(40);
	}
}

void setup()
{
	for (int i = 0; i < tasterCount; i++)
	{
		pinMode(tasterPin[i], INPUT_PULLUP);
	}

	Serial.setRxBufferSize(1024);
	Serial.begin(115200);
	mySoftwareSerial.begin(9600);
	dfmp3.begin();
	web.begin();
	// dfmp3.setVolume(15);
	// delay(10);
	// dfmp3.playRandomTrackFromAll();
	
	FastLED.addLeds<NEOPIXEL, D2>(leds, 256).setCorrection(TypicalLEDStrip);
	matrix = new FastLED_NeoMatrix(leds, 32, 8, NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG);
	matrix->begin();
	matrix->setTextWrap(false);
	matrix->setBrightness(30);
	matrix->setFont(&TomThumb);
	matrix->clear();
	matrix->setTextColor(matrix->Color(255, 0, 255));
	matrix->setCursor(9, 6);
	matrix->print("BOOT");
	matrix->show();
	delay(2000);

	Wire.begin(I2C_SDA, I2C_SCL);
	BMESensor.begin();
	ldr.setPhotocellPositionOnGround(false);

	// mqtt.setServer(awtrix_server, atoi(Port));
	// mqtt.setCallback(callback);
}

void loop()
{
	web.handleClient();
	ArduinoOTA.handle();
	float lux = ldr.getCurrentLux();
	// Serial.println("LDR: " + String(lux));
	int brightness = map(lux, 0, 300, 0, 255);
	matrix->clear();
	matrix->setBrightness(brightness);
	matrix->setCursor(0, 6);
	matrix->print("Lux: " + String(lux));
	matrix->show();
	BMESensor.refresh();
	// Serial.println("Temp: " + String(BMESensor.temperature) + "°C, Press: " + String(BMESensor.pressure) + "hPa");
	delay(100);
	for (int i = 0; i < tasterCount; i++)
	{
		uint8_t state = digitalRead(tasterPin[i]);
		Serial.println("Taster " + String(i) + ": " + String(state));
	}
}
