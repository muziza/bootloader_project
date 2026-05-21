#include "led.h"

// Green: PB0, Yellow: PE1, Red: PB14

void led_enable(led_t led) {
    if (led & led_green) {
        SET_BIT(RCC->AHB4ENR, RCC_AHB4ENR_GPIOBEN);
        MODIFY_REG(GPIOB->MODER, GPIO_MODER_MODE0_Msk, GPIO_MODER_MODE0_0);
    }
    if (led & led_yellow) {
        SET_BIT(RCC->AHB4ENR, RCC_AHB4ENR_GPIOEEN);
        MODIFY_REG(GPIOE->MODER, GPIO_MODER_MODE1_Msk, GPIO_MODER_MODE1_0);
    }
    if (led & led_red) {
        SET_BIT(RCC->AHB4ENR, RCC_AHB4ENR_GPIOBEN);
        MODIFY_REG(GPIOB->MODER, GPIO_MODER_MODE14_Msk, GPIO_MODER_MODE14_0);
    }
}

void led_on(led_t led) {
    if (led & led_green)  SET_BIT(GPIOB->BSRR, GPIO_BSRR_BS0);   // BSRR — атомарная запись
    if (led & led_yellow) SET_BIT(GPIOE->BSRR, GPIO_BSRR_BS1);
    if (led & led_red)    SET_BIT(GPIOB->BSRR, GPIO_BSRR_BS14);
}

void led_off(led_t led) {
    if (led & led_green)  SET_BIT(GPIOB->BSRR, GPIO_BSRR_BR0);   // BR — reset-биты [31:16]
    if (led & led_yellow) SET_BIT(GPIOE->BSRR, GPIO_BSRR_BR1);
    if (led & led_red)    SET_BIT(GPIOB->BSRR, GPIO_BSRR_BR14);
}

void led_toggle(led_t led) {
    if (led & led_green)  TOGGLE_BIT(GPIOB->ODR, GPIO_ODR_OD0);   // ODR — XOR для toggle
    if (led & led_yellow) TOGGLE_BIT(GPIOE->ODR, GPIO_ODR_OD1);
    if (led & led_red)    TOGGLE_BIT(GPIOB->ODR, GPIO_ODR_OD14);
}

void led_disable(led_t led) {
    if (led & led_green)  SET_BIT(GPIOB->MODER, GPIO_MODER_MODE0_Msk);  // Analog (11)
    if (led & led_yellow) SET_BIT(GPIOE->MODER, GPIO_MODER_MODE1_Msk);
    if (led & led_red)    SET_BIT(GPIOB->MODER, GPIO_MODER_MODE14_Msk);
}
