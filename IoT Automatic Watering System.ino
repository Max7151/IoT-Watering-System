#define BLYNK_TEMPLATE_ID "REPLACE WITH TEMPLATE ID"//Template ID
#define BLYNK_DEVICE_NAME "REPLACE WITH DEVICE NAME"//Device Name

#define BLYNK_PRINT Serial

//libraries
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Adafruit_BME280.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;
BlynkTimer timer;

//BME280 Connections
#define BME_CS 5
Adafruit_BME280 bme(BME_CS);  // hardware SPI

//Blynk Details
char auth[] = "REPLACE WITH AUTH TOKEN";//Auth Token

// Set password to "" for open networks.
char ssid[] = "REPLACE WITH WIFI SSID";//Ssid
char pass[] = "REPLACE WITH WIFI PASSWORD";/Pass

//Soil Moisture Sensor Calibration
const int AirValue = REPLACE WITH AIR VALUE;//Air Value for Moisture Sensor
const int WaterValue = REPLACE WITH WATER VALUE;//Water Value for Moisture Sensor

//relay pin (for soloeid water valve)
int relay = 26;

//Counter
int water_count = 0;

//Variables
int soilmoisturepercent;

int SoilTuningPercent;


//Create a 16Bit variable for ADS1115
int16_t adc0;

BLYNK_WRITE(V4) {
  SoilTuningPercent = param.asInt();
  Serial.println();
  Serial.println("Soil Tuning Set To: ");
  Serial.print(SoilTuningPercent);
}

void bme280() {
  float t = bme.readTemperature();
  float h = bme.readHumidity();
  float p = bme.readPressure() / 100;

  if (isnan(t) || isnan(h) || isnan(p)) {
    Serial.println("Failed to read from BME280 sensor!");
    return;
  }
  delay(5000);

  Serial.println();
  Serial.println("Temperature:");
  Serial.print(t);
  Serial.println();
  Serial.println("Humidity");
  Serial.print(h);
  Serial.println();
  Serial.println("Hpa");
  Serial.print(p);
  Serial.println();
  delay(2000);
  Blynk.virtualWrite(V0, t);
  Blynk.virtualWrite(V1, h);
  Blynk.virtualWrite(V3, p);
}

void completewatering() {
  if (soilmoisturepercent > SoilTuningPercent - 5 || soilmoisturepercent < SoilTuningPercent + 5) {
    Blynk.notify("Your Plant has succesfully been watered!");
    Blynk.logEvent("Watering Completed :D");
    delay(100);
  } else {
    delay(1000);
  }
}

//Watering
void watering() {
  digitalWrite(relay, HIGH);
  delay(10000);
  digitalWrite(relay, LOW);
  delay(100);
  completewatering();
}

void checkmoisture() {
  delay(1000);
  adc0 = ads.readADC_SingleEnded(0);

  soilmoisturepercent = map(adc0, AirValue, WaterValue, 0, 100);

  if (soilmoisturepercent >= 100) {
    Serial.println();
    Serial.print("Soil Moisture Percent: 100%");
  } else if (soilmoisturepercent <= 0) {
    Serial.println();
    Serial.print("Soil Moisture Percent: 0");
  } else if (soilmoisturepercent > 0 && soilmoisturepercent < 100) {
    Serial.println();
    Serial.println("Soil Moisture Percent:");
    Serial.print(soilmoisturepercent);
    Serial.print("%");
  }
  if (soilmoisturepercent < SoilTuningPercent - 15) {
    water_count++;
  }
  if (water_count == 5) {  //To wait for the water to go through the pot.
    watering();
    water_count = 0;
  }
}

void sendcheckmoisture() {
  if (soilmoisturepercent >= 100) {
    Blynk.virtualWrite(V2, 100);
  } else if (soilmoisturepercent <= 0) {
    Blynk.virtualWrite(V2, 0);
  } else if (soilmoisturepercent > 0 && soilmoisturepercent < 100) {
    Blynk.virtualWrite(V2, soilmoisturepercent);
  }
}

void setup() {
  // Debug console
  Serial.begin(115200);
  SPI.begin();
  ads.begin();
  bme.begin();
  Blynk.begin(auth, ssid, pass);

  pinMode(relay, OUTPUT);
  //events
  timer.setInterval(1000L, bme280);
  timer.setInterval(1000L, sendcheckmoisture);
}

void loop() {

  checkmoisture();
  Blynk.run();
  timer.run();
}