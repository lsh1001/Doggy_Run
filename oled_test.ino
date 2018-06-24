#include <Wire.h>
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include "bitmaps.h"
#include "InputController.h"

#define OLED_RESET 8

#define FULL_LOGO 0
#define ARDU_LOGO 1
#define X_LOGO    2
#define PNL_LOGO  3

#define STATUS_MENU 0
#define STATUS_CREDIT 1

int gameState = STATUS_MENU;    // Pause or not
boolean gameEnd = false;
unsigned long startTime = 0;

#define BUTTON_A 5
#define BUTTON_B 6
InputController inputController;
bool up, down, left, right, aBut, bBut;

// Menu
#define MENU_START 1
#define MENU_OPTION 2
#define MENU_CREDIT 3
#define MENU_MIN 1
#define MENU_MAX 3
int prevMenu = MENU_MAX;
int currentMenu = 0;

// Time variables
unsigned long lTime;
unsigned long iTime;
int gameFPS = 1000/10; 
int inputTIME = 1000/4;

PROGMEM char* const stringTable[] = {
  "Start game",
  "Options",
  "Credit"
};

Adafruit_SSD1306 display(OLED_RESET);

void stopUntilUserInput();
void setMenuMode();
void initUserInput();

void setup(){
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  display.setTextSize(1);
  display.setTextColor(WHITE);
  
  // set button pins as INPUT_PULLUP
  pinMode(BUTTON_A,INPUT_PULLUP);
  pinMode(BUTTON_B,INPUT_PULLUP);

  for(int i=0; i<37; i=i+2) {
    display.clearDisplay();
    display.drawBitmap(i,32, logo[ARDU_LOGO], 40,8,1);
    display.drawBitmap(71,i-3, logo[X_LOGO], 8,8,1);
    display.drawBitmap(112-i,32, logo[PNL_LOGO],16,8,1);
    display.display();
  }

  display.drawBitmap(32,28, logo[FULL_LOGO], 64,16,1);
  display.display();
  
  display.clearDisplay();
  display.display();
  
  display.drawBitmap(32,28, logo[FULL_LOGO], 64,16,1);
  display.display();
  delay(1500);

  //display main image
  display.clearDisplay(); 
  display.drawBitmap(5,28,lobby,32,32,1);
  display.drawBitmap(35,-3,title,64,64,1);
  display.drawBitmap(100,22,skeleton,32,32,1);
  display.drawBitmap(108,8,bone,16,16,1);
  display.drawBitmap(0,0,background1,64,64,1);
  display.drawBitmap(64,0,background2,64,64,1);
  display.display();

  stopUntilUserInput();    // Wait until user touch the button

  setMenuMode();  // Menu mode
}
void loop(){
  if(millis()> iTime + inputTIME){
    iTime = millis();
    
    uint8_t input = inputController.getInput();  // Get input status
    if (input & (1<<5)) left = true;
    if (input & (1<<4)) up = true;
    if (input & (1<<3)) right = true;
    if (input & (1<<2)) down = true;
    if (input & (1<<1)) aBut = true;  // a button
    if (input & (1<<0)) bBut = true;  // b button
  }

  if (millis() > lTime + gameFPS) {
    lTime = millis();

    if (gameState == STATUS_MENU) {  // Main menu
      if(up) currentMenu--;
      else if(down) currentMenu++;
      if(currentMenu > MENU_MAX) currentMenu = MENU_MAX;
      if(currentMenu < MENU_MIN) currentMenu = MENU_MIN;
      
      // User selected one of menu
      if(aBut || bBut) {
          if(currentMenu == MENU_CREDIT) {
            setCreditMode();
        }
        else if(currentMenu == MENU_START) {
//          setGameMode();
        }
        else if(currentMenu == MENU_OPTION) {
//          setOptionMode();
        }
      } 
      // Menu navigation
      else {
        if(prevMenu != currentMenu) {
          prevMenu = currentMenu;   // Remember current menu index
          
          // Draw menu
          display.clearDisplay();
          for(int i=0; i<MENU_MAX; i++) {
            display.setCursor(24, 10+i*12);
            display.print((const char*)pgm_read_word(&(stringTable[i])));
          }
          display.setCursor(10, 10 + (currentMenu - 1)*12);
          display.print("> ");
          display.display();
        }
      }
    }
    else if (gameState == STATUS_CREDIT) {  // If the game is paused
      display.clearDisplay();
      display.drawBitmap(5,28,lobby,32,32,1);
      display.drawBitmap(35,-3,title,64,64,1);
      display.drawBitmap(100,22,skeleton,32,32,1);
      display.drawBitmap(108,8,bone,16,16,1);
      display.drawBitmap(0,0,background2,64,64,1);
      display.drawBitmap(64,0,background2,64,64,1);
      
      display.setCursor(0,0);
      display.print("made by P&L");
      
      display.display();
      stopUntilUserInput();    // Wait until user touch the button
      setMenuMode();
    }
    // Initialize user input
    initUserInput();
  }
}

void stopUntilUserInput() {
  while (true) {  // While we wait for input
    if (digitalRead(BUTTON_A) == LOW) {  // Wait for a button press
      break;
    }
    delay(200);  // Slight delay the loop
  }
}

void setMenuMode() {
  gameState = STATUS_MENU;
  prevMenu = MENU_MAX;
  currentMenu = 0;
  delay(200);
}

void setCreditMode() {
  gameState = STATUS_CREDIT;
  delay(200);
}

void initUserInput() {
  left = false;
  up = false;
  right = false;
  down = false;
  aBut = false; // a button
  bBut = false; // b button
}
