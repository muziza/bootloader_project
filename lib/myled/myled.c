// #include "myled.h"
// #include <stm32h7xx.h>

// void myled_enable() {
//     RCC->AHB4ENR |= RCC_AHB4ENR_GPIOEEN;    // включаем тактирование GPIOE
//     GPIOE->MODER &= ~GPIO_MODER_MODE1_Msk;  // очищаем биты режима для pin1
//     GPIOE->MODER |= GPIO_MODER_MODE1_0;     // pin1 = Output (01)
// }

// void myled_toggle() {
//     GPIOE->ODR ^= 2; // XOR меняет бит 1: 0→1 и 1→0
// }

// void myled_disable() {
//     GPIOE->MODER &= ~GPIO_MODER_MODE1_Msk;
//     GPIOE->MODER |= GPIO_MODER_MODE1_0 | GPIO_MODER_MODE1_1; // pin1 = Analog (11)
// }

#include "myled.h"
#include <stm32h7xx.h>

void myled_enable() {
    SET_BIT(RCC->AHB4ENR, RCC_AHB4ENR_GPIOEEN);
    MODIFY_REG(GPIOE->MODER, GPIO_MODER_MODE1_Msk, GPIO_MODER_MODE1_0);
}

void myled_toggle() {
    TOGGLE_BIT(GPIOE->ODR, GPIO_ODR_OD1);
}

void myled_disable() {
    SET_BIT(GPIOE->MODER, GPIO_MODER_MODE1_Msk);
}
