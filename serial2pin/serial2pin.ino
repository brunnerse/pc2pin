#include <string.h>

#define NUM_PINS 25

char mode[NUM_PINS];
int outVals[NUM_PINS];


void setup() {
  Serial.begin(19200);

  for (int i = 0; i < NUM_PINS; i++) {
    mode[i] = -1;
    outVals[i] = 0;
  }
}

// the loop function runs over and over again forever
void loop() {
  if (Serial && Serial.available() > 0) {
    char action = Serial.read();
    int pin = Serial.parseInt();
    Serial.readStringUntil(' ');  // skip until next symbol
    String value = Serial.readStringUntil(' ');
    int repeat = Serial.parseInt();
    int delayVal = Serial.parseInt();
    // flush the rest
    Serial.readStringUntil('\n');

    value.trim();
    value.toLowerCase();

    bool error = false;

    for (int i = 0; i < repeat && !error; i++) {
      switch (action) {
        case 'r':
          if (mode[pin] == 'r' || mode[pin] == 'p') {
            Serial.printf("IN [Pin %2d]: %d\n", pin, digitalRead(pin));
          } else {
            Serial.printf("[ERROR] Pin %2d is not set to input\n", pin);
            error = true;
          }
          break;
        case 'w':
          if (mode[pin] == 'w') {
            unsigned int outVal = (value == "t") ? !outVals[pin] : atoi(value.c_str());
            if (outVal > 1) {
              Serial.printf("[ERROR] Value %d too high; use mode A for digital writing\n", outVal);
              error = true;
            } else {
              digitalWrite(pin, outVal);
              Serial.printf("OUT [Pin %2d]: %d\n", pin, outVal);
              outVals[pin] = outVal;
            }
          } else {
            Serial.printf("[ERROR] Pin %d is not set to output\n", pin);
            error = true;
          }
          break;
        case 'a':
          if (mode[pin] != 'w') {
            Serial.printf("IN A [Pin %2d]: %4d\n", pin, analogRead(pin));
          } else {
            Serial.printf("[ERROR] Cannot read analog: Pin %2d is set to output\n", pin);
            error = true;
          }
          break;
        case 'A':
          if (mode[pin] == 'w') {
            unsigned int outVal = atoi(value.c_str());
            if (outVal > 255) {
              Serial.printf("[ERROR] Value %d too high; analog value must be in range 0-255\n", outVal);
              error = true;
            } else {
              analogWrite(pin, outVal);
              Serial.printf("OUT A [Pin %2d]: %3d\n", pin, outVal);
            }
          } else {
            Serial.printf("[ERROR] Pin %d is not set to output\n", pin);
            error = true;
          }
          break;
        case 'p':
          if (value.endsWith("pullup")) {
            pinMode(pin, INPUT_PULLUP);
            Serial.printf("PINMODE [Pin %d]: input_pullup\n", pin);
            mode[pin] = 'p';
          }
           else if (value.startsWith("in")) {
            pinMode(pin, INPUT);
            Serial.printf("PINMODE [Pin %d]: input\n", pin);
            mode[pin] = 'r';
          } else if (value.startsWith("out")) {
            pinMode(pin, OUTPUT);
            Serial.printf("PINMODE [Pin %d]: output\n", pin);
            mode[pin] = 'w';
          } else {
            Serial.printf("[ERROR] Pin mode %s invalid\n", value.c_str());
            error = true;
          }
          break;
        default:
          Serial.printf("[ERROR] Command invalid\n");
          error = true;
      }
      delay(delayVal);
    }

  }

  //delay(10);
}
