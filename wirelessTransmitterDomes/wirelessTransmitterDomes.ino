#include <FastLED.h>

#define LORA Serial1

#define B_ALL_ON 11
#define B_ALL_OFF 12
#define B_ALL_ON_LED 13
#define B_ALL_OFF_LED 14
#define NUM_DOMES 9

const int  domeButtonPin[] =   { 2, 3, 4, 5, 6, 7, 8, 9,10};
const int  domeButtonLED[] =   {22,23,24,25,26,27,28,29,30};
const int  effectButtonPin[] = {B_ALL_ON, B_ALL_OFF};
const int  effectButtonLED[] = {B_ALL_ON_LED, B_ALL_OFF_LED};
const int  numButtons = sizeof(domeButtonPin)+sizeof(effectButtonPin);
long       buttonLastMillis[numButtons];
long lastSendMillis = 0;
bool       domeState[NUM_DOMES];

const int buttonDelay = 120;
const int sendDelay = 50;

int change = 2;

void setup() {
  LORA.begin(9600);
  Serial.begin(9600);
  while(!LORA){};
  LORA.print("AT+PARAMETER=10,7,1,7\r\n");
  delay(40);
  
  for(int i = 0; i < NUM_DOMES; i++){
    pinMode(domeButtonPin[i], INPUT_PULLUP);
    pinMode(domeButtonLED[i], OUTPUT);
    buttonLastMillis[i] = 0;
    domeState[i] = false;
  }
  
  for(int i = 0; i < sizeof(effectButtonPin); i++){
    pinMode(effectButtonPin[i], INPUT_PULLUP);
    pinMode(effectButtonLED[i], OUTPUT);
    buttonLastMillis[i+NUM_DOMES] = 0;
  }

  //insert ready animation or something
}

void loop() {
  checkButtons();
  //sendData();
  if(millis() - lastSendMillis > sendDelay && change > 0){
    lastSendMillis = millis();
    sendData();
    Serial.println(change);
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
    if(digitalRead(domeButtonPin[i]) == LOW && millis() - buttonLastMillis[i] > buttonDelay){
      domeState[i] = !domeState[i];
      buttonLastMillis[i] = millis();
      change = 2;
    }
  }
  for(int i = 0; i < sizeof(effectButtonPin); i++){
    if(digitalRead(effectButtonPin[i]) == LOW && millis() - buttonLastMillis[i+NUM_DOMES] > buttonDelay){
      if(i == 0){
        for(int i = 0; i < NUM_DOMES; i++){
          domeState[i] = true;
        }
      }else if (i == 1){
        for(int i = 0; i < NUM_DOMES; i++){
          domeState[i] = false;
        }
      }
      change = 2;
      buttonLastMillis[i+NUM_DOMES] = millis();
    }
  }
  
  ledUpdate();
}

void ledUpdate(){
  for(int i = 0; i < NUM_DOMES; i++){
    digitalWrite(domeButtonLED[i], domeState[i]);
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
  Serial.println(data);
  while(LORA.available()){
    LORA.read();
  }
}
