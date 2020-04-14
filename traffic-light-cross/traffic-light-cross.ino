// ##############################################################################
//
// LEGO LED Cross-intersection traffic light by Sheldon Maloff
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

// NOTE: I have provided two sequences here. Uncomment the one you prefer, and comment
// out the one that you don't want.

// Sequence One:
// This sequence goes through each light in turn all red, advance green, green, yellow, red.
// If you look at a cross-intersection closely, you'll see there is really no way that you
// could have two lights in the same direction with advance green at the same. The LEGO 
// cars would hit each other making their respective turns. This sequence is the most
// realistic.


int cycleTimes[]      =  { 2000,       10000,                                   1500,                            10000,       1000,         2000,       10000,                                   1500,                            10000,       1000,         2000,       10000,                                   1500,                            10000,       1000,         2000,      10000,                                   1500,                            10000,       1000         };
int leftLightCycle[]  =  { RED_LIGHT,  BLINK | ADVANCE_GREEN_LIGHT | RED_LIGHT, ADVANCE_GREEN_LIGHT | RED_LIGHT, GREEN_LIGHT, YELLOW_LIGHT, RED_LIGHT,  RED_LIGHT,                               RED_LIGHT,                       RED_LIGHT,   RED_LIGHT,    RED_LIGHT,  RED_LIGHT,                               RED_LIGHT,                       RED_LIGHT,   RED_LIGHT,    RED_LIGHT, RED_LIGHT,                               RED_LIGHT,                       RED_LIGHT,   RED_LIGHT    };
int rightLightCycle[] =  { RED_LIGHT,  RED_LIGHT,                               RED_LIGHT,                       RED_LIGHT,   RED_LIGHT,    RED_LIGHT,  BLINK | ADVANCE_GREEN_LIGHT | RED_LIGHT, ADVANCE_GREEN_LIGHT | RED_LIGHT, GREEN_LIGHT, YELLOW_LIGHT, RED_LIGHT,  RED_LIGHT,                               RED_LIGHT,                       RED_LIGHT,   RED_LIGHT,    RED_LIGHT, RED_LIGHT,                               RED_LIGHT,                       RED_LIGHT,   RED_LIGHT    };
int topLightCycle[] =    { RED_LIGHT,  RED_LIGHT,                               RED_LIGHT,                       RED_LIGHT,   RED_LIGHT,    RED_LIGHT,  RED_LIGHT,                               RED_LIGHT,                       RED_LIGHT,   RED_LIGHT,    RED_LIGHT,  BLINK | ADVANCE_GREEN_LIGHT | RED_LIGHT, ADVANCE_GREEN_LIGHT | RED_LIGHT, GREEN_LIGHT, YELLOW_LIGHT, RED_LIGHT, RED_LIGHT,                               RED_LIGHT,                       RED_LIGHT,   RED_LIGHT    };
int bottomLightCycle[] = { RED_LIGHT,  RED_LIGHT,                               RED_LIGHT,                       RED_LIGHT,   RED_LIGHT,    RED_LIGHT,  RED_LIGHT,                               RED_LIGHT,                       RED_LIGHT,   RED_LIGHT,    RED_LIGHT,  RED_LIGHT,                               RED_LIGHT,                       RED_LIGHT,   RED_LIGHT,    RED_LIGHT, BLINK | ADVANCE_GREEN_LIGHT | RED_LIGHT, ADVANCE_GREEN_LIGHT | RED_LIGHT, GREEN_LIGHT, YELLOW_LIGHT };


// Sequence Two:
// This sequence goes through each light in turn all red, advance green, green, yellow, red.
// Both lanes in the same road go through the same sequence while the cross traffic remains
// on red. Then the both lanes of the cross traffic go through the sequence. 

/*
int cycleTimes[]      =  { 2000,       10000,                                   1500,                            10000,       1000,         1000,      10000,                                   1500,                            10000,       1000,        };
int leftLightCycle[]  =  { RED_LIGHT,  BLINK | ADVANCE_GREEN_LIGHT | RED_LIGHT, ADVANCE_GREEN_LIGHT | RED_LIGHT, GREEN_LIGHT, YELLOW_LIGHT, RED_LIGHT, RED_LIGHT,                               RED_LIGHT,                       RED_LIGHT,   RED_LIGHT    };  
int rightLightCycle[] =  { RED_LIGHT,  BLINK | ADVANCE_GREEN_LIGHT | RED_LIGHT, ADVANCE_GREEN_LIGHT | RED_LIGHT, GREEN_LIGHT, YELLOW_LIGHT, RED_LIGHT, RED_LIGHT,                               RED_LIGHT,                       RED_LIGHT,   RED_LIGHT    };
int topLightCycle[] =    { RED_LIGHT,  RED_LIGHT,                               RED_LIGHT,                       RED_LIGHT,   RED_LIGHT,    RED_LIGHT, BLINK | ADVANCE_GREEN_LIGHT | RED_LIGHT, ADVANCE_GREEN_LIGHT | RED_LIGHT, GREEN_LIGHT, YELLOW_LIGHT };
int bottomLightCycle[] = { RED_LIGHT,  RED_LIGHT,                               RED_LIGHT,                       RED_LIGHT,   RED_LIGHT,    RED_LIGHT, BLINK | ADVANCE_GREEN_LIGHT | RED_LIGHT, ADVANCE_GREEN_LIGHT | RED_LIGHT, GREEN_LIGHT, YELLOW_LIGHT };
*/

// Determines the number of cycles in the arrays
const int maximumTrafficLightCycles = sizeof(cycleTimes) / sizeof(int);

// NOTE: The two event booleans that follow are poor-man's event signalling. Proper
// event signalling would use arrays of callback fuctions. I'm trying to keep the 
// code simple however, so anyone can understand it.

bool cycleEvent = false;  // This boolean turns true for one loop every time the current cycle elapses
bool blinkEvent = false;  // This boolean turns true for one loop every time the BLINK_PERIOD has elapsed
bool blinkState = HIGH;   // Indicates whether a blinking light is on or off

// These lines are the columns of the matrix, the anodes
#define RED_LIGHT_LINE            5  
#define YELLOW_LIGHT_LINE         4
#define GREEN_LIGHT_LINE          3
#define ADVANCE_GREEN_LIGHT_LINE  2

// These lines are the rows of the matrix, the common-cathodes
#define LEFT_LAMP_POST_LINE       6  
#define TOP_LAMP_POST_LINE        7
#define RIGHT_LAMP_POST_LINE      8
#define BOTTOM_LAMP_POST_LINE     9

void setup() {                
  // initialize output pins
  pinMode(RED_LIGHT_LINE, OUTPUT);     
  pinMode(YELLOW_LIGHT_LINE, OUTPUT);     
  pinMode(ADVANCE_GREEN_LIGHT_LINE, OUTPUT);     
  pinMode(GREEN_LIGHT_LINE, OUTPUT);     
  pinMode(LEFT_LAMP_POST_LINE, OUTPUT);     
  pinMode(RIGHT_LAMP_POST_LINE, OUTPUT);     
  pinMode(TOP_LAMP_POST_LINE, OUTPUT);     
  pinMode(BOTTOM_LAMP_POST_LINE, OUTPUT);     
  pinMode(BUILT_IN_LIGHT, OUTPUT);     
  testLights();
}

void loop() {
  // the lamp post currently being lit
  static int lampPost = BOTTOM_LAMP_POST_LINE;

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
// (This is a cycleEvent listener.)
void ensureAdvanceGreenIsOnForNewCycle() {
  if (cycleEvent) blinkState = HIGH;
}

// Provides a visual heartbeat by blinking the built-in LED every time there is
// a new cycle. (This is both a cycleEvent and blinkEvent listener.)
void heartbeat() {
  if (cycleEvent)
    digitalWrite(BUILT_IN_LIGHT, HIGH); 
  else if (blinkEvent) 
    digitalWrite(BUILT_IN_LIGHT, LOW);
}

// Keeps track of the two timers: cycle timer and blink timer. When each timer elapses
// past a certain value, this function signals the event by raising one or both of the
// cycleEvent or blinkEvent booleans true. Other functions can listen to these one or
// both of these events to perform additional processing. The next call to tickTock() 
// will reset the event the event signals.
void tickTock(int cycle) {
  // Keep track of the amount of time that has elapsed so we know ehen to signal an event.
  static int currentCycleTime = 0;
  static int currentBlinkTime = 0;
  
  // This keeps the LEDs of a lamp post lit for a small period of time.
  delay(DISPLAY_PERIOD);

  // Update the cycle time and possibly signal an event.
  currentCycleTime += DISPLAY_PERIOD;
  if (currentCycleTime >= cycleTimes[cycle]) {
    cycleEvent = true;     // indicate a cycle has elapsed
    currentCycleTime = 0;  // reset the timer
  } else {
    cycleEvent = false;
  }
  
  // Update the blink time and possibly signal an event.
  currentBlinkTime += DISPLAY_PERIOD;
  if (currentBlinkTime >= BLINK_PERIOD) {
    blinkEvent = true;
    currentBlinkTime = 0;
    // flip the state of a blinking light
    if (blinkState == HIGH) 
      blinkState = LOW;
    else 
      blinkState = HIGH;
  } else {
    blinkEvent = false;
  }
}

// Returns the cycle that we are on for this display period.
// (This is a cycleEvent listener.)
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
  // runs LEFT_LAMP_POST_LINE, TOP_LAMP_POST_LINE, RIGHT_LAMP_POST_LINE, BOTTOM_LAMP_POST_LINE
  // The order of lighting is meaningless with a high refresh rate, but becomes visible at
  // lower refresh rates.
  static int currentLampPost = BOTTOM_LAMP_POST_LINE;

  switch (currentLampPost) {
    case LEFT_LAMP_POST_LINE:
      currentLampPost = TOP_LAMP_POST_LINE;
      break;
    case TOP_LAMP_POST_LINE:
      currentLampPost = RIGHT_LAMP_POST_LINE;
      break;
    case RIGHT_LAMP_POST_LINE:
      currentLampPost = BOTTOM_LAMP_POST_LINE;
      break;
    case BOTTOM_LAMP_POST_LINE:
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
      digitalWrite(ADVANCE_GREEN_LIGHT_LINE, blinkState);
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
    case TOP_LAMP_POST_LINE:
      return topLightCycle[cycle];
    case RIGHT_LAMP_POST_LINE:
      return rightLightCycle[cycle];
    case BOTTOM_LAMP_POST_LINE:
      return bottomLightCycle[cycle];
  }
  return 0; // should not get here, represents all lights off
}

// Cycle through all trafic lights turning each on for a delay.
// All lights will be off at the end of the test.
void testLights() {
  allLightsOff();
  
  allLightsOnFor(LEFT_LAMP_POST_LINE);
  testWait();

  allLightsOnFor(TOP_LAMP_POST_LINE);
  testWait();

  allLightsOnFor(RIGHT_LAMP_POST_LINE);
  testWait();

  allLightsOnFor(BOTTOM_LAMP_POST_LINE);
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
  digitalWrite(TOP_LAMP_POST_LINE, HIGH);
  digitalWrite(BOTTOM_LAMP_POST_LINE, HIGH);

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
