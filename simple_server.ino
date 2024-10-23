//
// A simple server implementation showing how to:
//  * serve static messages
//  * read GET and POST parameters
//  * handle missing pages / 404s
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

AsyncWebServer server(80);

const char *ssid = "MANTICORE32";
const char *password = "12345678";

#define NUM_RELAYS 8
#define NUM_STEPS 16
#define NUM_PATTERNS 3

const byte RELAY_PINS[NUM_RELAYS] = {14, 11, 16, 2, 15, 12, 13, 10}; // RELAY1, RELAY2, RELAY3, RELAY4

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
    WiFi.softAP(ssid, password);

    // for (int i = 0; i < NUM_RELAYS; i++) {
    //     pinMode(RELAY_PINS[i], OUTPUT);
    //     digitalWrite(RELAY_PINS[i], HIGH);  // Turn off all relays initially
    // }

    for (int i = 0; i < NUM_PATTERNS; i++)
    {
        loadPatternFromFile(i);
    }

    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());

    if (!SPIFFS.begin(true))
    {
        Serial.println("An error occurred while mounting SPIFFS");
        return;
    }

    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
    server.serveStatic("/static/", SPIFFS, "/static/");

    server.onNotFound(notFound);
    attachRoutes();

    server.begin();
}

void loop()
{
}

void attachRoutes()
{
    Serial.println("Router");
    //   server.enableCORS(false);  // Disable automatic CORS handling
    // server.on("/setPattern", HTTP_OPTIONS, [](AsyncWebServerRequest *request)
    //           {
    // // request->header("Access-Control-Allow-Origin", "http://localhost:5173");
    // // request->header("Access-Control-Allow-Methods", "POST, OPTIONS");
    // // request->header("Access-Control-Allow-Headers", "Content-Type");
    // // request->header("Access-Control-Max-Age", "86400");
    // request->send(204); });
    server.on("/setPattern", HTTP_POST, handleSetPattern);
    server.on("/getPatterns", HTTP_GET, handleGetPatterns);
    server.on("/triggerAllRelays", HTTP_POST, handleTriggerAllRelays);
    server.on("/pause", HTTP_POST, handlePause);
    server.on("/setTiming", HTTP_POST, handleSetTiming);
    server.on("/lockPattern", HTTP_POST, handleLockPattern); // Add lock pattern endpoint
    // server.on("/lockPattern", HTTP_OPTIONS, [](AsyncWebServerRequest *request)
    //           {
    // // request->header("Access-Control-Allow-Origin", "http://localhost:5173");
    // // request->header("Access-Control-Allow-Methods", "POST, OPTIONS");
    // // request->header("Access-Control-Allow-Headers", "Content-Type");
    // // request->header("Access-Control-Max-Age", "86400");
    // request->send(204); });
}

void handleSetPattern(AsyncWebServerRequest *request)
{
    if (!request->hasParam("plain", true))
    {
        request->send(400, "text/plain", "Body not received");
        return;
    }

    String body = request->getParam("plain", true)->value();
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, body);

    if (error)
    {
        request->send(400, "text/plain", "Invalid JSON");
        return;
    }

    if (!doc.containsKey("patternIndex"))
    {
        request->send(400, "text/plain", "Missing patternIndex");
        return;
    }

    int patternIndex = doc["patternIndex"];

    if (patternIndex < 0 || patternIndex >= NUM_PATTERNS)
    {
        request->send(400, "text/plain", "Invalid patternIndex");
        return;
    }

    if (!doc.containsKey("patternData"))
    {
        request->send(400, "text/plain", "Missing patternData");
        return;
    }

    JsonArray patternData = doc["patternData"].as<JsonArray>();

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
}

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
            for (int step = 0; step < NUM_STEPS; step++)
            {
                patternData.add(patterns[i][relay][step]);
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

    if (doc.containsKey("baseStepDuration"))
    {
        baseStepDuration = doc["baseStepDuration"];
    }
    if (doc.containsKey("swingAmount"))
    {
        swingAmount = doc["swingAmount"];
    }
    if (doc.containsKey("relayOnTime"))
    {
        relayOnTime = doc["relayOnTime"];
    }

    request->send(200, "text/plain", "Timing parameters updated successfully");
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
