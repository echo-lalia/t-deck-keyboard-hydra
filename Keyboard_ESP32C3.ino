/**
 * @file      Keyboard_ESP32C3.ino
 * @author    Koby Hale
 * @license   MIT
 * @copyright Copyright (c) 2023  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2023-04-11
 *
 * modified by hasn0life 2023-10-23
 * modified by echo-lalia 2024-5-24
 */

#define I2C_DEV_ADDR 0x55
#define keyboard_BL_PIN  9
#define keyboard_INT_PIN 8
#define KB_SDA  2
#define KB_SCL  10
#include "Wire.h"

uint8_t  rows[] = {0, 3, 19, 12, 18, 6, 7 };
const int rowCount = sizeof(rows) / sizeof(rows[0]);

uint8_t  cols[] = {1, 4, 5, 11, 13};
const int colCount = sizeof(cols) / sizeof(cols[0]);

bool keys[colCount][rowCount];
bool lastValue[colCount][rowCount];
bool changedValue[colCount][rowCount];
char keyboard[colCount][rowCount];
char keyboard_symbol[colCount][rowCount];

bool symbolSelected;
bool BL_state = false;
bool comdata_flag = false;
char comdata;
bool capslock = false;
bool symlock = false;
bool numlock = false;

enum longpress_state {NO_KEY, WAITING, PRESSED, PRINTED};
enum longpress_state long_pressed = NO_KEY;
uint32_t last_time = 0;
const uint32_t LONGPRESS_DURATION = 400;

// value stores setting for 'raw' output or default output
bool rawOutput = false;
const int rawKeysCount = 4;
uint8_t rawKeys[rawKeysCount];

void onRequest();
void readMatrix();
bool keyPressed(int colIndex, int rowIndex);
bool keyActive(int colIndex, int rowIndex);
bool isPrintableKey(int colIndex, int rowIndex);
void printMatrix();
void set_keyboard_BL(bool state);

// process config changes on receive
void onReceive(int receivedBytes);
void clearRawOut(int clearStart);

void setup()
{
    // put your setup code here, to run once:
    keyboard[0][0] = 'q';
    keyboard[0][1] = 'w';
    keyboard[0][2] = 0; // symbol
    keyboard[0][3] = 'a';
    keyboard[0][4] = 0; // ALT
    keyboard[0][5] = ' ';
    keyboard[0][6] = '~'; // Mic

    keyboard[1][0] = 'e';
    keyboard[1][1] = 's';
    keyboard[1][2] = 'd';
    keyboard[1][3] = 'p';
    keyboard[1][4] = 'x';
    keyboard[1][5] = 'z';
    keyboard[1][6] = 0; // Left Shift

    keyboard[2][0] = 'r';
    keyboard[2][1] = 'g';
    keyboard[2][2] = 't';
    keyboard[2][3] = 0; // Right Shit
    keyboard[2][4] = 'v';
    keyboard[2][5] = 'c';
    keyboard[2][6] = 'f';

    keyboard[3][0] = 'u';
    keyboard[3][1] = 'h';
    keyboard[3][2] = 'y';
    keyboard[3][3] = 0; // Enter
    keyboard[3][4] = 'b';
    keyboard[3][5] = 'n';
    keyboard[3][6] = 'j';

    keyboard[4][0] = 'o';
    keyboard[4][1] = 'l';
    keyboard[4][2] = 'i';
    keyboard[4][3] = 0; // Backspace
    keyboard[4][4] = '$';  // speaker
    keyboard[4][5] = 'm';
    keyboard[4][6] = 'k';

    keyboard_symbol[0][0] = '#';
    keyboard_symbol[0][1] = '1';
    keyboard_symbol[0][2] = 0; // symbol
    keyboard_symbol[0][3] = '*';
    keyboard_symbol[0][4] = 0; // ALT
    keyboard_symbol[0][5] = ' ';
    keyboard_symbol[0][6] = '0';  // Mic

    keyboard_symbol[1][0] = '2';
    keyboard_symbol[1][1] = '4';
    keyboard_symbol[1][2] = '5';
    keyboard_symbol[1][3] = '@';
    keyboard_symbol[1][4] = '8';
    keyboard_symbol[1][5] = '7';
    keyboard_symbol[1][6] = 0; //lshift

    keyboard_symbol[2][0] = '3';
    keyboard_symbol[2][1] = '/';
    keyboard_symbol[2][2] = '(';
    keyboard_symbol[2][3] = 0; //rshift
    keyboard_symbol[2][4] = '?';
    keyboard_symbol[2][5] = '9';
    keyboard_symbol[2][6] = '6';

    keyboard_symbol[3][0] = '_';
    keyboard_symbol[3][1] = ':';
    keyboard_symbol[3][2] = ')';
    keyboard_symbol[3][3] = 0;  //enter
    keyboard_symbol[3][4] = '!';
    keyboard_symbol[3][5] = ',';
    keyboard_symbol[3][6] = ';';

    keyboard_symbol[4][0] = '+';
    keyboard_symbol[4][1] = '"';
    keyboard_symbol[4][2] = '-';
    keyboard_symbol[4][3] = 0; //backspace
    keyboard_symbol[4][4] = '\\'; // speaker now makes forward slash
    keyboard_symbol[4][5] = '.';
    keyboard_symbol[4][6] = '\'';

    Serial.begin(115200);

    Serial.setDebugOutput(true);
    Wire.onRequest(onRequest);

    // init config handler
    Wire.onReceive(onReceive);

    Wire.begin((uint8_t)I2C_DEV_ADDR, KB_SDA, KB_SCL, 100000); //needs frequency or it'll think it's in slave mode
    // Wire.begin((uint8_t)I2C_DEV_ADDR, SDA, SCL);

    Serial.println("Starting keyboard!");
    pinMode(keyboard_BL_PIN, OUTPUT);
    digitalWrite(keyboard_BL_PIN, BL_state);

    // add intrupt:
    pinMode(keyboard_INT_PIN, OUTPUT);
    digitalWrite(keyboard_INT_PIN, LOW);


    Serial.println("4");
    for (int x = 0; x < rowCount; x++) {
        Serial.print(rows[x]); Serial.println(" as input");
        pinMode(rows[x], INPUT);
    }

    for (int x = 0; x < colCount; x++) {
        Serial.print(cols[x]); Serial.println(" as input-pullup");
        pinMode(cols[x], INPUT_PULLUP);
    }

    symbolSelected = false;

}

void loop()
{
    readMatrix();
    printMatrix();


    if (!keyActive(1, 6) && keyPressed(0, 2)) {
        Serial.println("symbol selected");
        symbolSelected = true;
    }

    // key 3,3 is the enter key
    if (keyReleased(3, 3)) {
        Serial.println();
        comdata = (char)0x0D;
        comdata_flag = true;
    }
    if (keyReleased(4, 3) || (keyActive(4, 3) && (long_pressed == PRESSED))) { 
        Serial.println("backspace");
        comdata = (char)0x08;
        comdata_flag = true;
    }

    //state transition backspace long press at key release
    if (keyReleased(4, 3) && (long_pressed == PRESSED)){
      long_pressed = NO_KEY;
    }

    if (keyActive(0, 4) && keyPressed(3, 4) && (rawOutput == false)) { //Alt+B
        Serial.println("Alt+B");
        BL_state = !BL_state;
        set_keyboard_BL(BL_state);
    }

    if (keyActive(0, 4) && keyPressed(2, 5)) { //Alt+C
        Serial.println("Alt+C");
        comdata = (char)0x0C;
        comdata_flag = true;
    }

    //custom stuff
    if (keyActive(0, 4) && keyPressed(2, 3)) { //alt and right shift = capslock
        Serial.println("capslock");
        if(capslock == true){
          capslock = false;
        }else{
          capslock = true;
        }
    }
    // if (keyActive(1, 6) && keyPressed(2, 3)) { //left and right shift = numlock
    //     Serial.println("capslock");
    //     if(capslock == true){
    //       capslock = false;
    //     }else{
    //       capslock = true;
    //     }
    // }
    if (keyActive(1, 6) && keyPressed(0, 2)) { //left shift and sym = symlock
        Serial.println("symlock");
        if(symlock == true){
          symlock = false;
        }else{
          symlock = true;
        }
    }

    // pulse interupt line for main controller
    if(comdata_flag == true){
        digitalWrite(keyboard_INT_PIN, HIGH);
    } else {
        digitalWrite(keyboard_INT_PIN, LOW);
    }
}

void onRequest()
{
    if (rawOutput){
        // print raw array
        Wire.write(rawKeys, rawKeysCount);
        comdata_flag = false;

    } else {

        if (comdata_flag) {
            Wire.print(comdata);
            comdata_flag = false;
            Serial.print(" comdata :");
            Serial.println(comdata);
        } else {
            Wire.print((char)0x00);
        }
    }

}


// allow host to send options data to keyboard
void onReceive(int receivedBytes)
{
    int message = Wire.read();
    // set backlight (vals 0 and 1)
    if (message == 0){
      BL_state = false;
      digitalWrite(keyboard_BL_PIN, false);
    }
    if (message == 1){
      BL_state = true;
      digitalWrite(keyboard_BL_PIN, true);
    }

    // set raw output (vals 2 and 3)
    if (message == 2){
      rawOutput = false;
    }
    if (message == 3){
      rawOutput = true;
    }
}


// reset raw output to 0s (starting at clearstart)
void clearRawOut(int clearStart)
{
    for (int rawIndex = clearStart; rawIndex < rawKeysCount; rawIndex++) {
      rawKeys[rawIndex] = 0;
    }
}


void readMatrix()
{
    int rawIndex = 0;
    int delayTime = 0;
    // iterate the columns
    for (int colIndex = 0; colIndex < colCount; colIndex++) {
        // col: set to output to low
        uint8_t  curCol = cols[colIndex];
        pinMode(curCol, OUTPUT);
        digitalWrite(curCol, LOW);

        // row: interate through the rows
        for (int rowIndex = 0; rowIndex < rowCount; rowIndex++) {
            uint8_t  rowCol = rows[rowIndex];
            pinMode(rowCol, INPUT_PULLUP);
            delay(1); // arduino is not fast enought to switch input/output modes so wait 1 ms

            bool buttonPressed = (digitalRead(rowCol) == LOW);

            // store raw reads
            if (buttonPressed && (rawIndex < rawKeysCount)){
              // pack row&col into integer for raw output
              rawKeys[rawIndex] = (colIndex << 4) | (rowIndex + 1);
              rawIndex++;
            }
            
            keys[colIndex][rowIndex] = buttonPressed;

            if ((lastValue[colIndex][rowIndex] != buttonPressed)) {
                changedValue[colIndex][rowIndex] = true;
            } else {
                changedValue[colIndex][rowIndex] = false;
            }

            if(buttonPressed && changedValue[colIndex][rowIndex]){
              if(isPrintableKey(colIndex, rowIndex) || keyActive(4, 3)){  //backspace is longpressable
                last_time = millis();
                long_pressed = WAITING;
              }
            }

            if(buttonPressed && (long_pressed == WAITING)){
              if(isPrintableKey(colIndex, rowIndex) || keyActive(4, 3)){ //backspace is longpressable
                if((last_time + LONGPRESS_DURATION) < millis()){
                  long_pressed = PRESSED;
                  Serial.println("long pressed!!!");
                }
              }
            }

            lastValue[colIndex][rowIndex] = buttonPressed;
            pinMode(rowCol, INPUT);
        }
        // disable the column
        pinMode(curCol, INPUT);
    }

    // clear the remaining rawOutput slots
    clearRawOut(rawIndex);
}

bool keyPressed(int colIndex, int rowIndex)
{
    return changedValue[colIndex][rowIndex] && keys[colIndex][rowIndex] == true;
}

bool keyActive(int colIndex, int rowIndex)
{
    return keys[colIndex][rowIndex] == true;
}

bool keyReleased(int colIndex, int rowIndex)
{
    return changedValue[colIndex][rowIndex] && keys[colIndex][rowIndex] == false;
}

bool isPrintableKey(int colIndex, int rowIndex)
{
    return keyboard_symbol[colIndex][rowIndex] != 0 || keyboard[colIndex][rowIndex] != 0;
}

// Keyboard backlit status
void set_keyboard_BL(bool state)
{
    digitalWrite(keyboard_BL_PIN, state);
}

void printMatrix()
{
    for (int rowIndex = 0; rowIndex < rowCount; rowIndex++) {
        for (int colIndex = 0; colIndex < colCount; colIndex++) {
            // we only want to print if the key is pressed and it is a printable character
              if ((keyReleased(colIndex, rowIndex) && isPrintableKey(colIndex, rowIndex) ) || (keyActive(colIndex, rowIndex) && (long_pressed == PRESSED))) { //wait for key to be released so we can possibly long press it
                char toPrint;
                if (symbolSelected || symlock || keyActive(0, 2) 
                || ((long_pressed == PRESSED) && !keyActive(4, 3))) {  //dont move through backspace
                    symbolSelected = false;
                    long_pressed = PRINTED;
                    toPrint = char(keyboard_symbol[colIndex][rowIndex]);
                } else {
                  if(long_pressed == PRINTED){
                    long_pressed = NO_KEY;
                  }else{
                    toPrint = char(keyboard[colIndex][rowIndex]);
                  }
                }

                // keys 1,6 and 2,3 are Shift keys, so we want to upper case
                if (keyActive(1, 6) || keyActive(2, 3) || (capslock == true)) {
                    toPrint = (char)((int)toPrint - 32);
                }

                Serial.print(toPrint);

                comdata = toPrint;
                comdata_flag = true;

            }
        }
    }
}
