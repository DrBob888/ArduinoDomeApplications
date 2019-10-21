#include <SoftwareSerial.h>

#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>

#define LORA_TX_PIN 10   //tx pin on LORA
#define LORA_RX_PIN 9   //rx pin on LORA
#define LCD_TX_PIN  5   //tx pin for LCD
#define LCD_RX_PIN  6   //rx pin for LCD

#define LED_PIN     11   //digital pin for LEDs
#define NUM_LEDS    300  //maximum number of LEDs inside of strip

#define OVERRIDE_PIN 12 //override switch, turns on relay

#define USB Serial
#define BUFLEN 64
char workingBuffer[BUFLEN];

CRGB leds[NUM_LEDS];

// Todo: Create a LORA class.  Replace this line with "LORA myLORA(LORA_TX_PIN, LORA_RX_PIN);
//SoftwareSerial LORA (LORA_TX_PIN, LORA_RX_PIN);
#define LORA Serial1

// Todo: Create an LCD class.  Replace this line with "LCD myLCD(LCD_TX_PIN, LCD_RX_PIN);
SoftwareSerial LCD  (LCD_TX_PIN, LCD_RX_PIN);

int address = 0;
bool overSwitch = false;
bool loraSwitch = false;
bool override = false;
int hue = 0;

/* Sends a command to the LoRa and returns the response in a buffer */
// Todo: Make this a class method
void sendToLora(const char* command, char* response, int buflen) {
  LORA.print(command);  // Send command
  LORA.print("\r\n");   // Append terminator
  int n = LORA.readBytesUntil('\n', response, buflen); // Read the response (terminates in \r\n)
  response[n - 1] = 0; // Replace the last character (\r) with a string terminator
}

// Todo: make this a class method
void readLora(char* response, int buflen) {
  int n = LORA.readBytesUntil('\n', response, buflen); // Read the response (terminates in \r\n)
  response[n - 1] = 0; // Replace the last character (\r) with a string terminator
}

// Todo: make this a class method
void displayLcd(char* message) {
  LCD.write(0xFE);  // Control character
  LCD.write(0x01);  // Clear the screen
  LCD.print(message);
}


// Todo:  Create an address class
void setAddress() {
  if (digitalRead(50)) {
    address = 7;
  } else {
    address = 8;
  }
}

void setup() {
  // Start the serial port connected to the computer
  USB.begin(9600);
  //  while (!USB) {
  //    ;
  //  }
  //USB.println("Initializing");

  // Start the LCD display
  delay(750); // Wait for LCD to be ready
  LCD.begin(9600);
  while (!LCD) {
    ;
  }

  // Start the LoRa connection
  LORA.begin(9600);
  while (!LORA) {
    ;
  }

  // Todo: Replace with myLCD.clear();
  LCD.write(0xFE);
  LCD.write(0x01);


  sendToLora("AT+ADDRESS?", workingBuffer, BUFLEN);
  Serial.println(workingBuffer);
  LCD.print("A=");
  LCD.print(workingBuffer + 9);
  sendToLora("AT+NETWORKID?", workingBuffer, BUFLEN);
  Serial.println(workingBuffer);
  LCD.print(" N=");
  LCD.print(workingBuffer + 11);
  sendToLora("AT+PARAMETER=10,7,1,7", workingBuffer, BUFLEN);
  Serial.println(workingBuffer);
  sendToLora("AT+PARAMETER?", workingBuffer, BUFLEN);
  Serial.println(workingBuffer);
  LCD.print(" P=");
  LCD.print(workingBuffer + 11);


  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(50, INPUT_PULLUP);
  pinMode(OVERRIDE_PIN, INPUT_PULLUP);
  //pinMode(LED_PIN, OUTPUT);

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.clear();
  FastLED.show();

  setAddress();
  LCD.print(" ID=");
  LCD.print(address);

  USB.print("receiver ID: ");
  USB.println(address);
}

bool state = false;
long unsigned int timerCounter = 1000;

void xmitCharacter(char val) {
  String str("AT+SEND=1,1");
  LORA.print(str + val + "\r\n");
}

void loop() {
  // Handle the serial monitor

  // If there are characters available from the xcvr and the Serial port isn't full,
  // then echo the message to the Serial monitor

  if (LORA.available() > 0) {
    //    char rcvd = WLS.read();
    //    Serial.write(rcvd);
    readLora(workingBuffer, BUFLEN);
    Serial.println(workingBuffer);

    char *s1 = strtok(workingBuffer, ",");
    if (0 == strncmp(workingBuffer, "+R", 2)) {
      s1 = strtok(NULL, ",");
      s1 = strtok(NULL, ",");
      displayLcd(s1);

      if (s1[address] == '1') {
        loraSwitch = true;
      } else if (s1[address] == '0') {
        loraSwitch = false;
      }
    }
  }

  if (digitalRead(OVERRIDE_PIN)) {
    fill_solid(leds, NUM_LEDS, CHSV(hue,255,255));
    digitalWrite(13, HIGH);
  }else{
    if(loraSwitch){
      //fill_rainbow(leds, NUM_LEDS, hue);
      fill_solid(leds, NUM_LEDS, CHSV(hue, 255, 255));
      digitalWrite(13, HIGH);
    }else{
      FastLED.clear();
      digitalWrite(13, LOW);
    }
  }
  
  // If there are characters available from the serial monitor, send them to the xcvr
  if (Serial.available() > 0) {
    String str = "AT+" + Serial.readString();  // Append the AT command prefix

    // Strip the newline character off of the end of the string from the monitor
    str.remove(str.length() - 1, 1);

    // Echo the string back to the monitor
    USB.println(str);

    // Send the string the xcvr
    LORA.print(str + "\r\n");
  }
  
  //delay(100);
  //hue++;
  
  
  EVERY_N_MILLISECONDS(20){
    hue+=2;
    FastLED.show();
  }
}
