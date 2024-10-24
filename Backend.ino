
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
        uClock.stop();
    }
    else
    {
        uClock.start();
    }

    request->send(200, "text/plain", isPaused ? "Sequencer paused" : "Sequencer resumed");
}

void saveTimingToFile()
{
    File file = SPIFFS.open("/timing_params.csv", FILE_WRITE);
    if (file)
    {
        file.println("baseStepDuration,swingAmount,relayOnTime,sequenceName,isPaused");
        file.printf("%d,%d,%d,%s,%d\n", baseStepDuration, swingAmount, relayOnTime, sequenceName.c_str(), isPaused);
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
        Serial.println("Creating default timing_params.csv");
        
        // Create file with default values
        File newFile = SPIFFS.open("/timing_params.csv", FILE_WRITE);
        if (newFile) {
            const int defaultBaseStepDuration = baseStepDuration;
            const int defaultSwingAmount = swingAmount;
            const int defaultRelayOnTime = relayOnTime;
            const char* defaultSequenceName = sequenceName.c_str();
            const bool defaultIsPaused = isPaused;

            newFile.println("baseStepDuration,swingAmount,relayOnTime,sequenceName,isPaused");
            newFile.printf("%d,%d,%d,%s,%d\n", defaultBaseStepDuration, defaultSwingAmount, defaultRelayOnTime, defaultSequenceName, isPaused);
            newFile.close();
            
            // Set default values
            baseStepDuration = defaultBaseStepDuration;
            swingAmount = defaultSwingAmount;
            relayOnTime = defaultRelayOnTime;
            sequenceName = defaultSequenceName;
            isPaused = defaultIsPaused;

            Serial.println("Default timing parameters created");
        } else {
            Serial.println("Failed to create timing_params.csv");
        }
        return;
    }

    // Skip the header line
    file.readStringUntil('\n');

    // Read the values
    String line = file.readStringUntil('\n');
    int commaIndex1 = line.indexOf(',');
    int commaIndex2 = line.indexOf(',', commaIndex1 + 1);
    int commaIndex3 = line.indexOf(',', commaIndex2 + 1);
    int commaIndex4 = line.indexOf(',', commaIndex3 + 1);

    if (commaIndex1 != -1 && commaIndex2 != -1 && commaIndex3 != -1 && commaIndex4 != -1)
    {
        baseStepDuration = line.substring(0, commaIndex1).toInt();
        swingAmount = line.substring(commaIndex1 + 1, commaIndex2).toInt();
        relayOnTime = line.substring(commaIndex2 + 1, commaIndex3).toInt();
        sequenceName = line.substring(commaIndex3 + 1);
        sequenceName.trim(); // Remove any trailing whitespace
        isPaused = line.substring(commaIndex4 + 1).toInt();
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
        uClock.setTempo(baseStepDuration);
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
    if (doc.containsKey("sequenceName"))
    {
        sequenceName = doc["sequenceName"].as<String>();
        updated = true;
    }
    if (doc.containsKey("isPaused"))
    {
        isPaused = doc["isPaused"].as<bool>();
        updated = true;
        if (isPaused) {
            uClock.stop();
        } else {
            uClock.start();
        }
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

AsyncCallbackJsonWebHandler* setSettingsHandler = new AsyncCallbackJsonWebHandler("/setSettings", [](AsyncWebServerRequest *request, JsonVariant &json) {
    JsonObject jsonObj = json.as<JsonObject>();

    if (!jsonObj.containsKey("tempo") || !jsonObj.containsKey("swing") || !jsonObj.containsKey("velocity") || !jsonObj.containsKey("sequenceName")) {
        request->send(400, "text/plain", "Missing required timing parameters");
        return;
    }

    baseStepDuration = jsonObj["tempo"];
    swingAmount = jsonObj["swing"];
    relayOnTime = jsonObj["velocity"];
    sequenceName = jsonObj["sequenceName"].as<String>();

    saveTimingToFile();
    request->send(200, "text/plain", "Timing settings updated successfully");
});

void handleGetSettings(AsyncWebServerRequest *request) {
    StaticJsonDocument<200> doc;
    doc["tempo"] = baseStepDuration;
    doc["swing"] = swingAmount;
    doc["velocity"] = relayOnTime;
    doc["sequenceName"] = sequenceName;
    doc["isPaused"] = isPaused;
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}


void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
}


void attachRoutes()
{
    Serial.println("Router");
    server.addHandler(setPatternHandler);
    server.addHandler(setSettingsHandler);
    server.on("/getSettings", HTTP_GET, handleGetSettings);
    server.on("/getPatterns", HTTP_GET, handleGetPatterns);
    server.on("/triggerAllRelays", HTTP_POST, handleTriggerAllRelays);
    server.on("/pause", HTTP_POST, handlePause);
    server.on("/setTiming", HTTP_POST, handleSetTiming);
    server.on("/lockPattern", HTTP_POST, handleLockPattern); // Add lock pattern endpoint
}
