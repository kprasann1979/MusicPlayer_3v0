# 🎵 Arduino Music Box v3.0 - Complete Edition with Volume Control

The most advanced version of our Arduino music box, featuring a rotary encoder for smooth song selection, a 4-digit display, play/pause functionality, **dedicated volume control buttons**, **automatic track advancement with display updates**. This version offers complete control over your music experience!

## 📋 What This Project Does

This feature-rich music box allows you to:
- **Select songs 1-99** using a smooth rotary encoder (like a volume knob)
- **See the current track number** on a 4-digit 7-segment display
- **Play and pause music** with dedicated buttons
- **Control volume** with separate up/down buttons (0-30)
- **Visual feedback** with "PLAY", "PAUS", "UP", and "DN" display modes
- **Automatic track advancement** - When a song finishes, automatically plays the next track and updates the display
- **Wrap-around selection** - 99 goes to 1, and 1 goes to 99

## 🛠️ What You'll Need

### Hardware Components
| Component | Quantity | Purpose |
|-----------|----------|---------|
| Arduino Nano (ATmega328P) | 1 | The brain of the project |
| DFPlayer Mini MP3 Module | 1 | Plays MP3 files |
| MicroSD Card (with MP3 files) | 1 | Stores your music |
| Rotary Encoder (with push button) | 1 | Smooth song selection and play button |
| Push Button | 3 | Pause, Volume Up, Volume Down |
| 4-Digit 7-Segment Display (TM1637) | 1 | Shows track numbers and status |
| Jumper Wires | Many | Connect everything |
| Breadboard | 1 | For prototyping |
| Speaker (3W, 4Ω) | 1 | For audio output |

### Software
- **PlatformIO** (or Arduino IDE)
- **Arduino Core** for ATmega328P
- **Libraries** (automatically installed by PlatformIO):
  - `DFRobotDFPlayerMini` - Controls the MP3 player
  - `SoftwareSerial` - Creates a serial connection for the DFPlayer
  - `TM1637 Driver` - Controls the 4-digit display

## 🔌 Wiring Diagram

### Arduino Nano Pin Connections

| Component | Arduino Pin | Notes |
|-----------|-------------|-------|
| Rotary Encoder CLK | Digital Pin 3 | **Must be interrupt pin** |
| Rotary Encoder DT | Digital Pin 4 | Data pin for rotation |
| Rotary Encoder SW | Digital Pin 5 | Push button (play) |
| Pause Button | Digital Pin 9 | Pause/resume control |
| Volume Up Button | Digital Pin 8 | Increase volume |
| Volume Down Button | Digital Pin 10 | Decrease volume |
| DFPlayer TX | Digital Pin 11 | Software Serial RX |
| DFPlayer RX | Digital Pin 12 | Software Serial TX |
| Display CLK | Digital Pin 6 | TM1637 clock |
| Display DIO | Digital Pin 7 | TM1637 data |
| DFPlayer VCC | 5V | Power |
| DFPlayer GND | GND | Ground |
| Display VCC | 5V | Power |
| Display GND | GND | Ground |
| DFPlayer SPK1/SPK2 | Speaker | Audio output |

### Rotary Encoder Wiring
- CLK → Pin 3
- DT → Pin 4  
- SW → Pin 5
- + → 5V
- GND → GND

### TM1637 Display Wiring
- CLK → Pin 6
- DIO → Pin 7
- VCC → 5V
- GND → GND

### Push Buttons Wiring
Each button has one terminal connected to its respective pin (9, 8, or 10) and the other terminal to GND. Internal pull-up resistors are used.

## 📁 Setting Up the SD Card

1. **Format your MicroSD card** as FAT32
2. **Create a folder** named `mp3` in the root directory
3. **Name your MP3 files** with 3-digit numbers:
   - `001.mp3` - Song 1
   - `002.mp3` - Song 2
   - ... up to `099.mp3` - Song 99
4. **Insert the SD card** into the DFPlayer module

## 🚀 How to Use

### Building and Uploading
1. **Connect your Arduino Nano** to your computer via USB
2. **Open the project** in PlatformIO (or Arduino IDE)
3. **Click "Build and Upload"** (or the upload button)
4. **Wait for completion** - you should see "MUSIC PLAYER STARTED" in the serial monitor

### Operating the Music Box
1. **Turn the rotary encoder** to select a song (1-99)
   - The display shows the current track number
   - **Wrap-around**: 99 → 1, and 1 → 99
2. **Press the rotary encoder** to play the selected song
   - The display will show "PLAY" and the song will start
   - After 5 seconds, it returns to showing the track number
   - **Auto-advance**: When the song finishes, the next track plays automatically and the display updates
3. **Press the pause button** to pause/resume
   - Display shows "PAUS" when paused
   - Display shows "PLAY" when resumed
   - The 3-hour timer also pauses and resumes accordingly
4. **Press the volume up button** to increase volume (0-30)
   - Display shows "UP" temporarily
5. **Press the volume down button** to decrease volume
   - Display shows "DN" temporarily

## 💡 How It Works

### Key Features Explained

#### Rotary Encoder Magic
- Uses **interrupts** for smooth, responsive control
- **Wrap-around navigation**: 99 goes to 1, and 1 goes to 99
- **Debouncing**: Ignores noise within 5ms for clean readings

#### Volume Control
- **Range**: 0 (mute) to 30 (maximum)
- **Default**: 20 (moderate volume)
- **Visual feedback**: "UP" and "DN" displayed when adjusting
- **Auto-return**: Returns to track number after 5 seconds

#### Smart Display
- Shows track numbers (1-99) normally
- Displays "PLAY" when a song starts (for 5 seconds)
- Displays "PAUS" when paused (stays until resumed)
- Displays "UP" when volume increased
- Displays "DN" when volume decreased

#### Test Mode
- Set `TEST_MODE = true` in the code to test without the DFPlayer
- Shows all actions in the serial monitor
- Perfect for debugging wiring or testing logic

### Code Highlights
```cpp
// Interrupt-driven encoder reading with debouncing
attachInterrupt(digitalPinToInterrupt(ENC_CLK), readEncoderISR, FALLING);

// Stable direction detection
if (clkState == dtState)
  songNumber++;
else
  songNumber--;

// Wrap-around instead of clamp
if (songNumber > 99) songNumber = 1;
if (songNumber < 1)  songNumber = 99;

// Volume control (0-30)
if (currentVolume < 30) {
  currentVolume++;
  if (!TEST_MODE) dfPlayer.volume(currentVolume);
  displayUP();
}
```

## 🔧 Troubleshooting

### Display Not Working
- Check CLK and DIO pins are connected correctly
- Ensure the display has power (5V and GND)
- Verify the TM1637 library is installed

### Encoder Not Responding
- **Critical**: CLK pin MUST be connected to pin 3 (interrupt pin)
- Check all encoder connections (CLK, DT, SW, +, GND)
- Test with a multimeter to ensure proper grounding

### Volume Buttons Not Working
- Check wiring for each button (pins 8, 9, 10)
- Ensure internal pull-up resistors are enabled (they are in the code)
- Test each button individually

### No Sound
- Check speaker connections (SPK1 and SPK2)
- Ensure MP3 files are correctly named (001.mp3 to 099.mp3)
- Verify volume is set (default is 20 out of 30)
- Check if the speaker is working

### Songs Not Playing
- Ensure the SD card is properly formatted and inserted
- Check that MP3 files are in the `mp3` folder
- Verify the DFPlayer has power and is connected correctly

## 📝 Notes for Beginners

### What is a Rotary Encoder?
A rotary encoder is like a digital volume knob. It can detect both rotation direction and speed, making it perfect for smooth menu navigation.

### What is TM1637?
The TM1637 is a driver chip that controls 4-digit 7-segment displays. It uses only 2 wires (clock and data) to communicate with the Arduino.

### What are Interrupts?
Interrupts allow the Arduino to respond immediately to events (like encoder rotation) without constantly checking them in the main loop. This makes the interface much more responsive.

### What is Debouncing?
Debouncing filters out electrical noise when buttons are pressed or the encoder is turned. This prevents false readings and makes the interface more reliable.

### Wrap-Around vs Clamp
- **Clamp**: Stops at limits (1-99 stays within range)
- **Wrap-around**: Loops around (99 → 1, 1 → 99)
This version uses wrap-around for a seamless experience.

## 🎯 Possible Enhancements

Once you've mastered this project, try these modifications:
- Add a mute button that remembers the previous volume
- Implement a sleep mode that turns off the display after inactivity
- Create a playlist mode that plays multiple songs in sequence
- Add RGB LEDs that change color with the music
- Implement shuffle/random play functionality
- Add a "favorites" button that remembers your top songs
- Display volume level as a percentage (0-100%)

## 📄 License

This project is open source and free to use for personal and educational purposes.

## 🙏 Credits

- **DFRobot** for the DFPlayer Mini library
- **akj7** for the TM1637 Driver library
- **Arduino community** for endless inspiration
- **You** for building this awesome project!

---

**Happy Coding! 🚀**

*If you have any questions or suggestions, feel free to open an issue or contribute to this project.*