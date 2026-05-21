#include "unity_config.h"
#include <stdio.h>
#include <stm32h7xx.h>
#include <vterm.h>

static void delay(int ms) {
  uint32_t cycles = SystemCoreClock / 1000 * ms;
  DWT->CTRL |= 1;       // включаем аппаратный счётчик тактов
  DWT->CYCCNT = 0;       // обнуляем
  while (DWT->CYCCNT < cycles)
    __asm("nop");
}

void unity_output_start() {
  vterm_init(115200);
  delay(1000);           // ждём 1с, чтобы PlatformIO успел открыть терминал
}

void unity_output_char(char ch) { putchar(ch); }
