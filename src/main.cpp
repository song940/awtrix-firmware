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
#include <LightDependentResistor.h>
#include <Wire.h>
#include <SparkFun_APDS9960.h>
#include "SoftwareSerial.h"

#include <WiFiManager.h>
#include <DoubleResetDetect.h>
#include <Wire.h>
#include <BME280_t.h>
#include "Adafruit_HTU21DF.h"
#include <Adafruit_BMP280.h>

#include <DFMiniMp3.h>

#include "MenueControl/MenueControl.h"

// instantiate temp sensor
BME280<> BMESensor;
Adafruit_BMP280 BMPSensor; // use I2C interface
Adafruit_HTU21DF htu = Adafruit_HTU21DF();

enum MsgType
{
	MsgType_Wifi,
	MsgType_Host,
	MsgType_Temp,
	MsgType_Audio,
	MsgType_Gest,
	MsgType_LDR,
	MsgType_Other
};

enum TempSensor
{
	TempSensor_None,
	TempSensor_BME280,
	TempSensor_BMP280,
	TempSensor_HTU21D,
}; // None = 0

IPAddress Server;
WiFiClient espClient;
WiFiManager wifiManager;
PubSubClient client(espClient);
ESP8266WebServer server(80);

// Matrix Settings
CRGB leds[256];
FastLED_NeoMatrix *matrix;

//resetdetector
#define DRD_TIMEOUT 5.0
#define DRD_ADDRESS 0x00
DoubleResetDetect drd(DRD_TIMEOUT, DRD_ADDRESS);

//Taster_mid
int tasterPin[] = {D0, D4, D8};
int tasterCount = 3;

/// LDR Config
int ldrState = 1000;		// 0 = None
#define LDR_RESISTOR 1000 //ohms
#define LDR_PIN A0
#define LDR_PHOTOCELL LightDependentResistor::GL5516
LightDependentResistor ldr(LDR_PIN, ldrState, LDR_PHOTOCELL);

// Gesture Sensor
#define APDS9960_INT D6
#define I2C_SDA D3
#define I2C_SCL D1
SparkFun_APDS9960 apds = SparkFun_APDS9960();
volatile bool isr_flag = 0;

#ifndef ICACHE_RAM_ATTR
#define ICACHE_RAM_ATTR IRAM_ATTR
#endif

class Mp3Notify; 
SoftwareSerial mySoftwareSerial(D7, D5); // RX, TX
typedef DFMiniMp3<SoftwareSerial, Mp3Notify> DfMp3; 
DfMp3 dfmp3(mySoftwareSerial);

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
	mySoftwareSerial.begin(9600);
	dfmp3.begin();
	uint16_t mode = dfmp3.getPlaybackMode();
	Serial.print("playback mode ");
	Serial.println(mode);
	dfmp3.setVolume(15);
	delay(10);
	dfmp3.playRandomTrackFromAll();
	
	FastLED.addLeds<NEOPIXEL, D2>(leds, 256).setCorrection(TypicalLEDStrip);
	// FastLED.addLeds<NEOPIXEL, D2>(leds, 256).setTemperature(Candle);
	// FastLED.addLeds<NEOPIXEL, D2>(leds, 256).setTemperature(Tungsten40W);
	// FastLED.addLeds<NEOPIXEL, D2>(leds, 256).setTemperature(Tungsten100W);
	// FastLED.addLeds<NEOPIXEL, D2>(leds, 256).setTemperature(Halogen);
	// FastLED.addLeds<NEOPIXEL, D2>(leds, 256).setTemperature(CarbonArc);
	// FastLED.addLeds<NEOPIXEL, D2>(leds, 256).setTemperature(HighNoonSun);
	// FastLED.addLeds<NEOPIXEL, D2>(leds, 256).setTemperature(DirectSunlight);
	// FastLED.addLeds<NEOPIXEL, D2>(leds, 256).setTemperature(OvercastSky);
	// FastLED.addLeds<NEOPIXEL, D2>(leds, 256).setTemperature(ClearBlueSky);
	// FastLED.addLeds<NEOPIXEL, D2>(leds, 256).setTemperature(WarmFluorescent);
	// FastLED.addLeds<NEOPIXEL, D2>(leds, 256).setTemperature(StandardFluorescent);
	// FastLED.addLeds<NEOPIXEL, D2>(leds, 256).setTemperature(CoolWhiteFluorescent);
	// FastLED.addLeds<NEOPIXEL, D2>(leds, 256).setTemperature(FullSpectrumFluorescent);
	// FastLED.addLeds<NEOPIXEL, D2>(leds, 256).setTemperature(GrowLightFluorescent);
	// FastLED.addLeds<NEOPIXEL, D2>(leds, 256).setTemperature(BlackLightFluorescent);
	// FastLED.addLeds<NEOPIXEL, D2>(leds, 256).setTemperature(MercuryVapor);
	// FastLED.addLeds<NEOPIXEL, D2>(leds, 256).setTemperature(SodiumVapor);
	// FastLED.addLeds<NEOPIXEL, D2>(leds, 256).setTemperature(MetalHalide);
	// FastLED.addLeds<NEOPIXEL, D2>(leds, 256).setTemperature(HighPressureSodium);
	// FastLED.addLeds<NEOPIXEL, D2>(leds, 256).setTemperature(UncorrectedTemperature);
	matrix = new FastLED_NeoMatrix(leds, 32, 8, NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG);
	// matrix = new FastLED_NeoMatrix(leds, 8, 8, 4, 1, NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE);
	// matrix = new FastLED_NeoMatrix(leds, 32, 8, NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG);s
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
	server.begin();

	Wire.begin(I2C_SDA, I2C_SCL);
	BMESensor.begin();
	// BMPSensor.begin(BMP280_ADDRESS_ALT);
	// /* Default settings from datasheet. */
	// BMPSensor.setSampling(
	// 	Adafruit_BMP280::MODE_NORMAL,	/* Operating Mode. */
	// 	Adafruit_BMP280::SAMPLING_X2,	/* Temp. oversampling */
	// 	Adafruit_BMP280::SAMPLING_X16,	/* Pressure oversampling */
	// 	Adafruit_BMP280::FILTER_X16,	/* Filtering. */
	// 	Adafruit_BMP280::STANDBY_MS_500 /* Standby time. */
	// );

	ldr.setPhotocellPositionOnGround(false);
	apds.enableGestureSensor(true);
	if (apds.init()) {
		pinMode(APDS9960_INT, INPUT);
	}

	matrix->clear();
	matrix->setCursor(7, 6);
	for (int x = 32; x >= -90; x--)
	{
		matrix->clear();
		matrix->setCursor(x, 6);
		matrix->print("Host-IP: 0.0.0.0:8000");
		matrix->setTextColor(matrix->Color(0, 255, 50));
		matrix->show();
		delay(40);
	}

	// client.setServer(awtrix_server, atoi(Port));
	// client.setCallback(callback);
}

void loop()
{
	server.handleClient();
	ArduinoOTA.handle();
	// Serial.println("LDR: " + String(ldr.getCurrentLux()));
	// sensors_event_t temp_event, pressure_event;
	// BMPSensor.getTemperatureSensor()->getEvent(&temp_event);
	// BMPSensor.getPressureSensor()->getEvent(&pressure_event);
	// Serial.println("Temp: " + String(temp_event.temperature) + "°C, Press: " + String(pressure_event.pressure) + "hPa");
	BMESensor.refresh();
	Serial.println("Temp: " + String(BMESensor.temperature) + "°C, Press: " + String(BMESensor.pressure) + "hPa");
	delay(100);
}
