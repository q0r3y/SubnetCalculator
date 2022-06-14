#include <IRremote.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// TODO: Set max/min input, Setup subnetting, redo boundaries, clean up shit
// https://forum.arduino.cc/t/toggling-debug-code/47041/5

// Data types changed: intervals from longs to uint16_t
// changed: cursorX, cursorY from uint16_t to uint8_t

#define DEBUG 1 // Switch debug output on and off by 1 or 0

#ifdef DEBUG
  #define DEBUG_PRINT(x)      Serial.print (x)
  #define DEBUG_PRINTHEX(x)   Serial.print (x, HEX)
  #define DEBUG_PRINTLN(x)    Serial.println (x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTDEC(x)
  #define DEBUG_PRINTLN(x)
#endif

#define VERSION "VERSON: 0.04.28"
#define RECV_PIN 2  // Infrared Reciever Pin
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define ROWS      4     // Screen has 4 character ROWSs
#define COLUMNS   21    // Screen has 21 character COLUMNSs
#define ASCII     48

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
unsigned long previousCursorMillis = 0;
unsigned long previousIRMillis = 0;
const uint16_t cursorBlinkInterval = 500;
const uint16_t irRecvInterval = 500;
uint8_t cursorX = 0;
uint8_t cursorY = 0;
bool isCharacterVisible = true;


char displayArrayEmpty[ROWS][COLUMNS] = {
  {'I','P','A',':',' ',' ',' ',' ','.',' ',' ',' ','.',' ',' ',' ','.',' ',' ',' ',' '},
  {'S','N','M',':',' ',' ',' ',' ','.',' ',' ',' ','.',' ',' ',' ','.',' ',' ',' ',' '},
  {'S','U','B',':',' ',' ',' ',' ','.',' ',' ',' ','.',' ',' ',' ','.',' ',' ',' ',' '},
  {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
};

char displayArray[ROWS][COLUMNS] = {
  {'I','P','A',':',' ','1','9','2','.','1','6','8','.','0','0','1','.','0','0','1',' '},
  {'S','N','M',':',' ','2','5','5','.','2','5','5','.','2','5','5','.','0','0','0',' '},
  {'S','B','N',':',' ',' ',' ',' ','.',' ',' ',' ','.',' ',' ',' ','.',' ',' ',' ',' '},
  {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
};

void setup() {
  #if DEBUG
    Serial.begin(9600);
  #endif
  enableReceiver();
  initDisplay();
  drawDisplay();
}

void enableReceiver() {
  DEBUG_PRINTLN("[*] Enabling IRin");
  IrReceiver.begin(RECV_PIN, ENABLE_LED_FEEDBACK);
  DEBUG_PRINTLN("[+] Enabled IRin");
}

/*
*  Initilizes display and writes the startup text to screen
*/
void initDisplay() {
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    DEBUG_PRINTLN("[-] SSD1306 allocation failed, Check screen connections");
    for(;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE); // Draw white text
  setCursorLocation(0, 0);
  display.print("SUBNET CALCULATOR");
  setCursorLocation(0, 9);
  display.print(VERSION);
  display.display();
  setCursorLocation(0, 5);
  delay(3000);
}

/*
*  Moves the cursor location and sets global cursor tracking variables
*/
void setCursorLocation(uint8_t x, uint8_t y) {
  display.setCursor(x, y);
  cursorX = x;
  cursorY = y;
}

/*
*  Prints the contents of display array to the screen
*/
void drawDisplay() {
  display.clearDisplay();
  display.setCursor(0, 0);
  for (int16_t x=0; x<4; x++) {
    for (int16_t y=0; y<21; y++) {
      display.write(displayArray[x][y]);
    }
  }
  setCursorLocation(cursorX, cursorY); // Resets cursor to previous location
  display.display();
}

byte threeCharsToByte(char a, char b, char c) {
  return (a-ASCII)*100+(b-ASCII)*10+(c-ASCII)*1;
}

void byteArrToChars(byte (&conjoinedArray)[4], char (&subnet)[12]) {    // Needs to return 3 character array
  byte subnetIndex = 0;
  for (int i=0; i<=3; i++) { // 3 for conjoined array index
      byte number = conjoinedArray[i];
      byte reversedDigitArray[3] = {ASCII,ASCII,ASCII};
      byte rvDigitArrIndex = 2;
      while (number > 0) {
        DEBUG_PRINT("Digit:");
        int digit = number%10; //+ASCII
        reversedDigitArray[rvDigitArrIndex] += digit;
        number /= 10;
        rvDigitArrIndex--;
        DEBUG_PRINTLN(digit);
      }
      for (int j=0; j<3; j++) {
        DEBUG_PRINT("Reverse Digit Array:");
        DEBUG_PRINTLN(reversedDigitArray[j]);
        subnet[subnetIndex] = reversedDigitArray[j];
        subnetIndex++;
      }
  }
  DEBUG_PRINT("Subnet Array: ");
  DEBUG_PRINTLN(subnet);
}

// char * byteToCharacters(char &byteArr[]) {
//     // for characters in byte array add ASCII
// }

// https://stackoverflow.com/a/27740205
// Offset is for inputing in reverse
void byteToBinaryArray(byte (&storageArray)[32], byte num, byte offset) {

  DEBUG_PRINT("RECIEVED:");
  DEBUG_PRINTLN(num);

  int i=0;
  int r;
  byte lastBit = 0;

  while(lastBit!=8) {
    if (num!=0) {
      r = num % 2;
      storageArray[offset+i--] = r;
      num /= 2;
    } else {
      storageArray[offset+i--] = 0;
    }
    lastBit++;
  }
  DEBUG_PRINTLN("");
}

//https://www.tutorialspoint.com/cplusplus/cpp_return_arrays_from_functions.htm
void binaryToByteArr(byte (&array)[32], byte (&conjoinedArray)[4]) {
    int number = 0;
    int binaryMultiplier = 1;
    byte arrIndex = 3;
    // for every 8 add to conjoinedIntArr;
    DEBUG_PRINTLN("");
    for (int i=31; i>=0; i--) {
      number += array[i]*binaryMultiplier;
      binaryMultiplier *= 2;
      if (binaryMultiplier == 256) {
        conjoinedArray[arrIndex--] = number;
        DEBUG_PRINT("Number: ");
        DEBUG_PRINTLN(number);
        number = 0;
        binaryMultiplier = 1;
      }
    }
}

/*
*  Does subnet calcuating
*  Converts characters on screen to integer
*  Converts that integer to binary
*  Does the anding process to calculate subnet
*  Converts the result to Integers
*  Converts the Integers to Characters
*/
void calculateSubnet() {

  byte binaryIpArr[32];
  byte binarySubnetMaskArr[32];
  byte binarySubnetArr[32];
  byte conjoinedSubnetArr[4]; // Will look like ['192', '168', '1', '1']
  char subnet[12];

  DEBUG_PRINTLN("Calculating Subnet");
  DEBUG_PRINTLN("---");

  byte ipOct1 = threeCharsToByte(displayArray[0][5], displayArray[0][6], displayArray[0][7]);
  byte ipOct2 = threeCharsToByte(displayArray[0][9], displayArray[0][10], displayArray[0][11]);
  byte ipOct3 = threeCharsToByte(displayArray[0][13], displayArray[0][14], displayArray[0][15]);
  byte ipOct4 = threeCharsToByte(displayArray[0][17], displayArray[0][18], displayArray[0][19]);

  byte sbOct1 = threeCharsToByte(displayArray[1][5], displayArray[1][6], displayArray[1][7]);
  byte sbOct2 = threeCharsToByte(displayArray[1][9], displayArray[1][10], displayArray[1][11]);
  byte sbOct3 = threeCharsToByte(displayArray[1][13], displayArray[1][14], displayArray[1][15]);
  byte sbOct4 = threeCharsToByte(displayArray[1][17], displayArray[1][18], displayArray[1][19]);

  byteToBinaryArray(binaryIpArr, ipOct1, 7); // Needs to keep track of position or it overwrites the first 8 every time
  byteToBinaryArray(binaryIpArr, ipOct2, 15);
  byteToBinaryArray(binaryIpArr, ipOct3, 23);
  byteToBinaryArray(binaryIpArr, ipOct4, 31);

  #ifdef DEBUG
    DEBUG_PRINTLN("Printing Binary IP Array");
    for(int j=0;j<=31;j++){
      DEBUG_PRINT(binaryIpArr[j]);
    }
    DEBUG_PRINTLN(" ");
  #endif

  byteToBinaryArray(binarySubnetMaskArr, sbOct1, 7);
  byteToBinaryArray(binarySubnetMaskArr, sbOct2, 15);
  byteToBinaryArray(binarySubnetMaskArr, sbOct3, 23);
  byteToBinaryArray(binarySubnetMaskArr, sbOct4, 31);

  #ifdef DEBUG
    DEBUG_PRINTLN("Printing Binary SNM Array");
    for(int j=0;j<=31;j++){
      DEBUG_PRINT(binarySubnetMaskArr[j]);
    }
    DEBUG_PRINTLN(" ");
  #endif

  for (int i=0; i<=31; i++) {  // 31 = array length - 1
   byte ipBit = binaryIpArr[i];
   byte subBit = binarySubnetMaskArr[i];
   if (ipBit==1 && subBit==1) {
     binarySubnetArr[i] = 1;
   } else {
     binarySubnetArr[i] = 0;
   }
  }

  #ifdef DEBUG
    DEBUG_PRINTLN("Printing binary subnet");
    for(int i=0; i<=31; i++) {
      DEBUG_PRINT(binarySubnetArr[i]);
    }
    DEBUG_PRINTLN(" ");
  #endif

  binaryToByteArr(binarySubnetArr, conjoinedSubnetArr);

  #ifdef DEBUG
    DEBUG_PRINT("Conjoined array:");
    DEBUG_PRINT(conjoinedSubnetArr[0]);
    DEBUG_PRINT(conjoinedSubnetArr[1]);
    DEBUG_PRINT(conjoinedSubnetArr[2]);
    DEBUG_PRINTLN(conjoinedSubnetArr[3]);
    DEBUG_PRINTLN(" ");
  #endif

  byteArrToChars(conjoinedSubnetArr, subnet);

  #ifdef DEBUG
    DEBUG_PRINT("Subnet:");
    for (int i=0; i<12; i++) {
      DEBUG_PRINT(subnet[i]);
    }
    DEBUG_PRINTLN(" ");
  #endif

  displayArray[2][5] = subnet[0];
  displayArray[2][6] = subnet[1];
  displayArray[2][7] = subnet[2];
  displayArray[2][9] = subnet[3];
  displayArray[2][10] = subnet[4];
  displayArray[2][11] = subnet[5];
  displayArray[2][13] = subnet[6];
  displayArray[2][14] = subnet[7];
  displayArray[2][15] = subnet[8];
  displayArray[2][17] = subnet[9];
  displayArray[2][18] = subnet[10];
  displayArray[2][19] = subnet[11];

  // Needs to place calculated data in display array
}

/*
*  Used for automatically advancing the cursor within established boundaries
*/
void advanceCursor() {
  char currentChar;

  if ((cursorX == 0 || cursorX == 1) && (cursorY >= 5 || cursorY <= 20)) { // X & Y boundaries
    cursorY += 1;
    currentChar = displayArray[cursorX][cursorY];
    if (currentChar == 0x2E) {  // Checks if cursor is on a "."
      cursorY += 1;
    } else if (cursorY == 20 && cursorX != 1) {  // Moves cursor down a line
      cursorX += 1;
      cursorY = 5;
      setCursorLocation(cursorX, cursorY);
    } else if (cursorY == 20 && cursorX == 1) {  // Moves cursor to top line
      cursorX = 0;
      cursorY = 5;
      setCursorLocation(cursorX, cursorY);
    }
  }
}

/*
*  All the code it takes just to blink the damn cursor
*/
void blinkCursor() {
  int xPixelOffset = 6; // All characters are 5px on x-axis
  int yPixelOffset = 8; // All characters are 7px on y-axis
  char currentChar = displayArray[cursorX][cursorY];
  unsigned long currentMillis = millis();

  if (currentMillis - previousCursorMillis >= cursorBlinkInterval) {
    previousCursorMillis = currentMillis;
    if (isCharacterVisible) {
      display.drawChar(cursorY*xPixelOffset, cursorX*yPixelOffset, 0x5F, WHITE, BLACK, 1); // X&Y are backwards. 0x5F = _
      isCharacterVisible = false;
      display.display();
    } else {
      display.drawChar(cursorY*xPixelOffset, cursorX*yPixelOffset, currentChar, WHITE, BLACK, 1); // X&Y are backwards.
      isCharacterVisible = true;
      display.display();
    }
  }
}

/*
*  Converts infrared codes to characters and commands
*/
void handleRecievedData(uint32_t &data) {
  // Mappings: https://github.com/adafruit/Adafruit-GFX-Library/blob/master/Fonts/FreeMono12pt7b.h
  switch (data) {
    case 0xEE11FB04:  //1
      DEBUG_PRINTLN(": 1");
      displayArray[cursorX][cursorY] = 0x31;
      calculateSubnet();
      advanceCursor();
      drawDisplay();
      break;
    case 0xED12FB04:  //2
      DEBUG_PRINTLN(": 2");
      displayArray[cursorX][cursorY] = 0x32;
      calculateSubnet();
      advanceCursor();
      drawDisplay();
      break;
    case 0xEC13FB04:  //3
      DEBUG_PRINTLN(": 3");
      displayArray[cursorX][cursorY] = 0x33;
      calculateSubnet();
      advanceCursor();
      drawDisplay();
      break;
    case 0xEB14FB04:  //4
      DEBUG_PRINTLN(": 4");
      displayArray[cursorX][cursorY] = 0x34;
      calculateSubnet();
      advanceCursor();
      drawDisplay();
      break;
    case 0xEA15FB04:  //5
      DEBUG_PRINTLN(": 5");
      displayArray[cursorX][cursorY] = 0x35;
      calculateSubnet();
      advanceCursor();
      drawDisplay();
      break;
    case 0xE916FB04:  //6
      DEBUG_PRINTLN(": 6");
      displayArray[cursorX][cursorY] = 0x36;
      calculateSubnet();
      advanceCursor();
      drawDisplay();
      break;
    case 0xE817FB04:  //7
      DEBUG_PRINTLN(": 7");
      displayArray[cursorX][cursorY] = 0x37;
      calculateSubnet();
      advanceCursor();
      drawDisplay();
      break;
    case 0xE718FB04:  //8
      DEBUG_PRINTLN(": 8");
      displayArray[cursorX][cursorY] = 0x38;
      calculateSubnet();
      advanceCursor();
      drawDisplay();
      break;
    case 0xE619FB04:  //9
      DEBUG_PRINTLN(": 9");
      displayArray[cursorX][cursorY] = 0x39;
      calculateSubnet();
      advanceCursor();
      drawDisplay();
      break;
    case 0xEF10FB04:  //0
      DEBUG_PRINTLN(": 0");
      displayArray[cursorX][cursorY] = 0x30;
      calculateSubnet();
      advanceCursor();
      drawDisplay();
      break;
    // case 0xFFFB04:  //.
    //   DEBUG_PRINT(": .");
    //   displayArray[cursorX][cursorY] = 0x2E;
    //   calculateSubnet();
    //   advanceCursor();
    //   drawDisplay();
    //   break;
    case 0xB847FB04:  // Left
      if (cursorY != 5) {
        cursorY -= 1;
        drawDisplay();
      }
      break;
    case 0xB748FB04:  // Right
      if (cursorY != 19) {
        cursorY += 1;
        drawDisplay();
      }
      break;
    case 0xB946FB04:  // Down
      if (cursorX != 1) {
        cursorX += 1;
        drawDisplay();
      }
      break;
    case 0xBA45FB04:  // Up
      if (cursorX != 0) {
        cursorX -= 1;
        drawDisplay();
      }
      break;
    case 0xF708FB04:  // Clears everything
      for (int16_t x=0; x<4; x++) {
        for (int16_t y=0; y<21; y++) {
          displayArray[x][y] = displayArrayEmpty[x][y];
        }
      }
      cursorX = 0;
      cursorY = 5;
      calculateSubnet();
      drawDisplay();
      break;
  }
}

void loop() {
  unsigned long currentMillis = millis();

  blinkCursor();

  if (currentMillis - previousIRMillis >= irRecvInterval) {
    previousIRMillis = currentMillis;
    if (IrReceiver.decode()) {
      uint32_t recievedData = IrReceiver.decodedIRData.decodedRawData;
      DEBUG_PRINTHEX(IrReceiver.decodedIRData.decodedRawData);
      handleRecievedData(recievedData);
      DEBUG_PRINTLN(" ");
      IrReceiver.resume();
    }
  }
}
