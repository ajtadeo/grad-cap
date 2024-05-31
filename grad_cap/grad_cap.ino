#include <LiquidCrystal.h>
#include <ArduinoBLE.h>
#include <string.h>
#include "pitches.h"

#define D4 2
#define D5 3
#define D6 4
#define D7 5
#define RESET 6
#define ENABLE 7

#define LED_GOLD1 21
#define LED_GOLD2 20
#define LED_GOLD3 19
#define LED_GOLD4 18
#define LED_BLUE1 17
#define LED_BLUE2 16
#define LED_BLUE3 15
#define LED_BLUE4 14

#define BUZZER 8

#define ALTERNATE_STATE 0
#define SIDES_STATE 1
#define SCALE_STATE 2

int state = ALTERNATE_STATE;
unsigned long previousMillis = 0;
unsigned long previousStateMillis = 0;
bool playingMusic = false;

LiquidCrystal lcd(RESET, ENABLE, D4, D5, D6, D7);

int alternateOrder[] = {LED_GOLD1, LED_BLUE1, LED_GOLD2, LED_BLUE2, LED_GOLD3, LED_BLUE3, LED_GOLD4, LED_BLUE4};
int currentLED = 0;
bool blueSide = true;
int lcdIndex = 0;

int tempo = 100; // bpm
int melody[] = {
  NOTE_C4, 2, NOTE_B3, 8, NOTE_C4, 8, NOTE_D4, 4, 
  NOTE_A3, 2, NOTE_G3, 2,
  NOTE_F3, 2, NOTE_E3, 8, NOTE_F3, 8, NOTE_G3, 4,
  NOTE_D3, 1,
  NOTE_E3, 2, NOTE_FS3, 8, NOTE_G3, 8, NOTE_A3, 4,
  NOTE_D4, 2, NOTE_G3, 2, 
  NOTE_C4, 2, NOTE_C4, 8, NOTE_B3, 8, NOTE_A3, 4,
  NOTE_G3, -2, NOTE_D3, 4,
  NOTE_C4, 2, NOTE_B3, 8, NOTE_C4, 8, NOTE_D4, 4,
  NOTE_A3, 2, NOTE_G3, 2,
  NOTE_F3, 2, NOTE_E3, 8, NOTE_F3, 8, NOTE_G3, 4,
  NOTE_D3, 1,
  NOTE_E3, 2, NOTE_FS3, 8, NOTE_G3, 8, NOTE_A3, 4,
  NOTE_D4, 2, NOTE_G3, 2, 
  NOTE_F4, 2, NOTE_F4, 8, NOTE_E4, 8, NOTE_D4, 4,
  NOTE_E4, 1,
  NOTE_A3, 2, NOTE_B3, 8, NOTE_C4, 8, NOTE_D4, 4,
  NOTE_G3, 2, NOTE_C4, 2,
  NOTE_C4, 2, NOTE_F4, 8, NOTE_E4, 8, NOTE_D4, 4,
  NOTE_C4, -2, NOTE_D4, 4,
  NOTE_G4, 2, NOTE_FS4, 8, NOTE_G4, 8, NOTE_A4, 4,
  NOTE_E4, 2, NOTE_D4, 2,
  NOTE_C4, 2, NOTE_B3, 8, NOTE_C4, 8, NOTE_D4, 4,
  NOTE_A3, 4,
  NOTE_B3, 2, NOTE_CS4, 8, NOTE_D4, 8, NOTE_E4, 4,
  NOTE_A4, 2, NOTE_D4, 2,
  NOTE_G4, 2, NOTE_G4, 8, NOTE_FS4, 8, NOTE_E4, 4,
  NOTE_D4, 1,
  NOTE_G4, 2, NOTE_FS4, 8, NOTE_G4, 8, NOTE_A4, 4,
  NOTE_E4, 2, NOTE_D4,
  NOTE_C4, 2, NOTE_B3, 8, NOTE_C4, 8, NOTE_D4, 4,
  NOTE_A3, 1,
  NOTE_B3, 2, NOTE_CS4, 8, NOTE_D4, 8, NOTE_E4, 4,
  NOTE_A4, 2, NOTE_D4, 2,
  NOTE_C4, 2, NOTE_C4, 8, NOTE_B3, 8, NOTE_A3, 4,
  NOTE_B3, 1,
  NOTE_E4, 2, NOTE_FS4, 8, NOTE_G4, 8, NOTE_A4, 2,
  NOTE_D4, 2, NOTE_G4, 2,
  NOTE_C5, 2, NOTE_C5, 8, NOTE_B4, 8, NOTE_A4, 4,
  NOTE_G4, 4, NOTE_G4, 8, NOTE_G4, 8, NOTE_G4, 2
};
int notes = sizeof(melody) / sizeof(melody[0]) / 2;
int wholenote = (60000 * 4) / tempo; // ms
int divider = 0, noteDuration = 0;
unsigned long noteStartTime = 0; // Time when the current note started playing
int currentNote = 0; // Current note being played
bool isPlayingNote = false; // Flag to check if a note is currently playing

BLEService buzzerService("180A");
BLEByteCharacteristic switchCharacteristic("2A57", BLERead | BLEWrite);

void setup() {
  // LCD
  lcd.begin(16, 2);
  lcd.clear();

  // LED's
  pinMode(LED_GOLD1, OUTPUT);
  pinMode(LED_GOLD2, OUTPUT);
  pinMode(LED_GOLD3, OUTPUT);
  pinMode(LED_GOLD4, OUTPUT);
  pinMode(LED_BLUE1, OUTPUT);
  pinMode(LED_BLUE2, OUTPUT);
  pinMode(LED_BLUE3, OUTPUT);
  pinMode(LED_BLUE4, OUTPUT);

  // BLE
  pinMode(BUZZER, OUTPUT);
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");

    while (1);
  }
  BLE.setLocalName("Alyssa's Grad Cap");
  BLE.setAdvertisedService(buzzerService);
  buzzerService.addCharacteristic(switchCharacteristic);
  BLE.addService(buzzerService);
  switchCharacteristic.writeValue(0);
  BLE.advertise();
}

void loop() {
  BLEDevice central = BLE.central();
  if (central) {
    while (central.connected()) {
      if (switchCharacteristic.written()) {
        if (switchCharacteristic.value() == 1) {   // any value other than 0
          playingMusic = true;
          divider = 0;
          noteDuration = 0;
          noteStartTime = 0;
          currentNote = 0;
        } else if (switchCharacteristic.value() == 0) {
          playingMusic = false;
          divider = 0;
          noteDuration = 0;
          noteStartTime = 0;
          currentNote = 0;
        }
      }

      unsigned long currentMillis = millis();

      // buzzer
      if (playingMusic) {
        playMusic(currentMillis);
      }

      // LED and LCD
      if (currentMillis - previousMillis >= 1000) {
        previousMillis = currentMillis;
        displayLEDs(currentMillis);
        displayLCD();
      }
    }
  } else {
    unsigned long currentMillis = millis();

    // LED and LCD
    if (currentMillis - previousMillis >= 1000) {
      previousMillis = currentMillis;
      displayLEDs(currentMillis);
      displayLCD();
    }
  }
}

void playMusic(unsigned long currentMillis) {
  if (currentNote < notes * 2) {
    if (!isPlayingNote) {
      // Calculate note duration
      divider = melody[currentNote + 1];
      if (divider > 0) {
        noteDuration = (wholenote) / divider;
      } else if (divider < 0) {
        noteDuration = (wholenote) / abs(divider);
        noteDuration *= 1.5; // increases the duration in half for dotted notes
      }
      tone(BUZZER, melody[currentNote], noteDuration * 0.9);
      noteStartTime = currentMillis;
      isPlayingNote = true;
    } else if (currentMillis - noteStartTime >= noteDuration) {
      noTone(BUZZER);
      currentNote += 2;
      isPlayingNote = false;
    }
  } else {
    playingMusic = false;
  }
}

void displayLEDs(unsigned long currentMillis) {
  // LEDs
  if (state == ALTERNATE_STATE) {
    // Turn off all LEDs
    for (int i = 0; i < 8; i++) {
      digitalWrite(alternateOrder[i], LOW);
    }

    // Turn on the current LED
    digitalWrite(alternateOrder[currentLED], HIGH);

    // Move to the next LED
    currentLED++;
    if (currentLED >= 8) {
      currentLED = 0;
    }
  } else if (state == SIDES_STATE) {
    // Turn off all LEDs
    for (int i = 0; i < 8; i++) {
      digitalWrite(alternateOrder[i], LOW);
    }

    // Turn on one side
    if (blueSide) {
      digitalWrite(LED_BLUE1, HIGH);
      digitalWrite(LED_BLUE2, HIGH);
      digitalWrite(LED_BLUE3, HIGH);
      digitalWrite(LED_BLUE4, HIGH);
    } else {
      digitalWrite(LED_GOLD1, HIGH);
      digitalWrite(LED_GOLD2, HIGH);
      digitalWrite(LED_GOLD3, HIGH);
      digitalWrite(LED_GOLD4, HIGH);
    }

    blueSide = !blueSide;
  } else if (state == SCALE_STATE) {
    // Turn off all LEDs
    for (int i = 0; i < 8; i++) {
      digitalWrite(alternateOrder[i], LOW);
    }

    // Turn on one row of LED's
    digitalWrite(alternateOrder[currentLED], HIGH);
    digitalWrite(alternateOrder[currentLED + 1], HIGH);

    // Move to the next row
    currentLED = currentLED + 2;
    if (currentLED >= 8) {
      currentLED = 0;
    }
  }

  // check if we should switch state
  if (currentMillis - previousStateMillis >= 16000) {
    previousStateMillis = currentMillis;
    state = (state + 1) % 3;
  }
}

void displayLCD() {
  lcd.clear();
  if (playingMusic) {
    String line1 = "Now Playing:";
    String line2 = "<< Pomp and Circumstance >> ";

    lcd.setCursor(2, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2.substring(lcdIndex));  // Print the substring of text from position lcdIndex to the end
    if (lcdIndex > 0) {
      lcd.print(line2.substring(0, lcdIndex));  // Print the beginning of the text to complete the scroll
    }
    lcdIndex++;
    if (lcdIndex >= line2.length()) {
      lcdIndex = 0;
    }
  } else {
    String line1 = "<< Happy Graduation! >> ";
    String line2 = "Class of 2024";

    // line 1
    lcd.setCursor(0, 0);
    lcd.print(line1.substring(lcdIndex));  // Print the substring of text from position lcdIndex to the end
    if (lcdIndex > 0) {
      lcd.print(line1.substring(0, lcdIndex));  // Print the beginning of the text to complete the scroll
    }
    lcdIndex++;
    if (lcdIndex >= line1.length()) {
      lcdIndex = 0;
    }
    // line 2
    lcd.setCursor(1, 1);
    lcd.print(line2);
  }
}


