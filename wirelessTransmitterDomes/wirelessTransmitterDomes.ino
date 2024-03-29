// Uncomment the following line for stress testing
// #define STRESS_TESTING

//button debounce delay (needs to be defined before buttonClass is imported)
#define DEBOUNCE_DELAY 30
//delay before transmitting, allows for multiple buttons to be pressed
#define B_PUSH_DELAY 20
#include <buttonClass.h>
#include <lora.h>
#include <FastLED.h>

//// Tranceiver variables
#define SEND_DELAY 300
#define NUM_SEND_PER_CHANGE 2
lora LORA;
#define BUFLEN 64
char workingBuffer[BUFLEN];

//// Dome variables
#define NUMBER_DOMES 9
#define DOME_1 0
#define DOME_2 1
#define DOME_3 2
#define DOME_4 3
#define DOME_5 4
#define DOME_6 5
#define DOME_7 6
#define DOME_8 7
#define DOME_9 8
#define ALL_OFF 9
#define ALL_ON 10
#define EFFECT_1 11
#define EFFECT_2 12
#define EFFECT_3 13
#define EFFECT_4 14

//// Effect array definitions
#define EFFECT_1_NUMBER_PATTERNS 9
const int pattern_1[EFFECT_1_NUMBER_PATTERNS][NUMBER_DOMES] = {
// 1 2 3 4 5 6 7 8 9
  {0,0,0,0,0,0,0,0,0},
  {0,1,0,0,0,0,0,1,0},
  {0,0,0,0,0,0,0,0,0},
  {1,0,1,0,0,0,1,0,1},
  {0,0,0,0,0,0,0,0,0},
  {0,1,0,0,0,0,0,1,0},
  {1,0,1,0,0,0,1,0,1},
  {0,0,0,1,0,1,0,0,0},
  {0,0,0,0,1,0,0,0,0}
};

#define EFFECT_2_NUMBER_PATTERNS 2
const int pattern_2[EFFECT_2_NUMBER_PATTERNS][NUMBER_DOMES] = {
  {0,0,0,0,0,0,0,0,0},
  {1,0,0,1,1,1,0,0,1}
};
#define EFFECT_3_NUMBER_PATTERNS 2
const int pattern_3[EFFECT_3_NUMBER_PATTERNS][NUMBER_DOMES] = {
// 1 2 3 4 5 6 7 8 9
  {0,0,0,0,0,0,0,0,0},
  {0,0,0,2,0,2,0,0,0}
};

#define EFFECT_4_NUMBER_PATTERNS 2
const int pattern_4[EFFECT_4_NUMBER_PATTERNS][NUMBER_DOMES] = {
  {0,0,0,0,0,0,0,0,0},
  {1,1,1,-1,1,-1,1,1,1}
};



//// Button Array
button buttonArray[] = {
  button(38, 39, 2, true),                        // Dome 1   0
  button(36, 37, 2, true),                        // Dome 2   1
  button(34, 35, 2, true),                        // Dome 3   2
  button(32, 33, 2, true),                        // Dome 4   3
  button(30, 31, 2, true),                        // Dome 5   4
  button(28, 29, 2, true),                        // Dome 6   5
  button(26, 27, 2, true),                        // Dome 7   6
  button(24, 25, 2, true),                        // Dome 8   7
  button(52, 53, 2, true),                        // Dome 9   8
  button(40, 41, 2, true),                        // All off  9
  button(42, 43, 2, true),                        // All on   10
  button(44, 45, EFFECT_1_NUMBER_PATTERNS, false),// Effect 1 11
  button(46, 47, EFFECT_2_NUMBER_PATTERNS, false),// Effect 2 12
  button(48, 49, EFFECT_3_NUMBER_PATTERNS, false),// Effect 3 13
  button(50, 51, EFFECT_4_NUMBER_PATTERNS, false) // Effect 4 14
};

//// TX delay variables
// Time since last send in milliseconds
long lastSendMillis = 0;
// Time since last button press in milliseconds
long lastButtonMillis = 0;
// Number of sends left to do for the current action
int repeatsLeft = 2;


////////////////////////// SETUP ///////////////////////////////
void setup() {
  // Set up the serial port to talk to the computer
  Serial.begin(9600);
  delay(100);

  // Configure and read out the LoRa
  Serial.println("Starting...");
  LORA.send("AT+ADDRESS?");
  LORA.receive(workingBuffer, BUFLEN);
  Serial.println(workingBuffer);
  LORA.send("AT+NETWORKID?");
  LORA.receive(workingBuffer, BUFLEN);
  Serial.println(workingBuffer);
  LORA.send("AT+PARAMETER=10,7,1,7");
  LORA.receive(workingBuffer, BUFLEN);
  Serial.println(workingBuffer);
  LORA.send("AT+PARAMETER?");
  LORA.receive(workingBuffer, BUFLEN);
  Serial.println(workingBuffer);

  // Turn on "All On" and "All off" buttons.  Note that "All off" has priority.
  buttonArray[ALL_OFF].setState(true);
  buttonArray[ALL_ON].setState(true);
}

//////////////////////// LOOP /////////////////////////////
void loop() {
  // Check the button status & update dome states accordingly
  checkButtons();

  // We send out a message if the following conditions apply
  // 	1. It is more than a SEND_DELAY since the last message.
  //	2. It is more than a B_PUSH_DELAY since the last button push (to allow for mutliple buttons to be pushed "at once"
  //	3. We still have message left to send.  (Not sure why we check to see if Lora is not available).
  if (millis() - lastSendMillis > SEND_DELAY &&
      millis() - lastButtonMillis > B_PUSH_DELAY
      && repeatsLeft > 0 && !LORA.available()) {
    lastSendMillis = millis();
    sendData();		// Send the message
    repeatsLeft--;	// We need to send one fewer messages.
  }
  
  // Delay reading confirmation message until after LORA sends it
  // Allows for button presses / state updates while LORA is still sending
  // last state.
  if (millis() - lastSendMillis > 320 && LORA.available()) {
    LORA.receive(workingBuffer, BUFLEN);
    Serial.println(workingBuffer);
  }

  //stress testing code, enabled above in definition
  #ifdef STRESS_TESTING
    if (millis() - lastSendMillis > 1000) {
      repeatsLeft = 1;
      lastSendMillis = millis();
    }
  #endif
}

//////////////////////// checkButtons /////////////////////
void checkButtons() {
  // -1 means no buttons have changed.  >= 0 means the button with that value has changed.
  int buttonChanged = -1;

  // First check all of the effects buttons.  Note we break when we find one with a state change.
  // all other buttons get ignored.  Hence the prioritization order is "All on", "All off", "Effect 1", etc.
  for (int i = ALL_OFF; i <= EFFECT_4; i++) {
    if (buttonArray[i].stateChanged()) {
      buttonChanged = i;
      // A button was pushed, record the time.
      lastButtonMillis = millis();
      break;
    }
  }

  // At least one button changed, get ready to send a message.  Note the message
  // won't go out for about 20 ms.  This is to allow multiple buttons to be
  // pressed "simultaneously".
  if (buttonChanged != -1) {
    repeatsLeft = NUM_SEND_PER_CHANGE;
    int state = 0;

    // Take action based on what button changed.
    switch (buttonChanged) {

      // Turn all buttons off, then turn on the "all on"
      // and "all off" LEDs.
      case ALL_OFF:
        for (int i = DOME_1; i <= EFFECT_4; i++) {
          buttonArray[i].setState(false);
        }
        buttonArray[ALL_OFF].setState(true);
        buttonArray[ALL_ON].setState(true);
        break;

      // Turn all dome buttons on, turn effects buttons off.
      case ALL_ON:
        buttonArray[ALL_ON].setState(true);
        for (int i = DOME_1; i <= DOME_9; i++) {
          buttonArray[i].setState(true);
        }
        for (int i = EFFECT_1; i <= EFFECT_4; i++) {
          buttonArray[i].setState(false);
        }
        break;

      //Relay state from each stored pattern into the domes
      case EFFECT_1:
        state = buttonArray[EFFECT_1].getState();
        for (int i = DOME_1; i <= DOME_9; i++) {
          //check for null character '-1' (if we don't want the state to be overwritten)
          if(pattern_1[state][i] != -1){
            buttonArray[i].setState(pattern_1[state][i]);
          }
        }
        //turn off the other effect buttons
        for (int i = EFFECT_2; i <= EFFECT_4; i++) {
          buttonArray[i].setState(false);
        }
        break;

      //same as above
      case EFFECT_2:
        state = buttonArray[EFFECT_2].getState();
        for (int i = DOME_1; i <= DOME_9; i++) {
          if(pattern_2[state][i] != -1){
            buttonArray[i].setState(pattern_2[state][i]);
          }
        }
        buttonArray[EFFECT_1].setState(false);
        buttonArray[EFFECT_3].setState(false);
        buttonArray[EFFECT_4].setState(false);
        break;
      case EFFECT_3:
        state = buttonArray[EFFECT_3].getState();
        for(int i = DOME_1; i <= DOME_9; i++){
          if(pattern_3[state][i] != -1){
            buttonArray[i].setState(pattern_3[state][i]);
          }
        }
        buttonArray[EFFECT_1].setState(false);
        buttonArray[EFFECT_2].setState(false);
        buttonArray[EFFECT_4].setState(false);
        break;
      case EFFECT_4:
        state = buttonArray[EFFECT_4].getState();
        for(int i = DOME_1; i <= DOME_9; i++){
          if(pattern_4[state][i] != -1){
            buttonArray[i].setState(pattern_4[state][i]);
          }
        }
        buttonArray[EFFECT_1].setState(false);
        buttonArray[EFFECT_2].setState(false);
        buttonArray[EFFECT_3].setState(false);
        break;

      //default output, should never happen but useful for debugging
      default:
        Serial.println("bad button ID");
    }
  } else {

    //if no special buttons are pressed, then check each regular dome button
    //if they're pressed, then toggle the state to reflect button press.
    for (int i = DOME_1; i <= DOME_9; i++) {
      if (buttonArray[i].stateChanged()) {
        repeatsLeft = NUM_SEND_PER_CHANGE;
        lastButtonMillis = millis();
        //turn off all effect buttons
        for (int i = EFFECT_1; i <= EFFECT_4; i++) {
          buttonArray[i].setState(false);
        }
      }
    }
  }
}


///used to compile and send data to tranceiver
void sendData() {
  //get data from button states
  String data = setData();
  //add header
  String result = "AT+SEND=2,";
  //add data
  result += NUMBER_DOMES;
  result += ",";
  result += data;
  //send data as a char array
  LORA.send(result.c_str());
  //relay to serial monitor for debugging
  Serial.println(result);
}

///get data string from dome button states
String setData() {
  String data = "";
  for (int i = DOME_1; i <= DOME_3; i++) {
    if (buttonArray[i].getState()) {
      data += '1';
    } else {
      data += '0';
    }
  }
  if (buttonArray[DOME_5].getState()) {
    data += '1';
  } else {
    data += '0';
  }
  for (int i = DOME_7; i <= DOME_9; i++) {
    if (buttonArray[i].getState()) {
      data += '1';
    } else {
      data += '0';
    }
  }
  switch (buttonArray[DOME_4].getState()){
    case 0:
      data += '0';
      break;
    case 1:
      data += '1';
      break;
    case 2:
      data += '2';
      break;
    default:
      data += '0';
  }
  
  switch (buttonArray[DOME_6].getState()){
    case 0:
      data += '0';
      break;
    case 1:
      data += '1';
      break;
    case 2:
      data += '2';
      break;
    default:
      data += '0';
  }
  
  return data;
}
