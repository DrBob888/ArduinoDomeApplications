
#define DEBOUNCE_DELAY 30
#include <buttonClass.h>
#include <lora.h>
#include <FastLED.h>

//// Tranceiver variables
#define SEND_DELAY 300
#define B_PUSH_DELAY 20
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
  {1,1,1,2,1,2,1,1,1}
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
long lastSendMillis = 0;
long lastButtonMillis = 0;
int repeatsLeft = 2;

void setup() {
  Serial.begin(9600);
  delay(100);
  Serial.println("Starting...");
  LORA.send("AT+ADDRESS?", workingBuffer, BUFLEN);
  LORA.receive(workingBuffer, BUFLEN);
  Serial.println(workingBuffer);
  LORA.send("AT+NETWORKID?", workingBuffer, BUFLEN);
  LORA.receive(workingBuffer, BUFLEN);
  Serial.println(workingBuffer);
  LORA.send("AT+PARAMETER=10,7,1,7", workingBuffer, BUFLEN);
  LORA.receive(workingBuffer, BUFLEN);
  Serial.println(workingBuffer);
  LORA.send("AT+PARAMETER?", workingBuffer, BUFLEN);
  LORA.receive(workingBuffer, BUFLEN);
  Serial.println(workingBuffer);
  
  buttonArray[ALL_OFF].setState(true);
  buttonArray[ALL_ON].setState(true);
  
  //insert ready animation or something
}

/*
Here's the logic that we need to execute in the loop:
1. Iterate through all effects buttons for a recent action.  If one has been pressed, do the following
  a. Get the button state
  b. Overwrite the current output string (e.g. 010110102) with the one corresponding to the state of the effect button.
  c. Reset the state for all other effects buttons.
2. If no effect button has been pressed, then interate through all of the dome buttons. For each button that changed state:
  a. Set the relevant character in the output string to match
3. If any button was found to change state, then transmit the character string.
*/
void loop() {
  checkButtons();
  if(millis() - lastSendMillis > SEND_DELAY && 
     millis() - lastButtonMillis > B_PUSH_DELAY 
                            && repeatsLeft > 0 && !LORA.available()){
    lastSendMillis = millis();
    sendData();
    repeatsLeft--;
    //Serial.println(millis()-lastSendMillis);
  }
  if(millis() - lastSendMillis > 320 && LORA.available()){
    LORA.receive(workingBuffer, BUFLEN);
    Serial.println(workingBuffer);
  }
}

void checkButtons(){
  int buttonChanged = -1;
  for(int i = ALL_OFF; i <= EFFECT_4; i++){
    if(buttonArray[i].stateChanged()){
      buttonChanged = i;
      lastButtonMillis = millis();
      break;
    }
  }
  if(buttonChanged != -1){
    repeatsLeft = NUM_SEND_PER_CHANGE;
    int state = 0;
    switch (buttonChanged){
      case ALL_OFF:
        for(int i = DOME_1; i <= EFFECT_4; i++){
          buttonArray[i].setState(false);
        }
        buttonArray[ALL_OFF].setState(true);
        buttonArray[ALL_ON].setState(true);
        break;
        
      case ALL_ON:
        buttonArray[ALL_ON].setState(true);
        for(int i = DOME_1; i <= DOME_9; i++){
          buttonArray[i].setState(true);
        }
        for(int i = EFFECT_1; i <= EFFECT_4; i++){
          buttonArray[i].setState(false);
        }
        break;
        
      case EFFECT_1:
        state = buttonArray[EFFECT_1].getState();
        for(int i = DOME_1; i <= DOME_9; i++){
          buttonArray[i].setState(pattern_1[state][i]);
        }
        for(int i = EFFECT_2; i <= EFFECT_4; i++){
          buttonArray[i].setState(false);
        }
        break;
        
      case EFFECT_2:
        state = buttonArray[EFFECT_2].getState();
        for(int i = DOME_1; i <= DOME_9; i++){
          buttonArray[i].setState(pattern_2[state][i]);
        }
        buttonArray[EFFECT_1].setState(false);
        buttonArray[EFFECT_3].setState(false);
        buttonArray[EFFECT_4].setState(false);
        break;
      case EFFECT_3:
        state = buttonArray[EFFECT_3].getState();
        for(int i = DOME_1; i <= DOME_9; i++){
          buttonArray[i].setState(pattern_3[state][i]);
        }
        buttonArray[EFFECT_1].setState(false);
        buttonArray[EFFECT_2].setState(false);
        buttonArray[EFFECT_4].setState(false);
        break;
      case EFFECT_4:
        state = buttonArray[EFFECT_4].getState();
        for(int i = DOME_1; i <= DOME_9; i++){
          buttonArray[i].setState(pattern_4[state][i]);
        }
        buttonArray[EFFECT_1].setState(false);
        buttonArray[EFFECT_2].setState(false);
        buttonArray[EFFECT_3].setState(false);
        break;
      default:
        Serial.println("bruH");
    }
  }else{
    for(int i = DOME_1; i <= DOME_9; i++){
      if(buttonArray[i].stateChanged()){
        repeatsLeft = NUM_SEND_PER_CHANGE;
        lastButtonMillis = millis();
       for(int i = EFFECT_1; i <= EFFECT_4; i++){
          buttonArray[i].setState(false);
       }
      }
    }
  }
}

void sendData(){
  String data = setData();

  //add special effect bits here
  String result = "AT+SEND=2,";
  result += NUMBER_DOMES;
  result += ",";
  result += data;
  LORA.send(result.c_str(), workingBuffer, BUFLEN);
  Serial.println(result);
  //Serial.println(workingBuffer);
}

String setData(){
  String data = "";
  for(int i = DOME_1; i <= DOME_3; i++){
    if(buttonArray[i].getState()){
      data += '1';
    }else{
      data += '0';
    }
  }
  if(buttonArray[DOME_5].getState()){
    data += '1';
  }else{
    data += '0';
  }
  for(int i = DOME_7; i <= DOME_9; i++){
    if(buttonArray[i].getState()){
      data += '1';
    }else{
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
