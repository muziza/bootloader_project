#include "stm32h7xx.h"
#include <stdio.h>
#include <vterm.h>

extern uint32_t bootloader_SP;

void MemManage_Handler() {
  puts("\r\nMemory Management Fault exception!");
  uint32_t mmfsr = (SCB->CFSR) & SCB_CFSR_MEMFAULTSR_Msk;
  printf("MMFSR = 0x%02lx\n\r", mmfsr);
  if (mmfsr & 0x01) {
    puts("The processor attempted an instruction fetch from a location that "
      "does not permit execution");
  }
  if (mmfsr & 0x80)
    printf("MMFAR = 0x%lx\n\r", (SCB->MMFAR));
  NVIC_SystemReset();
}

void HardFault_Handler() {
  if (bootloader_SP) {
    __set_MSP(bootloader_SP);
    bootloader_SP = 0;
    vterm_init(115200);
    puts("\r\nApplication HardFault exception\r\n");
  } else {
    puts("\r\nBootloader HardFault exception\r\n");
  }
  NVIC_SystemReset();
}

void UsageFault_Handler() {
  puts("\r\nUsage Fault Exception!");
  // UFSR в старших 16 битах регистра CFSR
  uint32_t ufsr = (SCB->CFSR >> 16) & 0xFFFF;
  printf("UFSR = 0x%04lx\n\r", ufsr);
  NVIC_SystemReset();
}

void BusFault_Handler() {
  puts("\r\nBus Fault Exception!");

  // BFSR с 8 по 15 битах регистра CFSR
  uint32_t bfsr = (SCB->CFSR >> 8) & 0xFF;
  printf("BFSR = 0x%02lx\n\r", bfsr);
  
  if (bfsr & (1 << 7)) {
    uint32_t bfar = SCB->BFAR;
    printf("BFAR = 0x%08lx (fault address)\n\r", bfar);
  }
  
  NVIC_SystemReset();
}
