# C0R3SEQ32

## Install ESP32 Platform

1. Open Arduino IDE
2. Go to `File` > `Preferences`
3. Add the following URL to the `Additional Boards Manager URLs` field:
   - `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`

## Installation

1. **Navigate to the Tools Directory**:
   - Open Finder and go to `/Users/your-username/Documents/Arduino/`.
   - Inside this directory, create a new folder named `tools` if it doesn't already exist.

2. **Extract the ZIP File**:
   - Extract the contents of `ESP32FS-1.0.zip` into the `tools` directory.
   - Ensure the structure is correct: the path should look like `/Users/your-username/Documents/Arduino/tools/ESP32FS/tool/esp32fs.jar`.

```sh
/Users/manticore/Documents/Arduino/tools/
└── ESP32FS
    └── tool
        └── esp32fs.jar
```

3. **Restart the Arduino IDE**:
   - Close and reopen the Arduino IDE to load the new tool.

4. **Verify Installation**:
   - Open a sketch for an ESP32 board.
   - Go to `Tools` in the menu bar.
   - You should see an option like `ESP32 Sketch Data Upload`. This indicates the tool is installed correctly.


By following these steps, you should have the ESP32FS tool properly installed in your Arduino IDE on macOS.

5. **Install Libraries**:
   - Open the Arduino IDE.
   - Navigate to `Sketch` > `Include Library` > `Add .ZIP Library...`.
   - Select the ZIP files you want to install from `/Users/manticore/Documents/Arduino/C0R3SEQ32/libs/`.
   - The libraries will be added to your Arduino libraries.

   Install the following libraries:
   - `AsyncTCP-master.zip`
   - `ESPAsyncWebServer-master.zip`
   - `uClock-2.1.0.zip`

   After installation, you can include these libraries in your sketches by using `#include` statements.