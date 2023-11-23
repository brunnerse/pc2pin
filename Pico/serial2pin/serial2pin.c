#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"

#define NUM_PINS 25

char mode[NUM_PINS];
int outVals[NUM_PINS];



int main() {
    stdio_init_all();

    for (int i = 0; i < NUM_PINS; i++) {
        mode[i] = -1;
        outVals[i] = 0;
    }


    // loop
    char cmdStr[100];
    while(true){
        scanf("%s\n", cmdStr);

        char action = cmdStr[0];
        int pin = atoi(cmdStr+1);

        const char* value = strstr(cmdStr, " ") + 1; 
        char *endS = strstr(value, " ");
        *endS = 0;
        int repeat = atoi(endS+1);
        endS = strstr(endS+1, " ");
        int delayVal = atoi(endS+1);
        printf("Test\n");
        printf("%d %d %d\n", pin, repeat, delayVal);
        printf("%c\n", action);
        printf("%s\n", value);
        printf("%c %d %s %d %d\n", action, pin, value, repeat, delayVal);
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