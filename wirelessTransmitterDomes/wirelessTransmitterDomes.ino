#include <buttonClass.h>
#include <FastLED.h>

#define LORA Serial1

#define B_ALL_ON 11
#define B_ALL_OFF 12
#define B_ALL_ON_LED 13
#define B_ALL_OFF_LED 14
#define NUM_DOMES 9

const int  domeButtonPin[] =   { 2, 3, 4, 5, 6, 7, 8, 9,10};

button domeButtons[] = {button(2, 22),button(3, 23),button(4,24),button(5,25),button(6,26),button(7,27),button(8,28),button(9,29),button(10,30)};
button  effectButtons[] = {button(B_ALL_ON, B_ALL_ON_LED), button(B_ALL_OFF, B_ALL_OFF_LED)};
bool domeState[NUM_DOMES];
bool effectState[sizeof(effectButtons)];

const int  numButtons = sizeof(domeButtons)+sizeof(effectButtons);

long lastSendMillis = 0;
const int sendDelay = 50;

int change = 2;

void setup() {
  LORA.begin(9600);
  //Serial.begin(9600);
  while(!LORA){};
  LORA.print("AT+PARAMETER=10,7,1,7\r\n");
  delay(40);
  
  for(int i = 0; i < NUM_DOMES; i++){
    domeState[i] = false;
  }
  
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
  //sendData();
  if(millis() - lastSendMillis > sendDelay && change > 0){
    lastSendMillis = millis();
    sendData();
    //Serial.println(change);
    change--;
  }
  
  /*if(change){
    //change = false;
    sendData();
  }*/

}

void checkButtons(){
  //change = 0;
  for(int i = 0; i < NUM_DOMES; i++){
    if(domeButtons[i].getState() != domeState[i]){
      domeState[i] = domeButtons[i].getState();
      change = 2;
    }
  }
  
  for(int i = 0; i < sizeof(effectButtons)/sizeof(effectButtons[0]); i++){
    if(effectButtons[i].getState() != effectState[i]){
      if(i == 0){
        effectState[i] = effectButtons[i].getState();
        //Serial.println(effectState[i]);
        for(int i = 0; i < NUM_DOMES; i++){
          domeState[i] = true;
          domeButtons[i].setState(true);
        }
      }else if (i == 1){
        effectState[i] = effectButtons[i].getState();
        //Serial.println(effectState[i]);
        for(int i = 0; i < NUM_DOMES; i++){
          domeState[i] = false;
          domeButtons[i].setState(false);
        }
      }
      //Serial.println(i);
      change = 2;
    }
  }
}

void sendData(){
  String data = "";
  for(int i = 0; i < NUM_DOMES; i++){
    if(domeState[i]){
      data += '1';
    }else{
      data += '0';
    }
  }

  //add special effect bits here

  LORA.print("AT+SEND=2,");
  LORA.print(NUM_DOMES);
  LORA.print(",");
  LORA.print(data);
  LORA.print("\r\n");
  //Serial.println(data);
  while(LORA.available()){
    LORA.read();
  }
}
