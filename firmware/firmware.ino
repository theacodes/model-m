/* Copyright 2019 Alethea Flowers, Licensed under the MIT license. See LICENSE for more details */
/*
This program is for the Teensy 3.5 and requires Teensyduino.

Before uploading, modify the following settings in the Tools menu of the Arduino IDE:
- Board: Teensy 3.5
- USB Type: Serial + Keyboard + Mouse + Joystick
- CPU Speed: 120Mhz
- Keyboard Layout: US English

*/

#include <Adafruit_NeoPixel.h>

static const int settle_time = 20;  // Microseconds
static const int debounce_factor = 50; // Milliseconds
static const int strobe_pins_start = 27;
static const int sense_pins[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 24, 25, 26};

static const int rows = 8;
static const int cols = 16;

int keymap[rows][cols] = {
  /*    0                         1                        2             3                                        4             5             6                       7              8                     9                  10                   11               12                  13                14                15 */
  /*0*/ {0,                       0,                       KEY_ESC,      0,                                       KEY_F4,       KEY_G,        KEY_F5,                 KEY_H,         KEY_F6,               0,                 KEY_QUOTE,           0,               KEYPAD_0,           KEYPAD_PERIOD,    KEY_UP,           MODIFIERKEY_LEFT_ALT},
  /*1*/ {0,                       MODIFIERKEY_SHIFT,       KEY_TAB,      KEY_CAPS_LOCK,                           KEY_F3,       KEY_T,        KEY_BACKSPACE,          KEY_Y,         KEY_RIGHT_BRACE,      KEY_F7,            KEY_LEFT_BRACE,      KEYPAD_4,        KEYPAD_5,           KEYPAD_6,         0,                0},
  /*2*/ {MODIFIERKEY_LEFT_CTRL,   0,                       KEY_TILDE,    KEY_F1,                                  KEY_F2,       KEY_5,        KEY_F9,                 KEY_6,         KEY_EQUAL,            KEY_F8,            KEY_MINUS,           KEY_DELETE,      KEY_INSERT,         KEY_PAGE_UP,      KEY_HOME,         0},
  /*3*/ {0,                       0,                       KEY_1,        KEY_2,                                   KEY_3,        KEY_4,        KEY_F10,                KEY_7,         KEY_8,                KEY_9,             KEY_0,               KEY_F11,         KEY_F12,            KEY_PAGE_DOWN,    KEY_END,          KEY_PRINTSCREEN},
  /*4*/ {0,                       0,                       KEY_Q,        KEY_W,                                   KEY_E,        KEY_R,        0,                      KEY_U,         KEY_I,                KEY_O,             KEY_P,               KEYPAD_7,        KEYPAD_8,           KEYPAD_9,         KEYPAD_PLUS,      MODIFIERKEY_LEFT_SHIFT},
  /*5*/ {0,                       0,                       KEY_A,        KEY_S,                                   KEY_D,        KEY_F,        KEY_BACKSLASH,          KEY_J,         KEY_K,                KEY_L,             KEY_SEMICOLON,       KEYPAD_1,        KEYPAD_2,           KEYPAD_3,         KEYPAD_ENTER,     0},
  /*6*/ {MODIFIERKEY_RIGHT_CTRL,  MODIFIERKEY_RIGHT_SHIFT, KEY_Z,        KEY_X,                                   KEY_C,        KEY_V,        KEY_ENTER,              KEY_M,         KEY_COMMA,            KEY_PERIOD,        0,                   KEY_NUM_LOCK,    KEYPAD_SLASH,       KEYPAD_MINUS,     KEY_PAUSE,        0},               
  /*7*/ {0,                       0,                       0,            0,                                       0,            KEY_B,        KEY_SPACE,              KEY_N,         0,                    0,                 KEY_SLASH,           KEY_DOWN,        KEY_RIGHT,          KEYPAD_ASTERIX,   KEY_LEFT,         MODIFIERKEY_RIGHT_ALT}
};


/* Keymap modifications. These are done here to keep the above keymap matching the physical keys */
void user_keymap() {
  // Map caps lock to command/windows/gui key
  keymap[1][3] = MODIFIERKEY_LEFT_GUI;
  // Make printscr, scroll lock, and pause media keys (prev, next, and play/pause)
  keymap[3][15] = KEY_MEDIA_PREV_TRACK;
  keymap[4][15] = KEY_MEDIA_NEXT_TRACK;
  keymap[6][14] = KEY_MEDIA_PLAY_PAUSE;
}

/*
*
* DRIVER CODE ONLY BELOW HERE.
*
*/


struct Keystate {
  bool pressed = 0;
  unsigned long long last_change = 0;
};

Keystate keystate[rows][cols] = {};
bool numlock = false;

Adafruit_NeoPixel pixels(3, 14, NEO_RGB + NEO_KHZ800);


void setup() {
  user_keymap();

  for(int i = 0; i < rows; i++) {
    pinMode(strobe_pins_start + i, OUTPUT);
    digitalWrite(strobe_pins_start + i, false);
  }

  for(int i : sense_pins)  {
    pinMode(i, INPUT_PULLDOWN);
  }

  pixels.begin();
  pixels.setBrightness(50);
  pixels.clear();
  pixels.show();

  Serial.begin(9600);
}


void loop() {
  for(int row = 0; row < rows; row++) {
    auto strobe_pin = strobe_pins_start + row;
    digitalWrite(strobe_pin, HIGH);
    delayMicroseconds(settle_time);
  
    for(int col = 0; col < cols; col++) {
      auto now = millis();
      auto& state = keystate[row][col];
      auto pressed = digitalRead(sense_pins[col]);

      // Detect overflow on timing.
      if(state.last_change > now) {
        state.last_change = now - debounce_factor - 1;
      }

      // Enough time has passed, we can consider changing states.
      if(state.last_change < now - debounce_factor) {
        auto keycode = keymap[row][col];

        // key pressed
        if(state.pressed == false && pressed) {
          // Uncomment this line to debug matrix locations over the Arduino serial monitor.
          //Serial.printf("Matrix location %i, %i\n", row, col);
          if(keycode == KEY_NUM_LOCK) numlock = !numlock;
          
          Keyboard.press(keycode);

        // key released
        } else if(state.pressed == true && !pressed) {
          Keyboard.release(keycode);
        }
        
        state.pressed = pressed;
        state.last_change = now;
      }
    }
    digitalWrite(strobe_pin, LOW);
  }

  auto hue_ctrl = (millis() * 8);
  auto hue_1 = hue_ctrl % 65535;
  auto hue_2 = (hue_ctrl + 10000) % 65535;
  auto hue_3 = (hue_ctrl + 20000) % 65535;
  if(!numlock) {
    pixels.setPixelColor(0, pixels.gamma32(pixels.ColorHSV(hue_1)));
  } else {
    pixels.setPixelColor(0, pixels.Color(255, 255, 255));
  }
  pixels.setPixelColor(1, pixels.gamma32(pixels.ColorHSV(hue_2)));
  pixels.setPixelColor(2, pixels.gamma32(pixels.ColorHSV(hue_3)));
  pixels.show();
}
