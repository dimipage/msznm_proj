#include <ESP8266WiFi.h>
#include <espnow.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_DC   D1
#define OLED_RES  D3
#define OLED_CS   -1
#define BUZZ_PIN  D0

Adafruit_SSD1306 display(128, 64, &SPI, OLED_DC, OLED_RES, OLED_CS);

int scoreA = 0, scoreB = 0;
int setA = 0, setB = 0;
bool sidesSwapped = false;

struct Message { char action[16]; };

volatile bool msgPending = false;
char pendingAction[16];

void beepPoint() {
  tone(BUZZ_PIN, 1000, 100);
}

void beepUndo() {
  tone(BUZZ_PIN, 400, 200);
}

void beepSet() {
  tone(BUZZ_PIN, 800, 150);
  delay(180);
  tone(BUZZ_PIN, 1000, 150);
  delay(180);
  tone(BUZZ_PIN, 1200, 300);
}

void beepReset() {
  tone(BUZZ_PIN, 600, 400);
}

void checkSet() {
  bool aWins = scoreA >= 11 && (scoreA - scoreB) >= 2;
  bool bWins = scoreB >= 11 && (scoreB - scoreA) >= 2;
  if (aWins) {
    setA++;
    scoreA = 0;
    scoreB = 0;
    sidesSwapped = !sidesSwapped;
    beepSet();
  }
  if (bWins) {
    setB++;
    scoreA = 0;
    scoreB = 0;
    sidesSwapped = !sidesSwapped;
    beepSet();
  }
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.printf("Sets  %d : %d", setA, setB);

  display.setCursor(0, 10);
  if (!sidesSwapped) {
    display.print("A<---       --->B");
  } else {
    display.print("B<---       --->A");
  }

  display.setTextSize(3);
  char buf[16];
  snprintf(buf, sizeof(buf), "%2d:%2d", scoreA, scoreB);
  display.setCursor(0, 24);
  display.print(buf);

  display.setTextSize(1);
  display.setCursor(0, 56);
  display.print("  A            B");

  display.display();
}

void onReceive(uint8_t *mac, uint8_t *data, uint8_t len) {
  Message msg;
  memcpy(&msg, data, sizeof(msg));
  msg.action[15] = '\0';
  memcpy(pendingAction, msg.action, 16);
  msgPending = true;
}

void setup() {
  pinMode(BUZZ_PIN, OUTPUT);

  display.begin(SSD1306_SWITCHCAPVCC);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Starting...");
  display.display();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  esp_now_init();
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(onReceive);

  updateDisplay();
  tone(BUZZ_PIN, 1000, 200);
}

void loop() {
  if (msgPending) {
    msgPending = false;

    char action[16];
    strncpy(action, pendingAction, 16);

    if (sidesSwapped) {
      if      (strncmp(action, "A+", 2) == 0) strncpy(action, "B+", 16);
      else if (strncmp(action, "A-", 2) == 0) strncpy(action, "B-", 16);
      else if (strncmp(action, "B+", 2) == 0) strncpy(action, "A+", 16);
      else if (strncmp(action, "B-", 2) == 0) strncpy(action, "A-", 16);
    }

    if (strncmp(action, "A+", 2) == 0) {
      scoreA++;
      beepPoint();
    } else if (strncmp(action, "A-", 2) == 0) {
      scoreA = max(0, scoreA - 1);
      beepUndo();
    } else if (strncmp(action, "B+", 2) == 0) {
      scoreB++;
      beepPoint();
    } else if (strncmp(action, "B-", 2) == 0) {
      scoreB = max(0, scoreB - 1);
      beepUndo();
    } else if (strncmp(action, "RESET", 5) == 0) {
      scoreA = 0;
      scoreB = 0;
      beepReset();
    } else if (strncmp(action, "NEWGAME", 7) == 0) {
      scoreA = 0;
      scoreB = 0;
      setA = 0;
      setB = 0;
      sidesSwapped = false;
      beepReset();
    }

    checkSet();
    updateDisplay();
  }
}
