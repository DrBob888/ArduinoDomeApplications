#include <SoftwareSerial.h>

#define LORA_TX_PIN 2  //tx pin on LORA
#define LORA_RX_PIN 3  //rx pin on LORA
#define LCD_TX_PIN 5   //tx pin for LCD
#define LCT_RX_PIN 4   //rx pin for LCD

#define OVERRIDE_PIN 5 //override switch, turns on relay

#define USB Serial
#define BUFLEN 64
char workingBuffer[BUFLEN];


// Todo: Create a LORA class.  Replace this line with "LORA myLORA(LORA_TX_PIN, LORA_RX_PIN);
SoftwareSerial LORA (LORA_TX_PIN, LORA_RX_PIN);

// Todo: Create an LCD class.  Replace this line with "LCD myLCD(LCD_TX_PIN, LCD_RX_PIN);
SoftwareSerial LCD  (LCD_TX_PIN,LCD_RX_PIN);

int address = 0;
bool overSwitch = false;
bool loraSwitch = false;
bool override = false;

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
void setAddress(){
  int b1 = digitalRead(A0);
  int b2 = digitalRead(A1);
  int b3 = digitalRead(A2);
  int x = !b1;
  x = x<<1 | !b2;
  x = x<<1 | !b3;
  address = x;
}

void setup() {
  // Start the serial port connected to the computer
  USB.begin(9600);
//  while (!USB) {
//    ;
//  }
  //USB.println("Initializing");

  // Start the LCD display
  delay(500); // Wait for LCD to be ready
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
  LCD.print(workingBuffer+9);
  sendToLora("AT+NETWORKID?", workingBuffer, BUFLEN);
  Serial.println(workingBuffer);
  LCD.print(" N=");
  LCD.print(workingBuffer+11);
  sendToLora("AT+PARAMETER=10,7,1,7", workingBuffer, BUFLEN);
  Serial.println(workingBuffer);
  sendToLora("AT+PARAMETER?", workingBuffer, BUFLEN);
  Serial.println(workingBuffer);
  LCD.print(" P=");
  LCD.print(workingBuffer+11);
  
  
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(14, INPUT_PULLUP);
  pinMode(15, INPUT_PULLUP);
  pinMode(16, INPUT_PULLUP);
  pinMode(OVERRIDE_PIN, INPUT_PULLUP);

  setAddress();
  LCD.print(" ID=");
  LCD.print(address);
  
  USB.print("receiver ID: ");
  USB.println(address);
  /*strcpy(workingBuffer, "+RCV=1,5,HELLO,12,34");
  char *s1 = strtok(workingBuffer, ",");
  if (0 == strncmp(workingBuffer, "+RCV=", 5)) {
    s1 = strtok(NULL, ",");
    s1 = strtok(NULL, ",");
    displayLcd(s1);
  }*/
}

bool state = false;
long unsigned int timerCounter = 1000;

void xmitCharacter(char val) {
  String str("AT+SEND=1,1");
  LORA.print(str + val + "\r\n");
}

#define BUFLEN 64

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
    if (0 == strncmp(workingBuffer, "+RCV=", 5)) {
      s1 = strtok(NULL, ",");
      s1 = strtok(NULL, ",");
      displayLcd(s1);

      if(s1[address] == '1'){
        loraSwitch = true;
      }else if (s1[address] == '0'){
        loraSwitch = false;
      }
    }
  }
  
  if(digitalRead(OVERRIDE_PIN)){
    digitalWrite(13, HIGH);
  }else{
    digitalWrite(13, loraSwitch);
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
}
