#include <ESP8266WiFi.h>
#include <espnow.h>

// MAC address D1 mini
uint8_t receiverMAC[] = {0x2C, 0xF4, 0x32, 0x77, 0x84, 0x03};

#define BTN_A_PLUS  5   // D1
#define BTN_A_MINUS 4   // D2
#define BTN_B_PLUS  14  // D5
#define BTN_B_MINUS 12  // D6
#define VIBRATION_PIN D7
#define HOLD_TIME_MS 2000

struct Message { char action[16]; };

void onSent(uint8_t *mac, uint8_t status) {}

void sendAction(const char* action) {
  Message msg;
  strncpy(msg.action, action, sizeof(msg.action));
  esp_now_send(receiverMAC, (uint8_t*)&msg, sizeof(msg));
}

bool btnPressed(int pin) {
  return digitalRead(pin) == LOW;
}

// Vibration params
unsigned long lastVibration = 0;
#define VIBRATION_DEBOUNCE 100  // ms

// Combo state (NEWGAME)
unsigned long holdStartNewGame = 0;
bool holdingNewGame = false;

// Combo state (RESET)
unsigned long holdStartReset = 0;
bool holdingReset = false;

bool wasAp = false, wasAm = false, wasBp = false, wasBm = false;
unsigned long tAp = 0, tAm = 0, tBp = 0, tBm = 0;

bool justPressed(int pin, bool &wasDown, unsigned long &pressTime) {
  if (btnPressed(pin)) {
    if (!wasDown) { wasDown = true; pressTime = millis(); }
    return false;
  } else {
    if (wasDown) { wasDown = false; return true; }
    return false;
  }
}

void setup() {
  pinMode(BTN_A_PLUS,  INPUT_PULLUP);
  pinMode(BTN_A_MINUS, INPUT_PULLUP);
  pinMode(BTN_B_PLUS,  INPUT_PULLUP);
  pinMode(BTN_B_MINUS, INPUT_PULLUP);
  pinMode(VIBRATION_PIN, INPUT);
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_now_init();
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(onSent);
  esp_now_add_peer(receiverMAC, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
}

void loop() {
  bool aPlus  = btnPressed(BTN_A_PLUS);
  bool aMinus = btnPressed(BTN_A_MINUS);
  bool bPlus  = btnPressed(BTN_B_PLUS);
  bool bMinus = btnPressed(BTN_B_MINUS);

  // Combo NEWGAME
  if (aPlus && bPlus) {
    if (!holdingNewGame) { holdingNewGame = true; holdStartNewGame = millis(); }
    else if (millis() - holdStartNewGame > HOLD_TIME_MS) {
      sendAction("NEWGAME");
      holdingNewGame = false;
      delay(500);
    }
  } else {
    holdingNewGame = false;
  }

  // Combo RESET
  if (aMinus && bMinus) {
    if (!holdingReset) { holdingReset = true; holdStartReset = millis(); }
    else if (millis() - holdStartReset > HOLD_TIME_MS) {
      sendAction("RESET");
      holdingReset = false;
      delay(500);
    }
  } else {
    holdingReset = false;
  }

  if (digitalRead(VIBRATION_PIN) == HIGH) {
    if (millis() - lastVibration > VIBRATION_DEBOUNCE) {
      sendAction("RALLY");
      lastVibration = millis();
    }
  }

  // Single press
  if (!bPlus  && justPressed(BTN_A_PLUS,  wasAp, tAp)) sendAction("A+");
  if (!bMinus && justPressed(BTN_A_MINUS, wasAm, tAm)) sendAction("A-");
  if (!aPlus  && justPressed(BTN_B_PLUS,  wasBp, tBp)) sendAction("B+");
  if (!aMinus && justPressed(BTN_B_MINUS, wasBm, tBm)) sendAction("B-");

  delay(20);
}
