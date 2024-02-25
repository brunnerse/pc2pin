#include <stdio.h>
#include <string>
//#include <stdlib.h>
#include "pico/stdlib.h"

#define LED_PIN PICO_DEFAULT_LED_PIN



int main() { 
    char c;

    stdio_init_all();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    while (1) {
        gpio_put(LED_PIN, 1);
        int n = scanf("%c", c);
        if (n > 0) {
            gpio_put(LED_PIN, 0);
            printf("%c", c);
        }
    }
}

//      if (millis() - lastRecvTime >= BLINK_TIME_MS)
//        digitalWrite(LED_PIN, 1);
