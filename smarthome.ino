#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <Wire.h>
#include "RTClib.h"

RTC_DS3231 rtc;

const char* ssid = "15@PJ.NET";
const char* password = "asalketik";
const char* serverName = "https://api.0xta.my.id/api/";

const int relay1 = 25;
const int relay2 = 26;
const int relay3 = 27;
const int relay4 = 33;
const int sensorApi = 32;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);
  pinMode(relay4, OUTPUT);
  pinMode(sensorApi, INPUT);

  if (!rtc.begin()) {
    Serial.println("RTC not found!");
    while (1);
  }

  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    // POST JADWAL
    HTTPClient http3;
    http3.begin(String(serverName) + "jadwal");
    int httpRespon = http3.GET();

    if (httpRespon > 0) {
      String payload = http3.getString();
      // Serial.println("Response: " + payload);

      JSONVar doc = JSON.parse(payload);

      if (JSON.typeof(doc) != "undefined") {
        String nyalaStr = (const char*) doc["data"]["nyala"];
        String matiStr  = (const char*) doc["data"]["mati"];
        int statusJ = atoi((const char*) doc["data"]["status"]);

        DateTime now = rtc.now();
        char currentTime[9];
        sprintf(currentTime, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

        Serial.print("RTC Time: ");
        Serial.println(currentTime);

        auto timeToSeconds = [](String t) {
          int hh = t.substring(0, 2).toInt();
          int mm = t.substring(3, 5).toInt();
          int ss = t.substring(6, 8).toInt();
          return hh * 3600 + mm * 60 + ss;
        };

        int nowSec   = timeToSeconds(String(currentTime));
        int nyalaSec = timeToSeconds(nyalaStr);
        int matiSec  = timeToSeconds(matiStr);

        int relayStatus = 0;
        if(statusJ == 1){
          if (nyalaSec < matiSec) {
            if (nowSec >= nyalaSec && nowSec < matiSec) {
              digitalWrite(relay2, LOW); 
              relayStatus = 1;
            } else {
              digitalWrite(relay2, HIGH);
              relayStatus = 0;
            }
          } else {
            if (nowSec >= nyalaSec || nowSec < matiSec) {
              digitalWrite(relay2, LOW);
              relayStatus = 1;
            } else {
              digitalWrite(relay2, HIGH);
              relayStatus = 0;
            }
          }

          HTTPClient postHttp;
          postHttp.begin(String(serverName) + "switch/2");
          postHttp.addHeader("Content-Type", "application/json");

          String jsonData = "{\"relay2\":" + String(relayStatus) + "}";

          int postResponseCode = postHttp.POST(jsonData);

          if (postResponseCode > 0) {
            String response = postHttp.getString();
          } else {
            Serial.printf("POST Error code: %d\n", postResponseCode);
          }
          postHttp.end();
        }
      }
    } else {
      Serial.printf("Error code: %d\n", httpRespon);
    }
    http3.end();

    // GET STATUS RELAY
    HTTPClient http1;
    http1.begin(String(serverName)+"switch");
    int httpResponseCode = http1.GET();
    if (httpResponseCode > 0) {
      String payload = http1.getString();
      // Serial.println(payload);

      JSONVar doc = JSON.parse(payload);

      if (JSON.typeof(doc) != "undefined") {
        int r1 = atoi(((const char*) doc["data"]["relay1"]));
        int r2 = atoi(((const char*) doc["data"]["relay2"]));
        int r3 = atoi(((const char*) doc["data"]["relay3"]));
        int r4 = atoi(((const char*) doc["data"]["relay4"]));
        
        digitalWrite(relay1, (r1 == 1) ? LOW : HIGH);
        digitalWrite(relay2, (r2 == 1) ? LOW : HIGH);
        digitalWrite(relay3, (r3 == 1) ? LOW : HIGH);
        digitalWrite(relay4, (r4 == 1) ? LOW : HIGH);

      }
    } else {
      Serial.printf("Error code: %d\n", httpResponseCode);
    }
    http1.end();

    // POST API FIRE
    HTTPClient http2;
    double sensorValue = analogRead(sensorApi);

    http2.begin(String(serverName)+"fire");
    http2.addHeader("Content-Type", "application/json");

    String jsonData;
    if (sensorValue < 75) {
      jsonData = "{\"adaApi\":" + String(1) +"}";
    } else {
      jsonData = "{\"adaApi\":" + String(0) +"}";
    }

    int postResponseCode = http2.POST(jsonData);

    if (postResponseCode > 0) {
      String response = http2.getString();
    } else {
      Serial.printf("POST Error code: %d\n", postResponseCode);
    }

    http2.end();
  }

  delay(1500);
}
