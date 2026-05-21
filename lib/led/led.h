#pragma once
#include <stm32h7xx.h>

#define TOGGLE_BIT(REG, BIT)   ((REG) ^= (BIT))

typedef enum {
    led_green  = 1,       // битовая маска для зелёного
    led_yellow = 2,       // битовая маска для жёлтого
    led_red    = 4,       // битовая маска для красного
    led_all    = 1|2|4    // все три
} led_t;

void led_enable(led_t led);
void led_toggle(led_t led);
void led_on(led_t led);
void led_off(led_t led);
void led_disable(led_t led);
