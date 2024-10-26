
// Solenoid configuration
// Active pattern index
volatile uint8_t requestedPattern = 0;
volatile bool patternChangeRequested = false;

// Make sure all sequencer data are modified atomically
#define ATOMIC(X) noInterrupts(); X; interrupts();

// Shared data
bool _playing = false;
uint16_t _step = 0;

// Solenoid control variables

// Sequencer data structure
typedef struct
{
  uint16_t solenoidStates;  // Each bit represents the state of a solenoid in this step (bit 0 for solenoid 0, bit 1 for solenoid 1, etc.)
  bool rest;               // If true, no solenoids are activated during this step
} SEQUENCER_STEP_DATA;

// Main sequencer data
SEQUENCER_STEP_DATA _sequencer[STEP_MAX_SIZE];
uint16_t _step_length = STEP_MAX_SIZE;




// Function to load pattern data into the sequencer
void loadPatternData(uint8_t patternIndex) {
  ATOMIC(
    for (uint16_t step = 0; step < STEP_MAX_SIZE; step++) {
      uint16_t solenoidStates = 0;
      for (int solenoid = 0; solenoid < NUM_SOLENOIDS; solenoid++) {
        if (patterns[patternIndex][solenoid][step] == 1) {
          solenoidStates |= (1 << solenoid);
        }
      }
      _sequencer[step].solenoidStates = solenoidStates;
      _sequencer[step].rest = (solenoidStates == 0);
    }
  );
}

// Function to request a pattern change
void setActivePattern(uint8_t newPattern) {
  if (newPattern != activePattern && newPattern < NUM_PATTERNS) {
    ATOMIC(
      requestedPattern = newPattern;
      patternChangeRequested = true;
    );
  }
}

void decideNextPattern() {
  // Check if a pattern change has been requested
  if (randomMode) {
    // Create array of enabled patterns with their probabilities
    int enabledPatterns[NUM_PATTERNS];
    int totalProb = 0;
    int numEnabled = 0;
    
    for (int i = 0; i < NUM_PATTERNS; i++) {
        if (patternPlaylist[i].enabled) {
            enabledPatterns[numEnabled] = i;
            totalProb += patternPlaylist[i].probability;
            numEnabled++;
        }
    }
    if (numEnabled > 0) {
        // Pick random number between 0 and total probability
        int r = random(0, totalProb);
        int sum = 0;
        
        // Find which pattern this random number corresponds to
        for (int i = 0; i < numEnabled; i++) {
            sum += patternPlaylist[enabledPatterns[i]].probability;
            if (r < sum) {
                activePattern = enabledPatterns[i];
                break;
            }
        }
    }
    loadPatternData(activePattern);
    patternChangeRequested = false;
  }else{
    if (patternChangeRequested) {
      activePattern = requestedPattern;
      loadPatternData(activePattern);
      patternChangeRequested = false;
    }
    else {
      // Find next enabled pattern, or keep current if all disabled
      bool foundEnabled = false;
      int startPattern = activePattern;
      
      do {
        activePattern++;
        if (activePattern >= NUM_PATTERNS) {
          activePattern = 0;
        }
        if (patternPlaylist[activePattern].enabled) {
          foundEnabled = true;
          break;
        }
      } while (activePattern != startPattern);
      
      // Only load if we found an enabled pattern
      if (foundEnabled) {
        loadPatternData(activePattern);
      } else {
        // Reset to original pattern if none enabled
        isPaused = true;
        uClock.stop();
      }
    }
  }
}

// The callback function called by uClock each Pulse of 16 PPQN clock resolution.
void onStepCallback(uint32_t tick) 
{
  // Get the current step
  _step = tick % _step_length;

  if (_step == 0) {
    decideNextPattern();
  }
  // Activate the solenoids if this step is not a rest
  if (!_sequencer[_step].rest) {
    uint16_t solenoidStates = _sequencer[_step].solenoidStates;

    for (int i = 0; i < NUM_SOLENOIDS; i++) {
      if (solenoidStates & (1 << i)) {
        if (relayActiveStates[i] == 1 && SOLENOID_PINS[i] != -1 && !isPaused) {
          digitalWrite(SOLENOID_PINS[i], HIGH); // Turn on the solenoid
          solenoid_lengths[i] = patternPlaylist[activePattern].trackVelocities[i];    // Set the length based on step velocity
        }
      }
    }
  }  
}

// The callback function called by uClock each Pulse of 96 PPQN clock resolution.
void onPPQNCallback(uint32_t tick) 
{
  // Handle solenoid timing
  for (int i = 0; i < NUM_SOLENOIDS; i++) {
    if (solenoid_lengths[i] > 0) {
      solenoid_lengths[i]--;
      if (solenoid_lengths[i] == 0) {
        if (relayActiveStates[i] == 1 && SOLENOID_PINS[i] != -1) {
          digitalWrite(SOLENOID_PINS[i], LOW); // Turn off the solenoid
        }
      }
    }
  }
}

// The callback function called when clock starts using Clock.start() method.
void onClockStart() 
{
  _playing = true;
}

// The callback function called when clock stops using Clock.stop() method.
void onClockStop() 
{
  // Ensure all solenoids are turned off
  for (int i = 0; i < NUM_SOLENOIDS; i++) {
    if (relayActiveStates[i] == 1 && SOLENOID_PINS[i] != -1) {
      digitalWrite(SOLENOID_PINS[i], LOW);
      solenoid_lengths[i] = -1;
    }
  }
  _playing = false;
}