#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <EEPROM.h>
#include "index_html.h"
#include "Arduino.h"

#define SSID "Patsuk"
#define PASSWORD "@@@@@@@@"
#define EEPROM_SIZE 512
const String BASE_URL = "http://192.168.0.134";

const int flowPin = 0;
volatile int pulseCount = 0;

ESP8266WebServer server(80);

int delayToTriger = 10;
int frequencyTrashold = 20;

float frequency = 0;

void IRAM_ATTR pulseCounter() {
    pulseCount++;
}

String trimQuotes(String str) {
    // Find the first and last occurrence of the quotation mark
    int firstQuote = str.indexOf('"');
    int lastQuote = str.lastIndexOf('"');
    
    // Check if both quotes are found and they are not the same index
    if (firstQuote != -1 && lastQuote != -1 && firstQuote < lastQuote) {
        // Return the substring without the quotes
        return str.substring(firstQuote + 1, lastQuote);
    }
    return str; // Return original string if no valid quotes are found
}

int parseJson(const String& json, const String& field) {
    int start = json.indexOf("\"" + field + "\":") + field.length() + 3;
    int end = json.indexOf(",", start);
    if (end == -1) end = json.indexOf("}", start);  // Handle case where field is last in JSON

    // Logging
    Serial.print("Field: ");
    Serial.println(field);
    String substring = json.substring(start, end);
    Serial.print("Substring: ");
    Serial.println(substring);
    Serial.print("Int: ");
    Serial.println(trimQuotes(substring).toInt());


    return trimQuotes(substring).toInt();
}

void saveSettingsToEEPROM() {
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.put(0, delayToTriger);
    EEPROM.put(sizeof(delayToTriger), frequencyTrashold);
    EEPROM.commit();
}

void loadSettingsFromEEPROM() {
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.get(0, delayToTriger);
    EEPROM.get(sizeof(delayToTriger), frequencyTrashold);
}

void handleRoot() {
    server.send(200, "text/html", webpage);
}

void handleGetCurrentStatus() {
    String jsonResponse = "{\"frequency\":" + String(frequency) + "}";
    server.send(200, "application/json", jsonResponse);
}

void handleGetSettings() {
    String jsonResponse = "{\"frequencyThreshold\":" + String(frequencyTrashold) + ", \"delay\":" + String(delayToTriger) + "}";
    server.send(200, "application/json", jsonResponse);
}

void handleSetSettings() {
    if (server.hasArg("plain")) {
        String jsonBody = server.arg("plain");
        Serial.println("Save settings: " + jsonBody);
        int newFrequencyThreshold = parseJson(jsonBody, "frequencyThreshold");
        int newDelay = parseJson(jsonBody, "delay");
        
        frequencyTrashold = newFrequencyThreshold;
        delayToTriger = newDelay;
        saveSettingsToEEPROM();
        Serial.print("Saved frequencyTrashold:" + String(newFrequencyThreshold));
        Serial.println(" delay:" + String(delayToTriger));
        
        server.send(200, "text/plain", "Settings saved");
    } else {
        server.send(400, "text/plain", "Bad Request");
    }
}

bool triggerWent() {
    if (WiFi.status() == WL_CONNECTED) {
        WiFiClient wifiClient;
        HTTPClient http;
        http.begin(wifiClient, BASE_URL + "/changeState");
        int httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK) {
            Serial.println("changeState called successfully");
            http.end();
            return true;
        } else {
            Serial.printf("changeState request failed with code: %d\n", httpCode);
        }
        http.end();
    } else {
        Serial.println("Wi-Fi not connected");
    }
    return false;
}

void handleTestWentConnection() {
    if (triggerWent()) {
        server.send(200, "text/plain", "Went connection test successful");
    } else {
        server.send(500, "text/plain", "Failed to connect to Went");
    }
}

String getWentStatus() {
    if (WiFi.status() == WL_CONNECTED) {
        WiFiClient wifiClient;
        HTTPClient http;
        http.begin(wifiClient, BASE_URL + "/getMemoryData");
        int httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            Serial.println("Received JSON from getWentStatus: " + payload);
            http.end();
            return payload;
        } else {
            Serial.printf("GET request to /getMemoryData failed with code: %d\n", httpCode);
            http.end();
            return "";
        }
    } else {
        Serial.println("Wi-Fi not connected");
        return "";
    }
}

void turnWentOn() {
    String wentStatusJson = getWentStatus();
    if (!wentStatusJson.isEmpty()) {
        int wentSensorReading = parseJson(wentStatusJson, "wentSensorReading");
        int lightSensorReading = parseJson(wentStatusJson, "lightSensorReading");

        if (wentSensorReading == 0 && lightSensorReading == 1) {
            triggerWent();
        }
    }
}

void setup() {
    Serial.begin(9600);
    EEPROM.begin(EEPROM_SIZE);
    loadSettingsFromEEPROM();
    
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
    server.on("/getCurrentStatus", HTTP_GET, handleGetCurrentStatus);
    server.on("/getSettings", HTTP_GET, handleGetSettings);
    server.on("/setSettings", HTTP_POST, handleSetSettings);
    server.on("/testWentConnection", HTTP_GET, handleTestWentConnection);
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

        calculateAction();
    }
}

void calculateAction() {
    static unsigned long aboveThresholdStartTime = 0;
    static bool apiCalled = false;

    if (frequency >= frequencyTrashold) {
        // If we're above the threshold and haven't started tracking time yet, initialize the timer
        if (aboveThresholdStartTime == 0) {
            aboveThresholdStartTime = millis();
        }
        
        // Check if the duration above the threshold has exceeded delayToTrigger
        if (millis() - aboveThresholdStartTime >= delayToTriger * 1000) {
            if (!apiCalled) {
                turnWentOn();
                apiCalled = true;
            }
        }
    } else {
        apiCalled = false;
        aboveThresholdStartTime = 0;  // Reset the start time
    }
}