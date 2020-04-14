// ##############################################################################
//
// LEGO LED T-intersection traffic light by Sheldon Maloff
// licensed under a Creative Commons Attribution-ShareAlike 3.0 Unported License.
// http://creativecommons.org/licenses/by-sa/3.0/  
//
// ##############################################################################


// ------------------------------------------------------------------------------
//
// Some future ideas:
//
// - Add an input button that slows the DISPLAY_PERIOD to the point where the 
//   cycling of the lights is very obvious and then ramps back to 60Hz to show
//   how LED multiplexing works.
//
// - Add an input that shows all red lights for a fire truck to go through.
//
// - Add an input that causes the light to go into fail-safe mode, where all
//   directions are flashing red
//
// ------------------------------------------------------------------------------


// The LED that is built into the Arduino board used as a heart beat every cycle.
#define BUILT_IN_LIGHT       13 

#define TEST_ON_TIMEOUT      5000    // 5s on
#define TEST_OFF_TIMEOUT     100     // 1/10s off

// The rate at which the advance green blinks (ms), 50% duty cycle
#define BLINK_PERIOD         150     

// Each lamp post is on for this number of miliseconds and off for twice this
// number of milliseconds (total of three lights) so the refresh rate is about
// 1000 / 15 = 66Hz
#define DISPLAY_PERIOD       5      

// Bits used in the state-table of lamp post cycles that follows.
#define RED_LIGHT            B0001000   
#define YELLOW_LIGHT         B0000100
#define ADVANCE_GREEN_LIGHT  B0000010
#define GREEN_LIGHT          B0000001
// Bit to indicate advance green light will blink.
#define BLINK                B1000000

// State tables, one for each traffic light, as well as the time for each cyle
// Read through the four arrays vertically to see one cycle.
int cycleTimes[]      = { 2000,       10000,       3000,         2000,      10000,                                    1500,                              1000,        10000,       3000         };
int leftLightCycle[]  = { RED_LIGHT,  RED_LIGHT,   RED_LIGHT,    RED_LIGHT, BLINK |ADVANCE_GREEN_LIGHT | GREEN_LIGHT, ADVANCE_GREEN_LIGHT | GREEN_LIGHT, GREEN_LIGHT, GREEN_LIGHT, YELLOW_LIGHT };
int rightLightCycle[] = { RED_LIGHT,  RED_LIGHT,   RED_LIGHT,    RED_LIGHT, RED_LIGHT,                                RED_LIGHT,                         RED_LIGHT,   GREEN_LIGHT, YELLOW_LIGHT };
int crossLightCycle[] = { RED_LIGHT,  GREEN_LIGHT, YELLOW_LIGHT, RED_LIGHT, RED_LIGHT,                                RED_LIGHT,                         RED_LIGHT,   RED_LIGHT,   RED_LIGHT    };

const int maximumTrafficLightCycles = sizeof(cycleTimes) / sizeof(int);

bool isAdvanceGreenBlinkOn = true;     // Indicates whether the left advance green light is on or off
bool blinkEvent = false;               // This boolean turns true for one loop every time the BLINK_PERIOD has elapsed
bool cycleEvent = false;               // This boolean turns true for one loop every time the current cycle elapses

// These lines are the columns of the matrix, the anodes
#define RED_LIGHT_LINE            5  
#define YELLOW_LIGHT_LINE         4
#define GREEN_LIGHT_LINE          3
#define ADVANCE_GREEN_LIGHT_LINE  2

// These lines are the rows of the matrix, the common-cathodes
#define LEFT_LAMP_POST_LINE       6  
#define CROSS_LAMP_POST_LINE      7
#define RIGHT_LAMP_POST_LINE      8

void setup() {                
  // initialize output pins
  pinMode(RED_LIGHT_LINE, OUTPUT);     
  pinMode(YELLOW_LIGHT_LINE, OUTPUT);     
  pinMode(ADVANCE_GREEN_LIGHT_LINE, OUTPUT);     
  pinMode(GREEN_LIGHT_LINE, OUTPUT);     
  pinMode(LEFT_LAMP_POST_LINE, OUTPUT);     
  pinMode(RIGHT_LAMP_POST_LINE, OUTPUT);     
  pinMode(CROSS_LAMP_POST_LINE, OUTPUT);     
  pinMode(BUILT_IN_LIGHT, OUTPUT);     
  testLights();
}

void loop() {
  // the lamp post currently being lit
  static int lampPost = RIGHT_LAMP_POST_LINE;

  // allow house keeping once every cycle event 
  do {
    heartbeat();
  
    ensureAdvanceGreenIsOnForNewCycle();
    
    int cycle = getTrafficLightCycle();
    int nextLampPost = getLampPostForThisDisplayPeriod();
  
    turnOffLampPost(lampPost);
    showLightsForLampPostCycle(nextLampPost, cycle);
    lampPost = nextLampPost;
    
    tickTock(cycle);
  } while (!cycleEvent);
}

// Ensure that an advance green starts blinking by being on every new cycle
void ensureAdvanceGreenIsOnForNewCycle() {
  if (cycleEvent) isAdvanceGreenBlinkOn = true;
}

// Provides a visual heartbeat by blinking the built-in LED every time there is
// a new cycle.
void heartbeat() {
  if (cycleEvent)
    digitalWrite(BUILT_IN_LIGHT, HIGH); 
  else if (blinkEvent) 
    digitalWrite(BUILT_IN_LIGHT, LOW);
}

// Keeps track of the two timers: cycle timer and blink timer. When each timer elapses
// past a certain value, this function signals the event by raising one or both of the
// cycleEvent or blinkEvent booleans true. Code in loop() can use that true signal to
// perform processing. The next call to tickTock() will reset the event that fired
// so loop() doesn't have to keep track of that either.
void tickTock(int cycle) {
  static int currentCycleTime = 0;  // The amount of time that has elapsed in the current light cycle.
  static int currentBlinkTime = 0;  // The amount time that has elapsed before the next blinkEvent.
  
  delay(DISPLAY_PERIOD);

  currentCycleTime += DISPLAY_PERIOD;
  if (currentCycleTime >= cycleTimes[cycle]) {
    cycleEvent = true;     // indicate a cycle has elapsed
    currentCycleTime = 0;  // reset the timer
  } else {
    cycleEvent = false;
  }
  
  currentBlinkTime += DISPLAY_PERIOD;
  if (currentBlinkTime >= BLINK_PERIOD) {
    blinkEvent = true;
    currentBlinkTime = 0;
  } else {
    blinkEvent = false;
  }
}

// Returns the cycle that we are on for this display period.
int getTrafficLightCycle() {
  // The current traffic light cycle - runs 0 to maximumTrafficLightCycles - 1
  static int currentTrafficLightCycle = 0;    

  if (cycleEvent) {
    // advance to the next cycle, with wrap
    currentTrafficLightCycle += 1;     
    if (currentTrafficLightCycle >= maximumTrafficLightCycles) currentTrafficLightCycle = 0;
  }
  
  return currentTrafficLightCycle;
}

// Returns the lamp post that is to be lit during this display period.
// Every display period one lamp post gets a chance to light.
int getLampPostForThisDisplayPeriod() {
  // The traffic light that is lit for this display period
  // runs LEFT_LAMP_POST_LINE, CROSS_LAMP_POST_LINE, RIGHT_LAMP_POST_LINE
  static int currentLampPost = RIGHT_LAMP_POST_LINE;

  switch (currentLampPost) {
    case LEFT_LAMP_POST_LINE:
      currentLampPost = CROSS_LAMP_POST_LINE;
      break;
    case CROSS_LAMP_POST_LINE:
      currentLampPost = RIGHT_LAMP_POST_LINE;
      break;
    case RIGHT_LAMP_POST_LINE:
      currentLampPost = LEFT_LAMP_POST_LINE;
      break;
  }
  
  return currentLampPost;
}

// Shows the lights in the specified cycle for the current display period.
void showLightsForLampPostCycle(int lampPost, int cycle) {
  int lightState = getLightStateFor(lampPost, cycle);
  
  // Turn on the appropriate light
  digitalWrite(RED_LIGHT_LINE, ((lightState & RED_LIGHT)) ? HIGH : LOW);
  digitalWrite(YELLOW_LIGHT_LINE, (lightState & YELLOW_LIGHT) ? HIGH : LOW);
  digitalWrite(GREEN_LIGHT_LINE, (lightState & GREEN_LIGHT) ? HIGH : LOW);
  
  // Advance greens are complicated by the fact they could blink.
  if (lightState & ADVANCE_GREEN_LIGHT) {
    if (lightState & BLINK) {
      digitalWrite(ADVANCE_GREEN_LIGHT_LINE, (isAdvanceGreenBlinkOn) ? HIGH : LOW);
      if (blinkEvent) isAdvanceGreenBlinkOn = !isAdvanceGreenBlinkOn;  // flip state of the advance green  
    } else {
      digitalWrite(ADVANCE_GREEN_LIGHT_LINE, HIGH);
    }    
  } else {
    digitalWrite(ADVANCE_GREEN_LIGHT_LINE, LOW);
  }
  
  // Drive the lamp post low to light things up
  digitalWrite(lampPost, LOW);
}

// Turns off the inidicated lamp post. Purely to make self-documenting code.
void turnOffLampPost(int lampPost) {
  digitalWrite(lampPost, HIGH);
}

// Returns the light state for the specified lamp post and traffic light cycle.
int getLightStateFor(int lampPost, int cycle) {
  switch (lampPost) {
    case LEFT_LAMP_POST_LINE: 
      return leftLightCycle[cycle];
    case CROSS_LAMP_POST_LINE:
      return crossLightCycle[cycle];
    case RIGHT_LAMP_POST_LINE:
      return rightLightCycle[cycle];
  }
  return 0; // should not get here, represents all lights off
}

// Cycle through all trafic lights turning each on for a delay.
// All lights will be off at the end of the test.
void testLights() {
  allLightsOff();
  
  allLightsOnFor(LEFT_LAMP_POST_LINE);
  testWait();

  allLightsOnFor(CROSS_LAMP_POST_LINE);
  testWait();

  allLightsOnFor(RIGHT_LAMP_POST_LINE);
  testWait();
}

// Keeps the current lights on for a time, turns off all lights
// and then waits for a time.
void testWait() {
  delay(TEST_ON_TIMEOUT);
  allLightsOff();
  delay(TEST_OFF_TIMEOUT);
}

// Set pins so all lights are off..
void allLightsOff() {
  digitalWrite(LEFT_LAMP_POST_LINE, HIGH);
  digitalWrite(RIGHT_LAMP_POST_LINE, HIGH);
  digitalWrite(CROSS_LAMP_POST_LINE, HIGH);

  digitalWrite(RED_LIGHT_LINE, LOW);
  digitalWrite(YELLOW_LIGHT_LINE, LOW);
  digitalWrite(ADVANCE_GREEN_LIGHT_LINE, LOW);
  digitalWrite(GREEN_LIGHT_LINE, LOW);
}

// Turn all light on for a given traffic light.
void allLightsOnFor(int lampPost) {
  digitalWrite(RED_LIGHT_LINE, HIGH);
  digitalWrite(YELLOW_LIGHT_LINE, HIGH);
  digitalWrite(ADVANCE_GREEN_LIGHT_LINE, HIGH);
  digitalWrite(GREEN_LIGHT_LINE, HIGH);
  
  digitalWrite(lampPost, LOW);
}
