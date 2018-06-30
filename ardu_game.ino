#define SSD1306_I2C

#include <SPI.h>
#include "bitmaps.h"
#include <Wire.h> 
#include <avr/interrupt.h>
#include <avr/eeprom.h>
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

// System
int eepromAddr = 0;

#define FULL_LOGO 0
#define ARDU_LOGO 1
#define X_LOGO    2
#define PNL_LOGO  3

// Display variables
int HEIGHT = 64;
int WIDTH = 128;
int gameFPS = 1000/20;

// Time variables
unsigned long lTime;

// Game status
#define STATUS_MENU 0
#define STATUS_PLAYING 1
#define STATUS_PAUSED 2
#define STATUS_RESULT 3
#define STATUS_CREDIT 4

int gameState = STATUS_MENU;    // Pause or not
boolean gameEnd = false;
unsigned long startTime = 0;
int gameScore = 0;
int prevGameScore = 0;
int gameHighScore = 0;
#define OBSTACLE_SCORE 1
#define ENEMY_KILL_SCORE 2
#define ITEM_SCORE 2

// Input
#define BUTTON_A 5
#define BUTTON_B 6
InputController inputController;
bool up, down, left, right, aBut, bBut;

// Menu
#define MENU_START 1
#define MENU_CREDIT 2
#define MENU_MIN 1
#define MENU_MAX 2
int prevMenu = MENU_MAX;
int currentMenu = 0;

PROGMEM char* const stringTable[] = {
  "Start game",
  "Credit"
};

// Character parameters
#define CHAR_POS_X 15
#define CHAR_POS_Y 30.5
#define CHAR_WIDTH 32
#define CHAR_HEIGHT 32
#define CHAR_COLLISION_MARGIN 8

// Character status
#define CHAR_RUN 1
#define CHAR_JUMP 2
#define CHAR_FIRE 3
#define CHAR_DIE 4
int charStatus = CHAR_RUN;

// Run parameters
#define RUN_IMAGE_MAX 2
int charAniIndex = 1;
int charAniDir = 1;
boolean drawBg = true;

// Jump parameters
#define JUMP_MAX 20
#define JUMP_STEP 4
#define JUMP_IMAGE_INDEX 3
#define FIRE_IMAGE_INDEX 4
#define DIE_IMAGE_INDEX 5
int charJumpIndex = 0;
int charJumpDir = JUMP_STEP;
int prevPosX = CHAR_POS_X;
int prevPosY = CHAR_POS_Y;

// Object parameters
#define OBSTACLE_MAX 3
#define OBSTACLE_MOVE 3
#define OBSTACLE_WIDTH 3
#define OBSTACLE_HEIGHT 4
#define OBS_POS_Y 58
#define OBSTACLE_DEL_THRESHOLD 5
int obstacleX[] = {0, 0, 0};
int obstacleCount = 0;
unsigned long obstacleTime;

#define ENEMY_MAX 1
#define ENEMY_DELAY 4000
#define ENEMY_MOVE 2
#define ENEMY_WIDTH 16
#define ENEMY_HEIGHT 16
#define ENEMY_POS_Y 40
#define ENEMY_START_X 128
#define ENEMY_RUN_IMAGE_INDEX 0
#define ENEMY_DIE_IMAGE_INDEX 1
#define ENEMY_COLLISION_MARGIN 1
int enemyX[] = {0};
int prevEnemyPosX[] = {0};
int enemyCount = 0;
unsigned long enemyTime;

#define BULLET_MAX 1
#define BULLET_MOVE 3
#define BULLET_WIDTH 3
#define BULLET_POS_Y 48
#define BULLET_START_X 44
int bulletX[] = {0};
int prevBulletPosX[] = {0};
int bulletCount = 0;
unsigned long bulletTime;

#define BONE_MAX 3
#define BONE_MOVE 4
#define BONE_WIDTH 8
#define BONE_HEIGHT 8
#define BONE_POS_Y 15
#define BONE_START_X 128
#define BONE_DEL_THRESHOLD 10
int boneX[] = {0, 0, 0};
int prevBonePosX[] = {0, 0, 0};
int boneCount = 0;
unsigned long boneTime;

// Utilities
int getOffset(int s);
void initUserInput();
void stopUntilUserInput();
void setMenuMode();
void setGameMode();
void setResultMode();
void setCreditMode();
unsigned long getRandTime();
void checkInput();
void updateMove();
void checkCollision();
void draw();

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

    // Read high score from eeprom
  while (!eeprom_is_ready()); // Wait for EEPROM to be ready
  cli();
  gameHighScore = eeprom_read_word((uint16_t*)eepromAddr);
  sei();
  if(gameHighScore < 0 || gameHighScore > 65500) gameHighScore = 0;

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
  display.drawBitmap(105,30,(const unsigned char*)pgm_read_word(&(enemy_anim[ENEMY_RUN_IMAGE_INDEX])),16,16,1);
  display.drawBitmap(108,15,bone,8,8,1);
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
      checkCollision();
      
      // Draw game screen
      draw();

      if(charStatus == CHAR_DIE) {
        setResultMode();
      }
      
    }
    else if (gameState == STATUS_RESULT) {  // Draw a Game Over screen w/ score
      if (gameScore > gameHighScore) { 
        gameHighScore = gameScore; 
        cli();
        eeprom_write_word((uint16_t*)eepromAddr, gameHighScore);
        sei();
      }  // Update game score
      display.display();  // Make sure final frame is drawn
      
      // Draw game over screen
      display.drawRect(16,8,96,48, WHITE);  // Box border
      display.fillRect(17,9,94,46, BLACK);  // Black out the inside
      //display.drawBitmap(30,12,gameover,72,14,1);
      
      display.setCursor(56 - getOffset(gameScore),30);
      display.print(gameScore);
      display.setCursor(69,30);
      display.print(F("Score"));
    
      display.setCursor(56 - getOffset(gameHighScore),42);
      display.print(gameHighScore);
      display.setCursor(69,42);
      display.print(F("High"));

      display.display();

      stopUntilUserInput();    // Wait until user touch the button
      gameState = STATUS_MENU;  // Then start the game paused
      gameScore = 0;  // Reset score to 0
      setMenuMode();
    }
    else if (gameState == STATUS_CREDIT) {  // If the game is paused
      display.clearDisplay();
      display.drawBitmap(5,28,lobby,32,32,1);
      display.drawBitmap(35,-3,title,64,64,1);
      display.drawBitmap(105,30,(const unsigned char*)pgm_read_word(&(enemy_anim[ENEMY_RUN_IMAGE_INDEX])),16,16,1);
      display.drawBitmap(108,15,bone,8,8,1);
      display.drawBitmap(0,0,background2,64,64,1);
      display.drawBitmap(64,0,background2,64,64,1);
      
      display.setCursor(0,0);
      display.print("MADE");
      display.setCursor(0,5);
      display.print("by P&L");
      
      display.display();
      stopUntilUserInput();    // Wait until user touch the button
      setMenuMode();
    }
    // Initialize user input
    initUserInput();
  }
}

void checkInput(){
  if(up || aBut) {
    if(charStatus == CHAR_RUN){
      charStatus = CHAR_JUMP;
      charJumpIndex = 0;
      charJumpDir = JUMP_STEP;
      prevPosX = CHAR_POS_X;
      prevPosY = CHAR_POS_Y;
    }
  }
  else if(bBut){
    if(charStatus == CHAR_RUN){
      if(bulletCount < BULLET_MAX) {
            for(int i=0; i<BULLET_MAX; i++) {
              if(bulletX[i] < 1) {
                bulletX[bulletCount] = BULLET_START_X - BULLET_MOVE;
                charStatus = CHAR_FIRE;
                bulletCount++;
          }
        }
      }
    }
  }
}

void updateMove(){
  if(charStatus == CHAR_JUMP){
    charJumpIndex += charJumpDir;
    if(charJumpIndex >= JUMP_MAX) charJumpDir *= -1;
    // if jump ended
    if(charJumpIndex <= 0 && charJumpDir < 0) {
      charJumpDir = JUMP_STEP;
      charStatus = CHAR_RUN;
      display.fillRect(prevPosX, prevPosY, CHAR_WIDTH, CHAR_HEIGHT, BLACK);  // delete previous character drawing
    }
  }
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
          gameScore += OBSTACLE_SCORE;
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
          display.fillRect(enemyX[i] + ENEMY_MOVE, ENEMY_POS_Y, ENEMY_WIDTH, ENEMY_HEIGHT, BLACK);
          
          // delete enemy
          enemyX[i] = 0;
          enemyCount--;
        }
      }
    }
  }

  // Bullet move
  if(bulletCount > 0) {
    for(int i=0; i<BULLET_MAX; i++) {
      if(bulletX[i] > WIDTH - BULLET_MOVE) {
        // Bullet touched end of screen. delete bullet
        bulletX[i] = 0;
        bulletCount--;
      }
      else if(bulletX[i] > 0) {
        bulletX[i] += BULLET_MOVE;
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
        }
      }
    }
  }
}

void checkCollision() {
  // check obstacle touch
  if(obstacleCount > 0) {
    for(int i=0; i<OBSTACLE_MAX; i++) {
      if(obstacleX[i] > 0) {
        if(prevPosY + CHAR_HEIGHT >= OBS_POS_Y 
          && obstacleX[i] < prevPosX + CHAR_WIDTH - CHAR_COLLISION_MARGIN
          && obstacleX[i] + OBSTACLE_WIDTH > prevPosX + CHAR_COLLISION_MARGIN) {
          // Character stepped on obstacle. End game
          charStatus = CHAR_DIE;
          break;
        }
      }
    }
  }
  
  // check bullet touch
  if(bulletCount > 0) {
    for(int i=0; i<BULLET_MAX; i++) {
      if(bulletX[i] < 1) continue;
      for(int j=0; j<ENEMY_MAX; j++) {
        if(enemyX[j] < 1) continue;
        if(enemyX[j] < bulletX[i] + BULLET_WIDTH) {
          // Bullet touched enemy. delete bullet and enemy
          bulletX[i] = 0;
          bulletCount--;
          gameScore += ENEMY_KILL_SCORE;
          display.drawLine(prevBulletPosX[i], BULLET_POS_Y, prevBulletPosX[i] + BULLET_WIDTH, BULLET_POS_Y, BLACK); // Delete previous drawing
          
          enemyX[j] = -1;   // To draw broken enemy image, set this -1
        }
      }
    }
  }

  // check enemy touch
  if(enemyCount > 0) {
    for(int i=0; i<ENEMY_MAX; i++) {
      if(enemyX[i] > 0) {
        if(enemyX[i] <= CHAR_WIDTH) {
          // Character touched enemy. End game
          charStatus = CHAR_DIE;
          break;
        }
      }
    }
  }

  //check bone touch
  if(boneCount > 0) {
    for(int i=0; i<BONE_MAX; i++) {
      if(boneX[i] > 0) {
        if(boneX[i] <= prevPosX + CHAR_WIDTH 
        && prevPosY + CHAR_COLLISION_MARGIN <= BONE_POS_Y + BONE_HEIGHT) {
        display.fillRect(boneX[i] + BONE_MOVE, BONE_POS_Y, BONE_WIDTH, BONE_HEIGHT, BLACK);        
        boneX[i] = 0;
        boneCount--;
        gameScore += ITEM_SCORE;
        }
      }
    }
  }
}

void draw() {
  // draw background
  if(drawBg) {
    display.clearDisplay();
    display.drawLine(0, 62, 127, 62, WHITE);
    drawBg = false;
  }
  
  // draw char
  if(charStatus == CHAR_RUN) {
    charAniIndex += charAniDir;
    if(charAniIndex >= RUN_IMAGE_MAX || charAniIndex <= 0) charAniDir *= -1;
    display.fillRect(CHAR_POS_X, CHAR_POS_Y, CHAR_WIDTH, CHAR_HEIGHT, BLACK);
    display.drawBitmap(CHAR_POS_X, CHAR_POS_Y, (const unsigned char*)pgm_read_word(&(char_anim[charAniIndex])), CHAR_WIDTH, CHAR_HEIGHT, WHITE);
  } else if(charStatus == CHAR_JUMP) {
    display.fillRect(prevPosX, prevPosY, CHAR_WIDTH, CHAR_HEIGHT, BLACK);
    prevPosY = CHAR_POS_Y-charJumpIndex;
    display.drawBitmap(prevPosX, prevPosY, (const unsigned char*)pgm_read_word(&(char_anim[JUMP_IMAGE_INDEX])), CHAR_WIDTH, CHAR_HEIGHT, WHITE);
  } else if(charStatus == CHAR_FIRE) {
    prevPosY = CHAR_POS_Y;
    display.fillRect(CHAR_POS_X, prevPosY, CHAR_WIDTH, CHAR_HEIGHT, BLACK);
    display.drawBitmap(CHAR_POS_X, prevPosY, (const unsigned char*)pgm_read_word(&(char_anim[FIRE_IMAGE_INDEX])), CHAR_WIDTH, CHAR_HEIGHT, WHITE);
    charStatus = CHAR_RUN;
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

  //draw enemy
  if(enemyCount > 0) {
    for(int i=0; i<ENEMY_MAX; i++) {
      if(enemyX[i] == -1) {
        // Enemy dead image
        display.fillRect(prevEnemyPosX[i], ENEMY_POS_Y, ENEMY_WIDTH, ENEMY_HEIGHT, BLACK);  // clear previous drawing
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
        display.fillRect(prevEnemyPosX[i], ENEMY_POS_Y, ENEMY_WIDTH, ENEMY_HEIGHT, BLACK);  // clear previous drawing
        display.drawBitmap(enemyX[i], ENEMY_POS_Y, (const unsigned char*)pgm_read_word(&(enemy_anim[ENEMY_RUN_IMAGE_INDEX])), ENEMY_WIDTH, ENEMY_HEIGHT, WHITE);
        prevEnemyPosX[i] = enemyX[i];
      }
    }
  }

  // Draw bullet
  if(bulletCount > 0) {
    for(int i=0; i<BULLET_MAX; i++) {
      if(bulletX[i] < 1) continue;
      if(bulletX[i] > BULLET_START_X) {
        // Delete previous drawing
        display.drawLine(prevBulletPosX[i], BULLET_POS_Y, bulletX[i], BULLET_POS_Y, BLACK);
      }
      display.drawLine(bulletX[i], BULLET_POS_Y, bulletX[i] + BULLET_WIDTH, BULLET_POS_Y, WHITE);
      prevBulletPosX[i] = bulletX[i];
    }
  }
  
  // Character death animation
  if(charStatus == CHAR_DIE) {
    // Start game end drawing
    for(int i=prevPosY; i<=CHAR_POS_Y; i++) {
      display.fillRect(CHAR_POS_X, i, 32, 32, BLACK);
      display.drawBitmap(CHAR_POS_X, i, (const unsigned char*)pgm_read_word(&(char_anim[DIE_IMAGE_INDEX])), 32, 32, WHITE);
      display.display();
    }
    delay(400);
    return;
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

  // Character death animation
  if(charStatus == CHAR_DIE) {
    // Start game end drawing
    for(int i=prevPosY; i<=CHAR_POS_Y; i++) {
      display.fillRect(CHAR_POS_X, i, CHAR_WIDTH, CHAR_HEIGHT, BLACK);
      display.drawBitmap(CHAR_POS_X, i, (const unsigned char*)pgm_read_word(&(char_anim[DIE_IMAGE_INDEX])), CHAR_WIDTH, CHAR_HEIGHT, WHITE);
      display.display();
    }
    delay(400);
    return;
  }

  // Draw score
  if(prevGameScore != gameScore) {
    int tempS = gameScore+(int)((millis() - startTime) / 1000);
    int margin = 120 - getOffset(tempS);
    display.fillRect(margin, 1, 127, 9, BLACK);
    display.setCursor(margin, 1);
    display.print(gameScore);
    prevGameScore = gameScore;
  }
  
  // Show on screen
  display.display();
}

int getOffset(int s) {
  if (s > 9999) { return 20; }
  if (s > 999) { return 15; }
  if (s > 99) { return 10; }
  if (s > 9) { return 5; }
  return 0;
}

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

void setGameMode() {
  gameState = STATUS_PLAYING;
  drawBg = true;
  gameScore = 0;
  prevGameScore = -1;
  gameEnd = false;
  charAniIndex = 0;
  charAniDir = 1;
  charStatus = CHAR_RUN;
  bulletCount = 0;
  boneCount = 0;
  delay(300);
  for(int i=0; i<OBSTACLE_MAX; i++) obstacleX[i] = 0;
  obstacleCount = 0;
  for(int i=0; i<ENEMY_MAX; i++) {
    enemyX[i] = 0;
    prevEnemyPosX[i] = 0;
  }
  enemyCount = 0;
  for(int i=0; i<BULLET_MAX; i++) {
    bulletX[i] = 0;
    prevBulletPosX[i] = 0;
  }
  bulletCount = 0;

  for(int i=0; i<BONE_MAX; i++) {
    boneX[i] = 0;
    prevBonePosX[i] = 0;
  }

  startTime = millis();
  obstacleTime = startTime + 2000;
  enemyTime = startTime + 7000;
}

void setResultMode() {
  gameState = STATUS_RESULT;
  delay(200);
}

void setCreditMode() {
  gameState = STATUS_CREDIT;
  delay(200);
}

unsigned long getRandTime() {
  return millis() + 100 * random(12, 30);
}
