#include <stm32h7xx.h>
#include <led.h>

__attribute__((optimize("-O0")))
static void delay(int ms) {
    volatile int counter = SystemCoreClock / 1000 / 6 * ms;
    while (counter > 0) counter -= 1;
}

int main() {
    led_enable(led_all);
    while(1){
      led_toggle(led_yellow); delay(300);
    };
}
