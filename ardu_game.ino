#define SSD1306_I2C

#include "bitmaps.h"
#include <SPI.h> 
#include <Wire.h> 
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include "InputController.h"

#ifndef SSD1306_I2C
  #define CLK  13
  #define MOSI  11
  #define CS  6
  #define DC  4
  #define RST 12
  Adafruit_SSD1306 display(MOSI, CLK, DC, RST, CS);
#else
  #define OLED_RESET 8
  Adafruit_SSD1306 display(OLED_RESET);
#endif

#define FULL_LOGO 0
#define ARDU_LOGO 1
#define X_LOGO    2
#define PNL_LOGO  3

// Display variables
int HEIGHT = 64;
int WIDTH = 128;
int gameFPS = 1000/10;

// Time variables
unsigned long lTime;

// Game status
#define STATUS_MENU 0
#define STATUS_CREDIT 1
#define STATUS_PLAYING 2

int gameState = STATUS_MENU; // Pause or not
int gameScore = 0;
int gameHighScore = 0;

// Input
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

PROGMEM char* const stringTable[] = {
  "Start game",
  "Options",
  "Credit"
};

boolean drawBg = true;

// Object parameters
#define OBSTACLE_MAX 3
#define OBSTACLE_MOVE 3
#define OBSTACLE_WIDTH 3
#define OBSTACLE_HEIGHT 4
#define OBS_POS_Y 59
#define OBSTACLE_DEL_THRESHOLD 5
int obstacleX[] = {0, 0, 0};
int obstacleCount = 0;
unsigned long obstacleTime;

#define ENEMY_MAX 1
#define ENEMY_DELAY 4000
#define ENEMY_MOVE 2
#define ENEMY_WIDTH 32
#define ENEMY_HEIGHT 32
#define ENEMY_POS_Y 30
#define ENEMY_START_X 128
#define ENEMY_RUN_IMAGE_INDEX 0
#define ENEMY_DIE_IMAGE_INDEX 1
int enemyX[] = {0};
int prevEnemyPosX[] = {0};
int enemyCount = 0;
unsigned long enemyTime;

#define BULLET_MAX 2
#define BULLET_MOVE 3
#define BULLET_WIDTH 3
#define BULLET_POS_Y 48
#define BULLET_START_X 44
int bulletX[] = {0, 0};
int prevBulletPosX[] = {0, 0};
int bulletCount = 0;
unsigned long bulletTime;

#define BONE_MAX 3
#define BONE_MOVE 4
#define BONE_WIDTH 16
#define BONE_HEIGHT 16
#define BONE_POS_Y 10
#define BONE_START_X 128
#define BONE_DEL_THRESHOLD 10
int boneX[] = {0, 0, 0};
int prevBonePosX[] = {0, 0, 0};
int boneCount = 0;
unsigned long boneTime;

void draw();
void checkInput();
void updateMove();

// Utilities
int getOffset(int s);
void initUserInput();
void stopUntilUserInput();
void setMenuMode();
void setGameMode();
void setResultMode();
void setCreditMode();

unsigned long getRandTime();

void setup() {
  // initialize display
#ifndef SSD1306_12C
  SPI.begin();
  display.begin(SSD1306_SWITCHCAPVCC);
#else
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // initialize with the I2C addr 0x3D (for the 128x64)
#endif
  display.setTextSize(1);
  display.setTextColor(WHITE);

  //set buttons pins as INPUT_PULLUP
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);

  // display startup animation
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

void loop() {
  uint8_t input = inputController.getInput();  // Get input status
  if (input & (1<<5)) left = true;
  if (input & (1<<4)) up = true;
  if (input & (1<<3)) right = true;
  if (input & (1<<2)) down = true;
  if (input & (1<<1)) aBut = true;  // a button
  if (input & (1<<0)) bBut = true;  // b button

  if (millis() > lTime + gameFPS) {
    lTime = millis();

    if (gameState == STATUS_MENU) {  // Main menu
      if(up) currentMenu--;
      else if(down) currentMenu++;
      if(currentMenu > MENU_MAX) currentMenu = MENU_MAX;
      if(currentMenu < MENU_MIN) currentMenu = MENU_MIN;
      
      if(aBut || bBut) {
            if(currentMenu == MENU_CREDIT) {
              setCreditMode();
          }
          else if(currentMenu == MENU_START) {
              setGameMode();
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
    else if (gameState == STATUS_PLAYING) { // If the game is playing
      // Run game engine
      checkInput();
      updateMove();
//    checkCollision();
      
      // Draw game screen
      draw();
      
      // Exit condition
//    if(charStatus == CHAR_DIE) {
//      setResultMode();
//    }
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

void checkInput() {
  if(aBut) {
//  if(charStatus == CHAR_RUN) {
      if(bulletCount < BULLET_MAX) {
        for(int i=0; i<BULLET_MAX; i++) {
          if(bulletX[i] < 1) {
            bulletX[bulletCount] = BULLET_START_X - BULLET_MOVE;
//          charStatus = CHAR_FIRE;
            bulletCount++;
          }
        }
//    }
    }
  }
}

void updateMove() {
  
  // Make obstacle
  if(obstacleCount < OBSTACLE_MAX && millis() > obstacleTime) {
    // Make obstacle
    for(int i=0; i<OBSTACLE_MAX; i++) {
      if(obstacleX[i] < 1) {
        obstacleX[i] = 127;
        obstacleCount++;
        obstacleTime = getRandTime(); // Reserve next obstacle
        break;
      }
    }
  }

  // Obstacle move
  if(obstacleCount > 0) {
    for(int i=0; i<OBSTACLE_MAX; i++) {
      if(obstacleX[i] > 0) {
        obstacleX[i] -= OBSTACLE_MOVE;
        if(obstacleX[i] < OBSTACLE_DEL_THRESHOLD) {
          // clear last drawing
          display.fillRect(obstacleX[i] + OBSTACLE_MOVE, OBS_POS_Y, OBSTACLE_WIDTH, OBSTACLE_HEIGHT, BLACK);
          
          // delete obstacle
          obstacleX[i] = 0;
          obstacleCount--;
//          gameScore += OBSTACLE_SCORE;
        }
      }
    }
  }

  // Make enemy
  if(enemyCount < ENEMY_MAX && millis() > enemyTime) {
    // Make enemy
    for(int i=0; i<ENEMY_MAX; i++) {
      if(enemyX[i] < 1) {
        enemyX[i] = 127;
        enemyCount++;
        enemyTime = ENEMY_DELAY + getRandTime();  // Reserve next enemy
        break;
      }
    }
  }
  
  // Enemy move
  if(enemyCount > 0) {
    for(int i=0; i<ENEMY_MAX; i++) {
      if(enemyX[i] > 0) {
        enemyX[i] -= ENEMY_MOVE;
        if(enemyX[i] < OBSTACLE_DEL_THRESHOLD) {
          // clear last drawing
          display.fillRect(enemyX[i] + ENEMY_MOVE, ENEMY_POS_Y, ENEMY_WIDTH, ENEMY_HEIGHT-4, BLACK);
          
          // delete enemy
          enemyX[i] = 0;
          enemyCount--;
        }
      }
    }
  }

  // Make bone
  if(boneCount < BONE_MAX && millis() > boneTime) {
    // Make obstacle
    for(int i=0; i<BONE_MAX; i++) {
      if(boneX[i] < 1) {
        boneX[i] = 127;
        boneCount++;
        boneTime = getRandTime(); // Reserve next obstacle
        break;
      }
    }
  }

  // Bone move
  if(boneCount > 0) {
    for(int i=0; i<BONE_MAX; i++) {
      if(boneX[i] > 0) {
        boneX[i] -= BONE_MOVE;
        if(boneX[i] < BONE_DEL_THRESHOLD) {
          // clear last drawing
          display.fillRect(boneX[i] + BONE_MOVE, BONE_POS_Y, BONE_WIDTH, BONE_HEIGHT, BLACK);
          
          // delete obstacle
          boneX[i] = 0;
          boneCount--;
//          gameScore += OBSTACLE_SCORE;
        }
      }
    }
  }

}

void draw() {
  // draw background
  if(drawBg) {
    display.clearDisplay();
    display.drawBitmap(0, 0, background, 128, 64, 1);
    drawBg = false;
  }

  // draw obstacle
  if(obstacleCount > 0) {
    for(int i=0; i<OBSTACLE_MAX; i++) {
      if(obstacleX[i] > 0) {
        display.fillRect(obstacleX[i] + OBSTACLE_MOVE, OBS_POS_Y, OBSTACLE_WIDTH, OBSTACLE_HEIGHT, BLACK);  // clear previous drawing
        display.fillRect(obstacleX[i], OBS_POS_Y, OBSTACLE_WIDTH, OBSTACLE_HEIGHT, WHITE);
      }
    }
  }
  
  if(enemyCount > 0) {
    for(int i=0; i<ENEMY_MAX; i++) {
      if(enemyX[i] == -1) {
        // Enemy dead image
        display.fillRect(prevEnemyPosX[i], ENEMY_POS_Y, ENEMY_WIDTH, ENEMY_HEIGHT-4, BLACK);  // clear previous drawing
        display.drawBitmap(prevEnemyPosX[i], ENEMY_POS_Y, (const unsigned char*)pgm_read_word(&(enemy_anim[ENEMY_DIE_IMAGE_INDEX])), ENEMY_WIDTH, ENEMY_HEIGHT, WHITE);
        enemyX[i] = -2;
      }
      else if(enemyX[i] == -2) {
        // Clear enemy drawing
        display.fillRect(prevEnemyPosX[i], ENEMY_POS_Y, ENEMY_WIDTH, ENEMY_HEIGHT, BLACK);  // clear previous drawing
        enemyX[i] = 0;
        enemyCount--;
      }
      else if(enemyX[i] > 0) {
        // Enemy running image
        display.fillRect(prevEnemyPosX[i], ENEMY_POS_Y, ENEMY_WIDTH, ENEMY_HEIGHT-4, BLACK);  // clear previous drawing
        display.drawBitmap(enemyX[i], ENEMY_POS_Y, (const unsigned char*)pgm_read_word(&(enemy_anim[ENEMY_RUN_IMAGE_INDEX])), ENEMY_WIDTH, ENEMY_HEIGHT, WHITE);
        prevEnemyPosX[i] = enemyX[i];
      }
    }
  }

  // draw bone
  if(boneCount > 0) {
    for(int i=0; i<BONE_MAX; i++) {
      if(boneX[i] > 0) {
        display.fillRect(prevBonePosX[i], BONE_POS_Y, BONE_WIDTH, BONE_HEIGHT, BLACK);  // clear previous drawing
        display.drawBitmap(boneX[i], BONE_POS_Y, bone, BONE_WIDTH, BONE_HEIGHT, WHITE);
        prevBonePosX[i] = boneX[i];
      }
    }
  }
  
  
  display.display();
}

//=========================================================================================================

void initUserInput() {
  left = false;
  up = false;
  right = false;
  down = false;
  aBut = false; // a button
  bBut = false; // b button
}

void stopUntilUserInput() {
  while (true) {  // While we wait for input
    if (digitalRead(BUTTON_A) == LOW || digitalRead(BUTTON_B) == LOW) {  // Wait for a button press
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

void setGameMode() {
  gameState = STATUS_PLAYING;
  drawBg = true;
  delay(200);
}

unsigned long getRandTime() {
  return millis() + 100 * random(12, 30);
}

