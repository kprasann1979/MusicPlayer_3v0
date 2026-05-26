#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <TM1637Display.h>

// ===== CONFIG =====
#define TEST_MODE false  // Set to true to test without MP3 module
#define TOTAL_FOLDERS 10 // Number of category folders (01-10)

// ===== Pins =====
#define ENC_CLK 3      // MUST be interrupt pin
#define ENC_DT  4
#define ENC_SW  5
#define PAUSE_BTN 9
#define VOL_UP 8
#define VOL_DOWN 10

#define DF_RX 11
#define DF_TX 12

#define DISP_CLK 6
#define DISP_DIO 7

SoftwareSerial dfSerial(DF_RX, DF_TX);
DFRobotDFPlayerMini dfPlayer;
TM1637Display display(DISP_CLK, DISP_DIO);

// ===== Variables =====
volatile int currentFolder = 1;     // Current folder (01-10)
volatile bool encoderMoved = false;

bool isPlaying = false;
bool isPaused  = false;
int currentSongInFolder = 0;  // Track current song in folder (0 = not set)
const int MAX_SONGS_PER_FOLDER = 255;  // Max songs to try in a folder

int currentVolume = 10;   // Volume (range: 0-30)

unsigned long displayTimer = 0;
int displayState = 0;  // 0=folder, 1=status (PLAY/PAUS/UP/DN)

// ===== INTERRUPT ENCODER =====
void readEncoderISR() {

  static unsigned long lastInterruptTime = 0;
  unsigned long now = millis();

  // Debounce (ignore noise within 5ms)
  if (now - lastInterruptTime < 5) return;
  lastInterruptTime = now;

  // Read both signals once
  bool clkState = digitalRead(ENC_CLK);
  bool dtState  = digitalRead(ENC_DT);

  // Direction detection
  if (clkState == dtState) {
    currentFolder++;
    if (currentFolder > TOTAL_FOLDERS) currentFolder = 1;
  } else {
    currentFolder--;
    if (currentFolder < 1) currentFolder = TOTAL_FOLDERS;
  }

  encoderMoved = true;
}

// Segment codes for TM1637 (a=bit0, b=bit1, c=bit2, d=bit3, e=bit4, f=bit5, g=bit6, dp=bit7)
const uint8_t DIGIT_SEG[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x02, 0x7F, 0x6D}; // 0-9
const uint8_t CHAR_F = 0x71;     // F (segments c, d, e, f)
const uint8_t CHAR_DASH = 0x40;  // - (middle segment g)

// ===== SHOW FOLDER NUMBER (F-01 format) =====
void showFolderNumber() {
  // Display as "F-01", "F-02", ... "F-10"
  uint8_t folder = (uint8_t)currentFolder;
  if (folder < 10) {
    uint8_t data[] = {CHAR_F, CHAR_DASH, DIGIT_SEG[0], DIGIT_SEG[folder]};  // F - 0 1-9
    display.setSegments(data, 4, 0);
  } else {
    uint8_t data[] = {CHAR_F, CHAR_DASH, DIGIT_SEG[folder / 10], DIGIT_SEG[folder % 10]};  // F - tens ones
    display.setSegments(data, 4, 0);
  }
}

// ===== DISPLAY PLAY =====
void displayPLAY() {
  uint8_t data[] = {0x73, 0x38, 0x77, 0x6E};
  display.setSegments(data, 4, 0);
}

// ===== DISPLAY PAUS =====
void displayPAUS() {
  uint8_t data[] = {0x73, 0x77, 0x3E, 0x6D};
  display.setSegments(data, 4, 0);
}

// ===== DISPLAY UP =====
void displayUP() {
  uint8_t data[] = {0x00, 0x00, 0x3E, 0x3F};  // _ _ U P
  display.setSegments(data, 4, 0);
}

// ===== DISPLAY DN =====
void displayDN() {
  uint8_t data[] = {0x00, 0x00, 0x5E, 0x54};  // _ _ D N
  display.setSegments(data, 4, 0);
}

// ===== PLAY FOLDER (plays all songs in folder, loops automatically) =====
void playFolder(int folder) {
  if (!TEST_MODE) {
    dfPlayer.stop();
    delay(100);
    dfPlayer.playFolder(folder, 1);
    currentSongInFolder = 1;
  }
}

// ===== CHECK FOR SONG FINISHED AND ADVANCE =====
void checkSongFinished() {
  if (!isPlaying || isPaused) return;
  
  if (!TEST_MODE) {
    // Check if DFPlayer has finished playing current song
    if (dfPlayer.available()) {
      int responseType = dfPlayer.readType();
      // Type 0 = play finished, Type 257 = current play time query response
      if (responseType == 0 || responseType == 257) {
        // Song finished, advance to next song in folder
        currentSongInFolder++;
        if (currentSongInFolder > MAX_SONGS_PER_FOLDER) {
          currentSongInFolder = 1;  // Loop back to first song
        }
        dfPlayer.playFolder(currentFolder, currentSongInFolder);
        Serial.print("Playing folder ");
        Serial.print(currentFolder);
        Serial.print(", song ");
        Serial.println(currentSongInFolder);
      }
    }
  }
}

// ===== PLAY BUTTON =====
void readPlayButton() {

  if (digitalRead(ENC_SW) == LOW) {

    delay(200);  // Debounce

    // Wait for button release
    while (digitalRead(ENC_SW) == LOW);

    // Play current folder (all songs will play in order)
    playFolder(currentFolder);

    displayPLAY();
    displayTimer = millis();
    displayState = 1;

    isPlaying = true;
    isPaused  = false;
  }
}

// ===== PAUSE BUTTON =====
void readPauseButton() {
  if (digitalRead(PAUSE_BTN) == LOW && isPlaying) {
    delay(200);

    if (!isPaused) {
      if (!TEST_MODE) dfPlayer.pause();
      displayPAUS();
      isPaused = true;
    } else {
      if (!TEST_MODE) dfPlayer.start();
      displayPLAY();
      isPaused = false;
    }

    displayTimer = millis();
    displayState = 1;

    while (digitalRead(PAUSE_BTN) == LOW);
  }
}

// ===== VOLUME UP =====
void readVolumeUpButton() {
  if (digitalRead(VOL_UP) == LOW) {
    delay(200);

    if (currentVolume < 30) {
      currentVolume++;
      if (!TEST_MODE) dfPlayer.volume(currentVolume);
      displayUP();
      displayTimer = millis();
      displayState = 1;
    }

    while (digitalRead(VOL_UP) == LOW);
  }
}

// ===== VOLUME DOWN =====
void readVolumeDownButton() {
  if (digitalRead(VOL_DOWN) == LOW) {
    delay(200);

    if (currentVolume > 0) {
      currentVolume--;
      if (!TEST_MODE) dfPlayer.volume(currentVolume);
      displayDN();
      displayTimer = millis();
      displayState = 1;
    }

    while (digitalRead(VOL_DOWN) == LOW);
  }
}

// ===== AUTO RETURN TO FOLDER DISPLAY =====
void autoReturnDisplay() {
  if (displayState == 1 && millis() - displayTimer > 3000) {
    displayState = 0;
    if (isPlaying) {
      showFolderNumber();
    }
  }
}

// ===== SETUP =====
void setup() {
  Serial.begin(9600);

  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT, INPUT_PULLUP);
  pinMode(ENC_SW, INPUT_PULLUP);
  pinMode(PAUSE_BTN, INPUT_PULLUP);
  pinMode(VOL_UP, INPUT_PULLUP);
  pinMode(VOL_DOWN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENC_CLK), readEncoderISR, FALLING);

  display.setBrightness(1);
  showFolderNumber();

  if (!TEST_MODE) {
    dfSerial.begin(9600);
    if (dfPlayer.begin(dfSerial)) {
      dfPlayer.volume(currentVolume);
    }
  }

  Serial.println("===== MUSIC PLAYER STARTED =====");
  Serial.print("Total folders: ");
  Serial.println(TOTAL_FOLDERS);
}

// ===== LOOP =====
void loop() {
  readPlayButton();
  readPauseButton();
  readVolumeUpButton();
  readVolumeDownButton();
  checkSongFinished();  // Check for song completion and advance to next

  if (encoderMoved) {
    encoderMoved = false;
    if (!isPlaying || displayState == 0) {
      showFolderNumber();
      displayTimer = millis();
      displayState = 0;
    }
  }

  autoReturnDisplay();
}
