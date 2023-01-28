#define BLYNK_TEMPLATE_ID "Your Template ID"
#define BLYNK_DEVICE_NAME "Your Device Name"

#define BLYNK_PRINT Serial

//libraries
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Adafruit_BME280.h>
#include <SPI.h>
#include <Wire.h>
#include <ADS1X15.h>

ADS1115 ADS(0x48);
BlynkTimer timer;

//BME280 Connections
#define BME_CS 5
Adafruit_BME280 bme(BME_CS);  // hardware SPI

//Blynk Details
char auth[] = "Your Auth";  //Auth Token

// Set password to "" for open networks.
char ssid[] = "Your SSID";  //Ssid
char pass[] = "Your Password";   //Pass 

//Soil Moisture Sensor Calibration
const int AirValue = 12240;   //Air Value for Moisture Sensor
const int WaterValue = 5570;  //Water Value for Moisture Sensor

//relay pin (for soloeid water valve)
int relay = 26;

//Counter
int water_count = 0;

//Variables
int soilmoisturepercent;

int SoilTuningPercent;

bool watering = false;

BLYNK_WRITE(V4) {
  SoilTuningPercent = param.asInt();
  Serial.println();
  Serial.println("Soil Tuning Set To: ");
  Serial.print(SoilTuningPercent);
}

void setup() {
  // Debug console
  Serial.begin(115200);
  SPI.begin();
  ADS.begin();
  bme.begin();
  Blynk.begin(auth, ssid, pass);

  pinMode(relay, OUTPUT);
  //events
  timer.setInterval(1000L, bme280);
  timer.setInterval(1000L, checkmoisture);
}

void loop() {
  checkwatering();
  Blynk.run();
  timer.run();
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
void checkwatering() {
  if (watering == true) {
    digitalWrite(relay, HIGH);
  }
  if (soilmoisturepercent > SoilTuningPercent - 5) {
    watering == false;
    digitalWrite(relay, LOW);
    completewatering();
  }
}

void checkmoisture() {
  ADS.setGain(0);
  int16_t adc0 = ADS.readADC(0);

  soilmoisturepercent = map(adc0, AirValue, WaterValue, 0, 100);

  Serial.println(adc0);

  if (soilmoisturepercent >= 100) {
    Blynk.virtualWrite(V2, 100);
  } else if (soilmoisturepercent <= 0) {
    Blynk.virtualWrite(V2, 0);
  } else if (soilmoisturepercent > 0 && soilmoisturepercent < 100) {
    Blynk.virtualWrite(V2, soilmoisturepercent);
  }

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
  if (soilmoisturepercent < SoilTuningPercent - 10) {
    water_count++;
  }
  if (water_count == 5) {  //To wait for the water to go through the pot.
    watering = true;
    water_count = 0;
  }
}
