//
// C0R3SEQ32: A Sequencer Server for ESP32
//
// This program implements a web server on ESP32 that:
//  * Controls 8 relays in sequenced patterns
//  * Serves a web interface for pattern editing
//  * Handles API requests for pattern manipulation
//  * Stores patterns in SPIFFS for persistence
//
// Author: Budi Prakosa (aka manticore)
// License: MIT License
// Copyright (c) 2024 Budi Prakosa
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "AsyncJson.h"
AsyncWebServer server(80);

#define NUM_RELAYS 8
#define NUM_STEPS 16
#define NUM_PATTERNS 3

#define RELAY_PIN_1 14
#define RELAY_PIN_2 11
#define RELAY_PIN_3 16
#define RELAY_PIN_4 2
#define RELAY_PIN_5 15
#define RELAY_PIN_6 12
#define RELAY_PIN_7 13
#define RELAY_PIN_8 10

#define WIFI_SSID "C0R3SEQ32"
#define WIFI_PASSWORD "12345678"

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

const byte RELAY_PINS[NUM_RELAYS] = {RELAY_PIN_1, RELAY_PIN_2, RELAY_PIN_3, RELAY_PIN_4, RELAY_PIN_5, RELAY_PIN_6, RELAY_PIN_7, RELAY_PIN_8};

// Define multiple patterns for each relay (0 = OFF, 1 = ON)
byte patterns[NUM_PATTERNS][NUM_RELAYS][NUM_STEPS] = {
    {
        // Pattern 1
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY1 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY2 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY3 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY4 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY5 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY6 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY7 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}  // RELAY8 pattern
    },
    {
        // Pattern 2
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY1 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY2 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY3 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY4 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY5 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY6 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY7 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}  // RELAY8 pattern
    },
    {
        // Pattern 3
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY1 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY2 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY3 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY4 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY5 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY6 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY7 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}  // RELAY8 pattern
    }};

int baseStepDuration = 300; // Base duration of each step in milliseconds
int swingAmount = 30;       // Amount of swing in milliseconds
int relayOnTime = 50;       // Duration the relay stays ON within each step

int currentPattern = 0;
int currentStep = 0;
unsigned long lastStepTime = 0;
unsigned long relayStartTime = 0;
bool relayOn = false;
int currentRelay = 0;
bool isPaused = false;
bool isPatternLocked = false; // Flag to indicate if pattern is locked

void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
}

void setup()
{
    Serial.begin(115200);

    // Scan for existing networks
    int n = WiFi.scanNetworks();
    bool mainNetworkFound = false;

    for (int i = 0; i < n; ++i) {
        if (WiFi.SSID(i) == "CORESEQMAIN") {
            mainNetworkFound = true;
            break;
        }
    }

    if (mainNetworkFound) {
        // Connect to the main network
        WiFi.begin("CORESEQMAIN", "password"); // Replace "password" with the actual password
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }
        Serial.println("\nConnected to CORESEQMAIN");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    } else {
        // Act as the main network
        WiFi.softAP("CORESEQMAIN", "password"); // Replace "password" with a secure password
        Serial.println("Acting as main network CORESEQMAIN");
        Serial.print("AP IP Address: ");
        Serial.println(WiFi.softAPIP());
    }

    if (!SPIFFS.begin(true))
    {
        Serial.println("An error occurred while mounting SPIFFS");
        return;
    }
    for (int i = 0; i < NUM_PATTERNS; i++)
    {
        loadPatternFromFile(i);
    }

    loadTimingFromFile();
    
    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
    server.serveStatic("/static/", SPIFFS, "/static/");

    server.onNotFound(notFound);
    attachRoutes();

    server.begin();
}

AsyncCallbackJsonWebHandler* setPatternHandler = new AsyncCallbackJsonWebHandler("/setPattern", [](AsyncWebServerRequest *request, JsonVariant &json) {
    JsonObject jsonObj = json.as<JsonObject>();

    if (!jsonObj.containsKey("patternIndex"))
    {
        request->send(400, "text/plain", "Missing patternIndex");
        return;
    }

    int patternIndex = jsonObj["patternIndex"];

    if (patternIndex < 0 || patternIndex >= NUM_PATTERNS)
    {
        request->send(400, "text/plain", "Invalid patternIndex");
        return;
    }

    if (!jsonObj.containsKey("patternData"))
    {
        request->send(400, "text/plain", "Missing patternData");
        return;
    }

    JsonArray patternData = jsonObj["patternData"].as<JsonArray>();

    if (patternData.size() != NUM_RELAYS * NUM_STEPS)
    {
        request->send(400, "text/plain", "Invalid patternData size");
        return;
    }

    for (int relay = 0; relay < NUM_RELAYS; relay++)
    {
        for (int step = 0; step < NUM_STEPS; step++)
        {
            int value = patternData[relay * NUM_STEPS + step];
            if (value != 0 && value != 1)
            {
                request->send(400, "text/plain", "Invalid patternData value");
                return;
            }
            patterns[patternIndex][relay][step] = value;
        }
    }

    savePatternToFile(patternIndex);

    currentPattern = patternIndex;
    currentStep = 0; // Reset the current step to 0
    relayOn = false; // Ensure all relays are turned off
    request->send(200, "text/plain", "Pattern set successfully");
});

void handleGetPatterns(AsyncWebServerRequest *request)
{
    DynamicJsonDocument doc(2048);
    JsonArray patternsArray = doc.createNestedArray("patterns");

    for (int i = 0; i < NUM_PATTERNS; i++)
    {
        JsonObject pattern = patternsArray.createNestedObject();
        pattern["patternIndex"] = i;
        JsonArray patternData = pattern.createNestedArray("patternData");

        for (int relay = 0; relay < NUM_RELAYS; relay++)
        {
            JsonArray relayData = patternData.createNestedArray();
            for (int step = 0; step < NUM_STEPS; step++)
            {
                relayData.add(patterns[i][relay][step]);
            }
        }
    }

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void handleTriggerAllRelays(AsyncWebServerRequest *request)
{
    // Turn on all relays
    for (int i = 0; i < NUM_RELAYS; i++)
    {
        digitalWrite(RELAY_PINS[i], LOW);
    }

    // Wait for relayOnTime
    delay(relayOnTime);

    // Turn off all relays
    for (int i = 0; i < NUM_RELAYS; i++)
    {
        digitalWrite(RELAY_PINS[i], HIGH);
    }

    request->send(200, "text/plain", "All relays triggered");
}

void handlePause(AsyncWebServerRequest *request)
{
    if (!request->hasParam("plain", true))
    {
        request->send(400, "text/plain", "Body not received");
        return;
    }

    String body = request->getParam("plain", true)->value();
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, body);

    if (error)
    {
        request->send(400, "text/plain", "Invalid JSON");
        return;
    }

    if (!doc.containsKey("pause"))
    {
        request->send(400, "text/plain", "Missing pause parameter");
        return;
    }

    isPaused = doc["pause"].as<bool>();

    if (isPaused)
    {
        // Turn off all relays when paused
        for (int i = 0; i < NUM_RELAYS; i++)
        {
            digitalWrite(RELAY_PINS[i], HIGH);
        }
        relayOn = false;
    }

    request->send(200, "text/plain", isPaused ? "Sequencer paused" : "Sequencer resumed");
}

void saveTimingToFile()
{
    File file = SPIFFS.open("/timing_params.csv", FILE_WRITE);
    if (file)
    {
        file.println("baseStepDuration,swingAmount,relayOnTime");
        file.printf("%d,%d,%d\n", baseStepDuration, swingAmount, relayOnTime);
        file.close();
        Serial.println("Timing parameters saved successfully");
    }
    else
    {
        Serial.println("Failed to save timing parameters");
    }
}


void loadTimingFromFile()
{
    File file = SPIFFS.open("/timing_params.csv", FILE_READ);
    if (!file)
    {
        Serial.println("Failed to open timing_params.csv");
        return;
    }

    // Skip the header line
    file.readStringUntil('\n');

    // Read the values
    String line = file.readStringUntil('\n');
    int commaIndex1 = line.indexOf(',');
    int commaIndex2 = line.indexOf(',', commaIndex1 + 1);

    if (commaIndex1 != -1 && commaIndex2 != -1)
    {
        baseStepDuration = line.substring(0, commaIndex1).toInt();
        swingAmount = line.substring(commaIndex1 + 1, commaIndex2).toInt();
        relayOnTime = line.substring(commaIndex2 + 1).toInt();
        Serial.println("Timing parameters loaded successfully");
    }
    else
    {
        Serial.println("Invalid format in timing_params.csv");
    }

    file.close();
}

void handleSetTiming(AsyncWebServerRequest *request)
{
    if (!request->hasParam("plain", true))
    {
        request->send(400, "text/plain", "Body not received");
        return;
    }

    String body = request->getParam("plain", true)->value();
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, body);

    if (error)
    {
        request->send(400, "text/plain", "Invalid JSON");
        return;
    }

    bool updated = false;

    if (doc.containsKey("baseStepDuration"))
    {
        baseStepDuration = doc["baseStepDuration"];
        updated = true;
    }
    if (doc.containsKey("swingAmount"))
    {
        swingAmount = doc["swingAmount"];
        updated = true;
    }
    if (doc.containsKey("relayOnTime"))
    {
        relayOnTime = doc["relayOnTime"];
        updated = true;
    }

    if (updated)
    {
        saveTimingToFile();
        request->send(200, "text/plain", "Timing parameters updated and saved successfully");
    }
    else
    {
        request->send(200, "text/plain", "No timing parameters were updated");
    }
}


void handleLockPattern(AsyncWebServerRequest *request)
{
    if (!request->hasParam("plain", true))
    {
        request->send(400, "text/plain", "Body not received");
        return;
    }

    String body = request->getParam("plain", true)->value();
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, body);

    if (error)
    {
        request->send(400, "text/plain", "Invalid JSON");
        return;
    }

    if (!doc.containsKey("lock"))
    {
        request->send(400, "text/plain", "Missing lock parameter");
        return;
    }

    if (!doc.containsKey("patternIndex"))
    {
        request->send(400, "text/plain", "Missing patternIndex parameter");
        return;
    }

    isPatternLocked = doc["lock"].as<bool>();
    int patternIndex = doc["patternIndex"].as<int>();

    if (patternIndex < 0 || patternIndex >= NUM_PATTERNS)
    {
        request->send(400, "text/plain", "Invalid patternIndex");
        return;
    }

    currentPattern = patternIndex;

    request->send(200, "text/plain", isPatternLocked ? "Pattern locked" : "Pattern unlocked");
}

void savePatternToFile(int patternIndex)
{
    // Create a filename based on the pattern index
    String filename = "/pattern_" + String(patternIndex) + ".csv";

    // Open the file for writing
    File file = SPIFFS.open(filename, "w");
    if (!file)
    {
        Serial.println("Failed to open file for writing");
        return;
    }

    // Write the pattern data as CSV
    for (int relay = 0; relay < NUM_RELAYS; relay++)
    {
        for (int step = 0; step < NUM_STEPS; step++)
        {
            file.print(patterns[patternIndex][relay][step]);
            if (step < NUM_STEPS - 1)
                file.print(",");
        }
        file.println();
    }

    // Close the file
    file.close();
    Serial.println("Pattern saved to CSV file successfully");
}

void loadPatternFromFile(int patternIndex)
{
    // Create a filename based on the pattern index
    String filename = "/pattern_" + String(patternIndex) + ".csv";

    // Check if the file exists
    if (!SPIFFS.exists(filename))
    {
        Serial.println("File does not exist");
        return;
    }

    // Open the file for reading
    File file = SPIFFS.open(filename, "r");
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        return;
    }

    // Read and parse the CSV data
    for (int relay = 0; relay < NUM_RELAYS; relay++)
    {
        String line = file.readStringUntil('\n');
        int step = 0;
        int startIndex = 0;
        int commaIndex;

        while ((commaIndex = line.indexOf(',', startIndex)) != -1 && step < NUM_STEPS)
        {
            patterns[patternIndex][relay][step] = line.substring(startIndex, commaIndex).toInt();
            startIndex = commaIndex + 1;
            step++;
        }

        // Handle the last value in the row
        if (step < NUM_STEPS)
        {
            patterns[patternIndex][relay][step] = line.substring(startIndex).toInt();
        }
    }

    // Close the file
    file.close();
    Serial.println("Pattern loaded from CSV file successfully");

}

void attachRoutes()
{
    Serial.println("Router");
    server.addHandler(setPatternHandler);
    server.on("/getPatterns", HTTP_GET, handleGetPatterns);
    server.on("/triggerAllRelays", HTTP_POST, handleTriggerAllRelays);
    server.on("/pause", HTTP_POST, handlePause);
    server.on("/setTiming", HTTP_POST, handleSetTiming);
    server.on("/lockPattern", HTTP_POST, handleLockPattern); // Add lock pattern endpoint
}

void loop()
{
}