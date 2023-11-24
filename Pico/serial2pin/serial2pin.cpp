#include <stdio.h>
#include <string>
#include <stdlib.h>
#include "pico/stdlib.h"

#define NUM_PINS 25

char mode[NUM_PINS];
int outVals[NUM_PINS];

int readUntil(char* target, char terminating, unsigned int maxLen) {
    for (int i = 0; i < maxLen; i++) {
        char c = getchar();
        if (c == terminating)
            return i;
        target[i] = c;
    }
    return maxLen;
}


int main() {
    stdio_init_all();

    for (int i = 0; i < NUM_PINS; i++) {
        mode[i] = -1;
        outVals[i] = 0;
    }


    // loop
    char chunk[10];
    while(true){
        char action = getchar(); 
        getchar();
        readUntil(chunk, ' ', 10);
        int pin = atoi(chunk);
        readUntil(chunk, ' ', 10);
        std::string value = chunk; 
        readUntil(chunk, ' ', 10);
        int repeat = atoi(chunk);
        readUntil(chunk, '\n', 10);
        int delayVal = atoi(chunk);

        printf("Test\n");
        printf("%d %d %d\n", pin, repeat, delayVal);
        printf("%c\n", action);
        printf("%s\n", value.c_str());
        printf("%c %d %s %d %d\n", action, pin, value.c_str(), repeat, delayVal);
        bool error = false;

        for (int i = 0; i < repeat && !error; i++) {
            switch (action) {
                case 'r':
                if (mode[pin] == 'r' || mode[pin] == 'p') {
                    printf("IN [Pin %2d]: %d\n", pin, gpio_get(pin));
                } else {
                    printf("[ERROR] Pin %2d is not set to input\n", pin);
                    error = true;
                }
                break;
                case 'w':
                if (mode[pin] == 'w') {
                    if (value == "n"){
                        printf("OUT [Pin %2d]: %d\n", pin, outVals[pin]);
                        break;
                    }
                    unsigned int outVal = (value == "t") ? !outVals[pin] : atoi(value.c_str());
                    if (outVal > 1) {
                    printf("[ERROR] Value %d too high; use mode A for digital writing\n", outVal);
                    error = true;
                    } else {
                        gpio_put(pin, outVal);
                        printf("OUT [Pin %2d]: %d\n", pin, outVal);
                        outVals[pin] = outVal;
                    }
                } else {
                    printf("[ERROR] Pin %d is not set to output\n", pin);
                    error = true;
                }
                break;
                case 'a':
                if (mode[pin] != 'w') {
                    //printf("IN A [Pin %2d]: %4d\n", pin, analogRead(pin));
                } else {
                    printf("[ERROR] Cannot read analog: Pin %2d is set to output\n", pin);
                    error = true;
                }
                break;
                case 'A':
                if (mode[pin] == 'w') {
                    unsigned int outVal = atoi(value.c_str());
                    if (outVal > 255) {
                    printf("[ERROR] Value %d too high; analog value must be in range 0-255\n", outVal);
                    error = true;
                    } else {
                    //analogWrite(pin, outVal);
                    printf("OUT A [Pin %2d]: %3d\n", pin, outVal);
                    outVals[pin] = outVal;
                    }
                } else {
                    printf("[ERROR] Pin %d is not set to output\n", pin);
                    error = true;
                }
                break;
                case 'p':
                if (value.find("pullup") != std::string::npos) {
                    gpio_init(pin);
                    gpio_set_dir(pin, GPIO_IN);
                    gpio_pull_up(pin);
                    //pinMode(pin, INPUT_PULLUP);
                    printf("PINMODE [Pin %d]: input_pullup\n", pin);
                    mode[pin] = 'p';
                }
                else if (value.find("in") == 0) {
                    gpio_init(pin);
                    gpio_set_dir(pin, GPIO_IN);
                    printf("PINMODE [Pin %d]: input\n", pin);
                    mode[pin] = 'r';
                } else if (value.find("out") == 0) {
                    gpio_init(pin);
                    gpio_set_dir(pin, GPIO_OUT);
                    printf("PINMODE [Pin %d]: output\n", pin);
                    mode[pin] = 'w';
                } else {
                    printf("[ERROR] Pin mode %s invalid\n", value.c_str());
                    error = true;
                }
                break;
                default:
                printf("[ERROR] Command invalid\n");
                error = true;
            }
            sleep_ms(delayVal);
            }


    }

    return 0;
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    while (true) {
        gpio_put(LED_PIN, 1);
        sleep_ms(1000);
        printf("Switching...");
        gpio_put(LED_PIN, 0);
        sleep_ms(250);
    }
}