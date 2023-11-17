#include <string.h>

#define NUM_PINS 25

int mode[NUM_PINS];


void setup() {
  Serial.begin(19200);

  for (int i = 0; i < NUM_PINS; i++)
    mode[i] = -1;
}

// the loop function runs over and over again forever
void loop() {
  if (Serial && Serial.available() > 0) {
    char action = Serial.read();
    int pin = Serial.parseInt();
    String value = Serial.readStringUntil('\n');
    value.trim();
    value.toLowerCase();

    switch(action) {
      case 'r':
        if (mode[pin] == 'r' || mode[pin] == 'p') {
          Serial.printf("IN [Pin %d]: %2d\n", pin, digitalRead(pin));
        } else {
          Serial.printf("[ERROR] Pin %2d is not set to input\n", pin);
        }
        break;
      case 'w':
        if (mode[pin] == 'w') {
          // TODO value checking? PWM?
          int outVal = atoi(value.c_str());
          digitalWrite(pin, outVal);
          Serial.printf("OUT [Pin %2d]: %d\n", pin, outVal);
        } else {
          Serial.printf("[ERROR] Pin %d is not set to output\n", pin);
        }
        break;
      case 'p':
        if (value.startsWith("in")) {
          pinMode(pin, INPUT);
          Serial.printf("PINMODE [Pin %d]: input\n", pin);
          mode[pin] = 'r';
        } else if (value.startsWith("out")) {
          pinMode(pin, OUTPUT);
          Serial.printf("PINMODE [Pin %d]: output\n", pin);
          mode[pin] = 'w';
        } else if (value.endsWith("pullup")) {
          pinMode(pin, INPUT_PULLUP);
          Serial.printf("PINMODE [Pin %d]: input_pullup\n", pin);
          mode[pin] = 'p';
        } else {
          Serial.printf("[ERROR] Pin mode %s invalid\n", value.c_str());
        }
        break;
      default:
        Serial.printf("[ERROR] Command invalid\n");
    }
  }

  delay(50);
}
