//
// C0R3SEQ32: A Sequencer Server for ESP32
//
// This program implements a web server on ESP32 that:
//  * Controls 8 relays in sequenced patterns using uClock for precise timing
//  * Uses uClock's 96 PPQN (Pulses Per Quarter Note) resolution for accurate timing
//  * Serves a web interface for pattern editing built with React and shadcn/ui
//  * Handles API requests for pattern manipulation
//  * Stores patterns in SPIFFS for persistence
//  * Supports tempo control and swing timing via uClock integration
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
#include <uClock.h>

AsyncWebServer server(80);
#define NUM_RELAYS 16
#define NUM_STEPS 16
#define NUM_PATTERNS 3

#define STEP_MAX_SIZE    16  // Number of steps in a pattern
// #define NOTE_LENGTH      12  // Adjust as needed
#define NUM_SOLENOIDS    16   // Number of solenoids (relays)
#define NUM_PATTERNS     3   // Number of patterns

#define RELAY_PIN_1 14
#define RELAY_PIN_2 11
#define RELAY_PIN_3 16
#define RELAY_PIN_4 2
#define RELAY_PIN_5 15
#define RELAY_PIN_6 12
#define RELAY_PIN_7 13
#define RELAY_PIN_8 10
#define RELAY_PIN_9 9
#define RELAY_PIN_10 8
#define RELAY_PIN_11 7
#define RELAY_PIN_12 6
#define RELAY_PIN_13 5
#define RELAY_PIN_14 4
#define RELAY_PIN_15 3
#define RELAY_PIN_16 1

#define WIFI_SSID "C0R3SEQ32"
#define WIFI_PASSWORD "12345678"

int SOLENOID_PINS[NUM_SOLENOIDS] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};  // Pins connected to the solenoids
int relayActiveStates[NUM_SOLENOIDS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int NOTE_LENGTH = 12;

int solenoid_lengths[NUM_SOLENOIDS];
volatile uint8_t activePattern = 0;

int baseStepDuration = 120; // Base duration of each step in milliseconds
bool isBaseStepDurationUpdated = false;

int swingAmount = 30;       // Amount of swing in milliseconds
int relayOnTime = 50;       // Duration the relay stays ON within each step
String sequenceName = "Default";

int currentPattern = 0;
int currentStep = 0;
unsigned long lastStepTime = 0;
unsigned long relayStartTime = 0;
bool relayOn = false;
int currentRelay = 0;
bool isPaused = false;
bool isPausedUpdated = false;
bool isPatternLocked = false; // Flag to indicate if pattern is locked

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

const byte RELAY_PINS[NUM_RELAYS] = {RELAY_PIN_1, RELAY_PIN_2, RELAY_PIN_3, RELAY_PIN_4, RELAY_PIN_5, RELAY_PIN_6, RELAY_PIN_7, RELAY_PIN_8, RELAY_PIN_9, RELAY_PIN_10, RELAY_PIN_11, RELAY_PIN_12, RELAY_PIN_13, RELAY_PIN_14, RELAY_PIN_15, RELAY_PIN_16};
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
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY8 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY9 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY10 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY11 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY12 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY13 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY14 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY15 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}  // RELAY16 pattern
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
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY8 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY9 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY10 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY11 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY12 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY13 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY14 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY15 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}  // RELAY16 pattern
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
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY8 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY9 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY10 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY11 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY12 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY13 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY14 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // RELAY15 pattern
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}  // RELAY16 pattern
    }};

void setupWiFi() {
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
}

void setupFileSystem() {
    if (!SPIFFS.begin(true)) {
        Serial.println("An error occurred while mounting SPIFFS");
        return;
    }
    
    for (int i = 0; i < NUM_PATTERNS; i++) {
        loadPatternFromFile(i);
    }
    loadTimingFromFile();
    loadPinSettingsFromFile();
}

void setupWebServer() {
    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
    server.serveStatic("/static/", SPIFFS, "/static/");
    server.onNotFound(notFound);
    attachRoutes();
    server.begin();
}

void setupSolenoids() {
    for (int i = 0; i < NUM_SOLENOIDS; i++) {
        if (SOLENOID_PINS[i] != -1 && relayActiveStates[i] == 1) {
            pinMode(SOLENOID_PINS[i], OUTPUT);
            digitalWrite(SOLENOID_PINS[i], LOW);
        }
    }
}

void setupClock() {
    uClock.init();
    
    // Set the callback functions
    uClock.setOnPPQN(onPPQNCallback);
    uClock.setOnStep(onStepCallback);  
    uClock.setOnClockStart(onClockStart);  
    uClock.setOnClockStop(onClockStop);
    
    // Set the clock BPM
    uClock.setTempo(baseStepDuration);

    // Load the initial pattern into the sequencer
    loadPatternData(activePattern);

    // Start the sequencer
    if (!isPaused) {
        uClock.start();
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Setting up WiFi...");
    setupWiFi();
    Serial.println("Setting up file system...");
    setupFileSystem();
    Serial.println("Setting up web server...");
    setupWebServer();
    Serial.println("Setting up solenoids...");
    setupSolenoids();
    Serial.println("Setting up clock...");
    setupClock();
    Serial.println("Setup complete");
}

void loop()
{
    if (isBaseStepDurationUpdated) {
        uClock.setTempo(baseStepDuration);
        isBaseStepDurationUpdated = false;
    }
}