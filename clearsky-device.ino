#include "SimpleTimer.h"
#include "ThingSpeak.h"
#include "secrets.h"
#include <ESP8266WiFi.h>
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#define DUST_SENSOR_DIGITAL_PIN_PM10  D1        // DSM501 Pin 2 of DSM501 (jaune / Yellow)
#define DUST_SENSOR_DIGITAL_PIN_PM25  D3        // DSM501 Pin 4 (rouge / red) 
#define EXCELLENT                     "Excellent"
#define GOOD                          "GOOD"
#define ACCEPTABLE                    "ACCEPTABLE"
#define MODERATE                      "MODERATE"
#define HEAVY                         "HEAVY"
#define SEVERE                        "SEVERE"
#define HAZARDOUS                     "HAZARDOUS"
#define CHANNEL_NUMBER                904995
#define WRITE_API_KEY                 "T55JGKDOP2FUVNE5"

float           ratio = 0;
unsigned long   sampletime_ms = 2 * 60 * 1000;  // sample time (ms)

unsigned long myChannelNumber = CHANNEL_NUMBER;
const char * myWriteAPIKey = WRITE_API_KEY;
const int ledPin = 14;              // D5
int keyIndex = 0;            // your network key index number (needed only for WEP)
WiFiClient  client;

struct structAQI {

  unsigned long   lowpulseoccupancyPM10 = 0;
  unsigned long   lowpulseoccupancyPM25 = 0;
  // Sensor AQI data
  float         concentrationPM25 = 0;
  float         concentrationPM10  = 0;
};
struct structAQI AQI;

SimpleTimer timer;

void updateAQI() {

  float ratio = AQI.lowpulseoccupancyPM10 / (sampletime_ms * 10.0);
  float concentration = 1.1 * pow( ratio, 3) - 3.8 * pow(ratio, 2) + 520 * ratio + 0.62; // using spec sheet curve
  if ( sampletime_ms < 3600000 ) {
    concentration = concentration * ( sampletime_ms / 3600000.0 );
  }
  AQI.lowpulseoccupancyPM10 = 0;
  AQI.concentrationPM10 = concentration;

  ratio = AQI.lowpulseoccupancyPM25 / (sampletime_ms * 10.0);
  concentration = 1.1 * pow( ratio, 3) - 3.8 * pow(ratio, 2) + 520 * ratio + 0.62;
  if ( sampletime_ms < 3600000 ) {
    concentration = concentration * ( sampletime_ms / 3600000.0 );
  }
  AQI.lowpulseoccupancyPM25 = 0;
  AQI.concentrationPM25 = concentration;

  Serial.print("Concentrations => PM2.5: "); Serial.print(AQI.concentrationPM25); Serial.print(" | PM10: "); Serial.println(AQI.concentrationPM10);

  int httpStatus = sendToAPI(AQI.concentrationPM10, AQI.concentrationPM25);
  if (httpStatus != 200)
  {
    Serial.print("Http error: " + httpStatus);
    for (int i = 1; i <= 3; i++)
    {
      digitalWrite(ledPin, HIGH);
      delay(1000);
      digitalWrite(ledPin, LOW);
    }
  }
}

void setup() {

  Serial.begin(115200);
  pinMode(DUST_SENSOR_DIGITAL_PIN_PM10, INPUT);
  pinMode(DUST_SENSOR_DIGITAL_PIN_PM25, INPUT);
  pinMode(ledPin, OUTPUT);

  WiFiManager wifiManager;
  wifiManager.autoConnect("AutoConnectAP");
  ThingSpeak.begin(client);

  digitalWrite(ledPin, HIGH);
  // wait 60s for DSM501 to warm up
  for (int i = 1; i <= 60; i++)
  {
    delay(1000); // 1s
    Serial.print(i);
    Serial.println(" s (wait 60s for DSM501 to warm up)");
  }
  Serial.println("Ready!");
  digitalWrite(ledPin, LOW);

  timer.setInterval(sampletime_ms, updateAQI);
}

int sendToAPI(float concentrationPM10, float concentrationPM25) {

  digitalWrite(ledPin, HIGH);
  int status = ThingSpeak.writeFields(myChannelNumber, 1, concentrationPM10, 2, concentrationPM25, myWriteAPIKey);
  delay(1000);
  digitalWrite(ledPin, LOW);
  return status;
}

void loop() {
  AQI.lowpulseoccupancyPM10 += pulseIn(DUST_SENSOR_DIGITAL_PIN_PM10, LOW);
  AQI.lowpulseoccupancyPM25 += pulseIn(DUST_SENSOR_DIGITAL_PIN_PM25, LOW);

  timer.run();
}
