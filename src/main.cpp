#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include "index_html.h"
#include "Arduino.h"

#define SSID "Patsuk"
#define PASSWORD "@@@@@@@@"
#define EEPROM_SIZE 512

const int flowPin = 0;
volatile int pulseCount = 0;

ESP8266WebServer server(80);

float frequency = 0;

void IRAM_ATTR pulseCounter() {
    pulseCount++;
}


void handleRoot() {
    server.send(200, "text/html", webpage);
}

void handleGetFrequency() {
    String jsonResponse = "{\"frequency\":" + String(frequency) + "}";
    server.send(200, "application/json", jsonResponse);
}

void setup() {
    Serial.begin(9600);
    
    pinMode(flowPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(flowPin), pulseCounter, RISING);

    WiFi.begin(SSID, PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to Wi-Fi");

    // Set up server routes
    server.on("/", handleRoot);
    server.on("/getFrequency", HTTP_GET, handleGetFrequency);

    server.begin();
    Serial.println("HTTP server started");
}

void loop() {
    server.handleClient();
    static unsigned long lastMillisFrequencyCalculation = 0;
    unsigned long currentMillis = millis();
    if (currentMillis - lastMillisFrequencyCalculation >= 1000) {  // Every second
        lastMillisFrequencyCalculation = currentMillis;
        frequency = pulseCount;
        pulseCount = 0;  // Reset pulse count
        Serial.print("Frequency: ");
        Serial.println(frequency);

    }
}
