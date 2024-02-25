#include <string.h>

#define LED_PIN 2
#define BLINK_TIME_MS 100

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))


#define BUFFER_SIZE 100

char buffer[BUFFER_SIZE];




void setup() {
  Serial.begin(19200);
  pinMode(LED_PIN, OUTPUT);
  while (!Serial) { 
  }
  digitalWrite(LED_PIN, 1);
}

// the loop function runs over and over again forever
void loop() {
  static unsigned long lastRecvTime = millis();

  if (Serial) {
    uint32_t n = Serial.available();
    if (n > 0) {
      digitalWrite(LED_PIN, 0);
      lastRecvTime = millis();
      n = Serial.readBytes(buffer, MIN(BUFFER_SIZE, n));
      Serial.write(buffer, n);
      Serial.flush();
    } else {
      if (millis() - lastRecvTime >= BLINK_TIME_MS)
        digitalWrite(LED_PIN, 1);
    }
  } else {
    digitalWrite(LED_PIN, 0);
  }
}
