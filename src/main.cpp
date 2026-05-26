#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <TM1637Display.h>

// ===== CONFIG =====
#define TEST_MODE false  // Set to true to test without MP3 module
#define TOTAL_FOLDERS 10 // Number of category folders (01-10)
#define SONGS_PER_FOLDER 99 // Max songs per folder (001-99)

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
volatile int currentFolder = 1;     // Current category folder (01-10)
volatile int currentSong = 1;       // Current song within folder (001-99)
volatile bool encoderMoved = false;

bool isPlaying = false;
bool isPaused  = false;
bool loopMode = true;    // 🔁 Default to loop mode for folder playback
bool songSelectMode = false; // When true, encoder selects song within folder

int currentVolume = 10;   // 🔊 Mid-level volume (range: 0-30)

unsigned long statusDisplayTime = 0;
bool showingStatus = false;

// ===== INTERRUPT ENCODER =====
void readEncoderISR() {

  static unsigned long lastInterruptTime = 0;
  unsigned long now = millis();

  // ✅ Debounce (ignore noise within 5ms)
  if (now - lastInterruptTime < 5) return;
  lastInterruptTime = now;

  // Read both signals once
  bool clkState = digitalRead(ENC_CLK);
  bool dtState  = digitalRead(ENC_DT);

  // ✅ Stable direction detection
  if (clkState == dtState) {
    if (songSelectMode) {
      currentSong++;
      if (currentSong > SONGS_PER_FOLDER) currentSong = 1;
    } else {
      currentFolder++;
      if (currentFolder > TOTAL_FOLDERS) currentFolder = 1;
    }
  } else {
    if (songSelectMode) {
      currentSong--;
      if (currentSong < 1) currentSong = SONGS_PER_FOLDER;
    } else {
      currentFolder--;
      if (currentFolder < 1) currentFolder = TOTAL_FOLDERS;
    }
  }

  encoderMoved = true;
}

// ===== SHOW FOLDER NUMBER =====
void showFolderNumber() {
  // Display folder number with leading zero (01, 02, ... 10)
  display.showNumberDecEx(currentFolder, 0, true, 2);
}

// ===== SHOW SONG NUMBER =====
void showSongNumber() {
  // Display song number with leading zeros (001, 002, ... 099)
  display.showNumberDecEx(currentSong, 0, true, 3);
}

// ===== DISPLAY PLAY =====
void displayPLAY() {
  uint8_t data[] = {0x73, 0x38, 0x77, 0x6E};
  display.setSegments(data);
}

// ===== DISPLAY PAUS =====
void displayPAUS() {
  uint8_t data[] = {0x73, 0x77, 0x3E, 0x6D};
  display.setSegments(data);
}

// ===== DISPLAY UP =====
void displayUP() {
  uint8_t data[] = {0x00, 0x00, 0x3E, 0x3F}; // _ _ U P (centered on right)
  display.setSegments(data, 4, 0);
}

// ===== DISPLAY DN =====
void displayDN() {
  uint8_t data[] = {0x00, 0x00, 0x5E, 0x54}; // _ _ D N (centered on right)
  display.setSegments(data, 4, 0);
}

// ===== DISPLAY LOOP =====
void displayLOOP() {
  uint8_t data[] = {0x38, 0x3F, 0x1C, 0x1C}; // L O O P
  display.setSegments(data, 4, 0);
}

// ===== DISPLAY NOLP =====
void displayNOLP() {
  uint8_t data[] = {0x5E, 0x38, 0x3F, 0x5E}; // N O L P
  display.setSegments(data, 4, 0);
}

// ===== DISPLAY SONG MODE =====
void displaySONG() {
  uint8_t data[] = {0x00, 0x00, 0x6D, 0x77}; // _ _ S G (S on pos 3, G on pos 4)
  display.setSegments(data, 4, 0);
}

// ===== PLAY FOLDER (with loop) =====
void playFolderLoop(int folder) {
  if (!TEST_MODE) {
    dfPlayer.stop();
    delay(100);
    dfPlayer.playFolder(folder, 1);
  }
}

// ===== PLAY SPECIFIC SONG IN FOLDER =====
void playSpecificSong(int folder, int song) {
  if (!TEST_MODE) {
    dfPlayer.stop();
    delay(100);
    dfPlayer.playFolder(folder, song);
  }
}

// ===== PLAY BUTTON =====
void readPlayButton() {

  if (digitalRead(ENC_SW) == LOW) {

    delay(200);  // Debounce

    unsigned long pressStartTime = millis();
    
    // Wait for button release or timeout (3 seconds for long press)
    while (digitalRead(ENC_SW) == LOW) {
      if (millis() - pressStartTime >= 3000) {
        // Long press detected - toggle loop mode
        loopMode = !loopMode;
        
        if (loopMode) {
          displayLOOP();
        } else {
          displayNOLP();
        }
        
        statusDisplayTime = millis();
        showingStatus = true;
        
        // Wait for button release
        while (digitalRead(ENC_SW) == LOW);
        return;
      }
    }
    
    // Short press - play current selection
    if (songSelectMode) {
      // In song select mode: play specific song
      playSpecificSong(currentFolder, currentSong);
    } else {
      // In folder select mode: play entire folder (loop)
      playFolderLoop(currentFolder);
    }

    displayPLAY();

    isPlaying = true;
    isPaused  = false;

    statusDisplayTime = millis();
    showingStatus = true;
  }
}

// ===== PAUSE BUTTON (also toggles song select mode when not playing) =====
void readPauseButton() {
  if (digitalRead(PAUSE_BTN) == LOW) {
    delay(200);

    if (isPlaying) {
      // If playing, toggle pause/resume
      if (!isPaused) {
        if (!TEST_MODE) dfPlayer.pause();
        displayPAUS();
        isPaused = true;
      } else {
        if (!TEST_MODE) dfPlayer.start();
        displayPLAY();
        isPaused = false;
      }
    } else {
      // If not playing, toggle song select mode
      songSelectMode = !songSelectMode;
      
      if (songSelectMode) {
        // Enter song selection mode
        currentSong = 1; // Reset to first song
        displaySONG();
        delay(1500); // Show "SG" briefly
        showSongNumber();
        Serial.print("Song Select Mode - Folder: ");
        Serial.print(currentFolder);
        Serial.print(", Song: ");
        Serial.println(currentSong);
      } else {
        // Exit song selection mode, return to folder display
        showFolderNumber();
        Serial.print("Folder Select Mode - Folder: ");
        Serial.println(currentFolder);
      }
    }

    statusDisplayTime = millis();
    showingStatus = true;

    while (digitalRead(PAUSE_BTN) == LOW);
  }
}

// ===== 🔊 VOLUME UP =====
void readVolumeUpButton() {
  if (digitalRead(VOL_UP) == LOW) {
    delay(200);

    if (currentVolume < 30) {
      currentVolume++;
      if (!TEST_MODE) dfPlayer.volume(currentVolume);
      displayUP();
    }

    statusDisplayTime = millis();
    showingStatus = true;

    while (digitalRead(VOL_UP) == LOW);
  }
}

// ===== 🔊 VOLUME DOWN =====
void readVolumeDownButton() {
  if (digitalRead(VOL_DOWN) == LOW) {
    delay(200);

    if (currentVolume > 0) {
      currentVolume--;
      if (!TEST_MODE) dfPlayer.volume(currentVolume);
      displayDN();
    }

    statusDisplayTime = millis();
    showingStatus = true;

    while (digitalRead(VOL_DOWN) == LOW);
  }
}

// ===== AUTO RETURN =====
void autoReturnDisplay() {

  if (isPaused) return;

  if (isPlaying && showingStatus && millis() - statusDisplayTime > 5000) {
    if (songSelectMode) {
      showSongNumber();
    } else {
      showFolderNumber();
    }
    showingStatus = false;
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
  Serial.print("Songs per folder: ");
  Serial.println(SONGS_PER_FOLDER);
}

// ===== LOOP =====
void loop() {
  readPlayButton();
  readPauseButton();
  readVolumeUpButton();
  readVolumeDownButton();

  if (encoderMoved && !isPaused) {
    encoderMoved = false;
    if (songSelectMode) {
      showSongNumber();
    } else {
      showFolderNumber();
    }
  }

  autoReturnDisplay();
}