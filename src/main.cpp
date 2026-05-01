#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <TM1637Display.h>

// ===== CONFIG =====
#define TEST_MODE false  // Set to true to test without MP3 module

// ===== Pins (UPDATED AS REQUESTED) =====
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
volatile int songNumber = 1;
volatile bool encoderMoved = false;
volatile unsigned long lastEncoderTime = 0;
volatile int stepSize = 1;

bool isPlaying = false;
bool isPaused  = false;

int currentVolume = 20;   // 🔊 NEW

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
  if (clkState == dtState)
    songNumber++;
  else
    songNumber--;

  // Wrap-around instead of clamp
  if (songNumber > 99) songNumber = 1;
  if (songNumber < 1)  songNumber = 99;

  encoderMoved = true;
}

// ===== SHOW NUMBER =====
void showNumber() {
  display.showNumberDecEx(songNumber, 0, false);
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
  uint8_t data[] = {0x3E, 0x3F}; // U P
  display.setSegments(data, 2, 1);
}

// ===== DISPLAY DN =====
void displayDN() {
  uint8_t data[] = {0x5E, 0x54}; // D N
  display.setSegments(data, 2, 1);
}

// ===== PLAY BUTTON =====
void readPlayButton() {

  if (digitalRead(ENC_SW) == LOW) {

    delay(200);

    if (!TEST_MODE) {
      dfPlayer.stop();
      delay(100);
      dfPlayer.play(songNumber);
    }

    displayPLAY();

    isPlaying = true;
    isPaused  = false;

    statusDisplayTime = millis();
    showingStatus = true;

    while (digitalRead(ENC_SW) == LOW);
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

    statusDisplayTime = millis();
    showingStatus = true;
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
    showNumber();
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
  pinMode(VOL_UP, INPUT_PULLUP);      // NEW
  pinMode(VOL_DOWN, INPUT_PULLUP);    // NEW

  attachInterrupt(digitalPinToInterrupt(ENC_CLK), readEncoderISR, FALLING);

  display.setBrightness(1);
  showNumber();

  if (!TEST_MODE) {
    dfSerial.begin(9600);
    if (dfPlayer.begin(dfSerial)) {
      dfPlayer.volume(currentVolume);
    }
  }

  Serial.println("===== MUSIC PLAYER STARTED =====");
}

// ===== LOOP =====
void loop() {
  readPlayButton();
  readPauseButton();
  readVolumeUpButton();     // NEW
  readVolumeDownButton();   // NEW

  if (encoderMoved && !isPaused) {
    encoderMoved = false;
    showNumber();
  }

  autoReturnDisplay();
}