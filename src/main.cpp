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

WiFiClient wifi;
WiFiManager wifiManager;
PubSubClient mqtt(wifi);
ESP8266WebServer web(80);

char mqtt_server[16] = "192.168.2.220";
char mqtt_port[6] = "1883";
void onMessage(char *topic, byte *payload, unsigned int length);

// Matrix Settings
CRGB leds[256];
FastLED_NeoMatrix *matrix;
BME280<> BMESensor;

// Taster_mid
int tasterPin[] = {D0, D4, D8};
int tasterCount = 3;

/// LDR Config
#define LDR_PIN A0
#define LDR_RESISTOR 1000 // ohms
#define LDR_PHOTOCELL LightDependentResistor::GL5516
LightDependentResistor ldr(LDR_PIN, LDR_RESISTOR, LDR_PHOTOCELL);

class Mp3Notify;
SoftwareSerial serial(D7, D5); // RX, TX
typedef DFMiniMp3<SoftwareSerial, Mp3Notify> DfMp3;
DfMp3 dfmp3(serial);
class Mp3Notify
{
};

void setup()
{
	for (int i = 0; i < tasterCount; i++)
	{
		pinMode(tasterPin[i], INPUT_PULLUP);
	}

	Serial.setRxBufferSize(1024);
	Serial.begin(115200);
	serial.begin(9600);
	dfmp3.begin();
	web.begin();
	Wire.begin(I2C_SDA, I2C_SCL);
	BMESensor.begin();

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

	WiFi.begin("wifi@lsong.one", "song940@163.com");
	Serial.println("Connecting to " + String(WiFi.SSID()));
	while (WiFi.status() != WL_CONNECTED)
	{
		Serial.print(".");
		delay(500);
	}
	Serial.println();
	Serial.println("Connected: " + WiFi.localIP().toString());

	// dfmp3.setVolume(15);
	// delay(10);
	// dfmp3.playRandomTrackFromAll();
	ldr.setPhotocellPositionOnGround(false);
	mqtt.setServer("192.168.2.220", 1883);
	mqtt.setCallback(onMessage);
}

void reconnect()
{
	String clientId = "AWTRIXController-" + String(ESP.getChipId(), HEX);
	Serial.println("connect to "  +String(mqtt_server) + " as " + clientId);
	if (mqtt.connect(clientId.c_str()))
	{
		mqtt.subscribe("test/#");
	}
}

void onMessage(char *topic, byte *payload, unsigned int length)
{
	Serial.println("incoming: " + String(topic));
}

void loop()
{
	if (!mqtt.connected())
	{
		reconnect();
	}
	else
	{
		mqtt.loop();
	}
	web.handleClient();
	float lux = ldr.getCurrentLux();
	int brightness = map(lux, 0, 1000, 5, 255);
	// Serial.println("LDR: " + String(lux));
	matrix->setBrightness(brightness);
	matrix->clear();
	matrix->drawCircle(3, 3, 3, matrix->Color(255, 0, 255));
	matrix->setCursor(8, 6);
	matrix->print(String(lux));
	matrix->show();

	// BMESensor.refresh();
	// Serial.println("Temp: " + String(BMESensor.temperature) + "°C, Press: " + String(BMESensor.pressure) + "hPa");
	// for (int i = 0; i < tasterCount; i++)
	// {
	// 	uint8_t state = digitalRead(tasterPin[i]);
	// 	Serial.println("Taster " + String(i) + ": " + String(state));
	// }
	delay(100);
}
