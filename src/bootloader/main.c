#include <stm32h7xx.h>
#include <assert.h>
#include <stdio.h>
#include <vterm.h>
//
static void delay(int ms);
static int check_sram_app(void);
static int check_flash_app(void);
//
#define APP_SRAM_OFFSET 0x24000000
#define APP_FLASH_OFFSET 0x08020000
#define NUM_COMMANDS 6
#if NUM_COMMANDS > 9
#error NUM_COMMANDS must be less then 10 or change read_handler_index()
#endif

#ifndef FLASH_CR_PG2
  #define FLASH_CR_PG2     FLASH_CR_PG
#endif

#ifndef FLASH_SR_QW2
  #define FLASH_SR_QW2     FLASH_SR_QW
#endif

extern void HardFault_Handler();
uint32_t bootloader_SP = 0;

static const char *gc_help_msg =
  u8"\n\r┌────────────┬──────────────┬────────────┬────────────┬────────────┬─────────────┐"
  u8"\n\r│ 1:BootSRAM │ 2:UsageFault │ 3:BusFault │ 4:MemFault │ 5:assert() │ 6:BootFlash │"
  u8"\n\r└────────────┴──────────────┴────────────┴────────────┴────────────┴─────────────┘"
  u8"\n\r Выбор [1-6] > ";

static void do_BootSRAM();
static void do_UsageFault();
static void do_MemFault();
static void do_BusFault();
static void do_Assert();
static void do_BootFlash();

typedef void (*handler_func_t)();

handler_func_t handlers[NUM_COMMANDS] = {do_BootSRAM, do_UsageFault, do_BusFault,
                                         do_MemFault, do_Assert, do_BootFlash};

uint8_t read_handler_index() {
  while (vterm_keypressed() != 0)
    ;
  char str[2];
  int sz = vterm_gets(str, sizeof(str), 1);
  if (sz < 1)
    return UINT8_MAX;
  return str[0] >= '1' ? str[0] - '1' : UINT8_MAX;
}

void enable_fault_handlers() {
  // Включить генерацию исключений для UsageFault; cм. PM0253, п. 4.3.7 на стр. 200
  // SCB->CCR ....
  SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk;
  SCB->CCR |= SCB_CCR_UNALIGN_TRP_Msk;

  // Разрешить генерацию исключений; см. PM0253, п. 4.3.9 на
  // стр. 204 SCB->SHCSR ...
  SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk;
  SCB->SHCSR |= SCB_SHCSR_BUSFAULTENA_Msk;
  SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk;
}

int main() {
  vterm_init(115200);
  enable_fault_handlers();
  test_flash_write();
  for(;;) {
    delay(100);
  }
  return 0;
}

/***************************** Обработчики команд ************************************/
void do_BootSRAM() {

  printf("\nJumping to SRAM app at %08lx....\n",APP_SRAM_OFFSET);

  // 1) Определить ТВП (таблица векторов прерываний) приложения, адреса начала стека и точки входа приложения
  const uint32_t* app_IV = (uint32_t*)(APP_SRAM_OFFSET);
  uint32_t app_end_stack = (*((uint32_t *)(app_IV)));
  void* app_entry = (void *)(*((uint32_t *)(APP_SRAM_OFFSET + 4)));

  // Доп.1.) Признак того, что был запуск приложения bootloader_SP != 0
  bootloader_SP = __get_MSP();

  // 2) Отключить все прерывания
  __disable_irq();

  // 3) заменить текущий адрес стека на начальный адрес стека приложения
  __set_MSP(app_end_stack);

  // 4) задать новый адрес таблицы векторов прерываний
  SCB->VTOR = app_IV;

  // Доп.2) Заменили обработчика HardFault в ТВП на собственный
  NVIC_SetVector(HardFault_IRQn, (uint32_t)HardFault_Handler); 

  // Инвалидация кеша инстуркций у ядра Cortex-M7
  SCB_InvalidateICache();

  // 5) Безусловный переход на точку входу
  __ASM volatile("bx %0" ::"r"(app_entry));
}

void do_UsageFault() {
  // Отслеживаемые ошибки задаются в SCB->UFSR (PM0253.rev5 стр. 209 )
  // Например, деление на ноль, Доступ к невыровненным данным
  volatile int op1 = 55;
  volatile int op2 = 0;
  volatile int res = op1 / op2;
  (void)res; 
}


void do_MemFault() {
  // Нарушнеие аттрибутов памяти
  // например, попытка выполнения кода из области памяти для переферийных устройств
  void *ptr = (void *)0x40000000;
  goto *ptr;
}

void do_BusFault() {
  // Ошибка доступа к памяти по шине
  // Например, попытка чтения из отсутствующей внешней памяти (0х60000000)
  volatile uint32_t *invalid_address = (uint32_t *)0x60000000;
  volatile uint32_t value = *invalid_address;
  (void)value;
}

void do_Assert() { assert(!"Assertion example"); }

// void do_User() { puts(u8"\r\nВнезапно выпал снег\n"); }

void do_BootFlash() { 
  printf("Flash application found. Starting\r\n", APP_FLASH_OFFSET);

  printf("\nJumping to Flash app at %08lx....\n",APP_FLASH_OFFSET);

  // 1) Определить ТВП (таблица векторов прерываний) приложения, адреса начала стека и точки входа приложения
  const uint32_t* app_IV = (uint32_t*)(APP_FLASH_OFFSET);
  uint32_t app_end_stack = (*((uint32_t *)(app_IV)));
  void* app_entry = (void *)(*((uint32_t *)(APP_FLASH_OFFSET + 4)));

  // Доп.1.) Признак того, что был запуск приложения bootloader_SP != 0
  bootloader_SP = __get_MSP();

  // 2) Отключить все прерывания
  __disable_irq();

  // 3) заменить текущий адрес стека на начальный адрес стека приложения
  __set_MSP(app_end_stack);

  // 4) задать новый адрес таблицы векторов прерываний
  SCB->VTOR = app_IV;

  // Доп.2) Заменили обработчика HardFault в ТВП на собственный
  NVIC_SetVector(HardFault_IRQn, (uint32_t)HardFault_Handler); 

  // Инвалидация кеша инстуркций у ядра Cortex-M7
  SCB_InvalidateICache();

  // 5) Безусловный переход на точку входу
  __ASM volatile("bx %0" ::"r"(app_entry));
}


// Second task
__attribute__((optimize("-O0")))
static void delay(int ms) {
  volatile int counter = SystemCoreClock / 1000 / 6 * ms;
  while (counter > 0) counter -= 1;
}

static int check_sram_app(void) {
  uint32_t *app = (uint32_t *)APP_SRAM_OFFSET; 
  uint32_t sp = app[0]; 
  uint32_t entry = app[1];

  printf("Do check sram app at 0x%08lx: stack pointer=0x%08lx, entry=0x%08lx\r\n", APP_SRAM_OFFSET, sp, entry);
  
  // В диапазоне sram
  if (sp > 0x24080000 || sp < 0x24000000) {
    printf("Invalid sp\r\n");
    return 0;
  }
  // В диапазоне sram
  if (entry > 0x24080000 || entry < 0x24000000) {
    printf("Invalid entry\r\n");
    return 0;
  }
  
  return 1;
}

static int check_flash_app(void) {
  uint32_t *app = (uint32_t *)APP_FLASH_OFFSET; 
  uint32_t sp = app[0]; 
  uint32_t entry = app[1];
  
  printf("Do check flash app at 0x%08lx: stack pointer=0x%08lx, Entry=0x%08lx\r\n", APP_FLASH_OFFSET, sp, entry);
    
  // В диапазоне sram
  if (sp > 0x24080000 || sp < 0x24000000) {
    printf("Invalid sp\r\n");
    return 0;
  }
  // В диапазоне flash
  if (entry < APP_FLASH_OFFSET || entry >= 0x08100000) {
    printf("Invalid entry\r\n");
    return 0;
  }
  return 1;
}

void flash_write_bank2(uint32_t addr, uint32_t* data) {
    // Отключаем прерывания на время записи
    __disable_irq();
    
    // Разблокируем
    if (FLASH->CR2 & FLASH_CR_LOCK) {
        FLASH->KEYR2 = 0x45670123;
        FLASH->KEYR2 = 0xCDEF89AB;
    }
    
    // Очищаем флаги ошибок
    FLASH->SR2 |= (FLASH_SR_WRPERR | FLASH_SR_PGSERR | FLASH_SR_STRBERR);
    
    FLASH->CR2 |= FLASH_CR_PG2;
    
    for (int i = 0; i < 8; i++) {
        ((__IO uint32_t*)addr)[i] = data[i];
        __DSB();  // Data synchronization barrier
    }
    
    // Ждём с таймаутом
    volatile uint32_t timeout = 100000;
    while ((FLASH->SR2 & FLASH_SR_QW2) && timeout--) {
        __NOP();
    }
    
    if (timeout == 0) {
        printf("Flash timeout!\r\n");
    }
    
    FLASH->CR2 &= ~FLASH_CR_PG2;
    
    __enable_irq();
}

void test_flash_write(void) {
    __disable_irq();
    // // Проверяем ID устройства и размер flash
    // uint32_t flash_size_reg = *((uint32_t*)0x1FF1E880);
    // uint16_t device_id = (DBGMCU->IDCODE >> 16) & 0xFFF;
    
    // printf("Device ID: 0x%03X\r\n", device_id);
    // printf("Flash size register: %d KB\r\n", flash_size_reg);
    
    // // Для STM32H7:
    // // 0x483 - STM32H743/753 (2MB flash)
    // // 0x450 - STM32H742 (1MB flash)
    // // 0x451 - STM32H743/753 (2MB)
    
    // if (flash_size_reg <= 1024) {
    //     printf("ERROR: Flash size <= 1MB, bank 2 NOT present!\r\n");
    //     return;
    // }
    
    // // Пробуем просто прочитать по адресу bank 2
    // printf("Trying to read from 0x08100000...\r\n");
    // uint32_t test_read = *((uint32_t*)0x08100000);
    // printf("Read value: 0x%08X\r\n", test_read);
    
    // // Если дошли сюда - bank 2 доступен
    // printf("Bank 2 is present! Writing data...\r\n");
    
    // Подготавливаем данные для записи (32 байта)
    volatile uint32_t test_data[8] __attribute__((aligned(32)));
    test_data[0] = 0xDEADBEEF;
    test_data[1] = 0xCAFEBABE;
    test_data[2] = 0x12345678;
    test_data[3] = 0x90ABCDEF;
    test_data[4] = 0x11112222;
    test_data[5] = 0x33334444;
    test_data[6] = 0x55556666;
    test_data[7] = 0x77778888;
    
    // printf("Data array address: 0x%p\r\n", (void*)test_data);
    
    // Адрес для записи в bank 2 (начало bank 2)
    uint32_t flash_addr = 0x08100000;
    printf("Target flash address: 0x%08X\r\n", flash_addr);
    
    // // 1. Разблокируем CR2
    // if (FLASH->CR2 & FLASH_CR_LOCK) {
    //     printf("Unlocking FLASH_CR2...\r\n");
    //     FLASH->KEYR2 = 0x45670123;
    //     FLASH->KEYR2 = 0xCDEF89AB;
    // }
    
    // // 2. Очищаем флаги ошибок
    // FLASH->SR2 |= (FLASH_SR_WRPERR | FLASH_SR_PGSERR | FLASH_SR_STRBERR);
    
    // // 3. Стираем сектор 0 в bank 2
    // printf("Erasing sector 0...\r\n");
    // FLASH->CR2 |= FLASH_CR_SER;           // Включаем стирание сектора
    // FLASH->CR2 &= ~(0xF << 3);            // Очищаем номер сектора
    // FLASH->CR2 |= (0 << 3);               // Выбираем сектор 0
    // FLASH->CR2 |= FLASH_CR_START;         // Запускаем стирание
    
    // // Ждем завершения стирания
    // volatile uint32_t timeout = 100000;
    // while ((FLASH->SR2 & FLASH_SR_QW2) && timeout--) {
    //     __NOP();
    // }
    
    // if (timeout == 0) {
    //     printf("Erase timeout!\r\n");
    //     return;
    // }
    // printf("Erase complete\r\n");
    
    // // 4. Включаем режим программирования
    // printf("Writing data...\r\n");
    // FLASH->CR2 |= FLASH_CR_PG2;
    
    // // 5. Пишем 8 слов (32 байта)
    // for (int i = 0; i < 8; i++) {
    //     ((__IO uint32_t*)flash_addr)[i] = test_data[i];
    //     __DSB();  // Data synchronization barrier
    // }
    
    // // 6. Ждем завершения записи
    // timeout = 100000;
    // while ((FLASH->SR2 & FLASH_SR_QW2) && timeout--) {
    //     __NOP();
    // }
    
    // if (timeout == 0) {
    //     printf("Write timeout!\r\n");
    //     FLASH->CR2 &= ~FLASH_CR_PG2;
    //     return;
    // }
    
    // // 7. Выключаем режим программирования
    // FLASH->CR2 &= ~FLASH_CR_PG2;
    
    // 8. Проверяем записанные данные
    printf("Verifying data...\r\n");
    int verify_ok = 1;
    for (int i = 0; i < 8; i++) {
        uint32_t read_value = ((uint32_t*)flash_addr)[i];
        printf("  [%d] wrote: 0x%08X, read: 0x%08X\r\n", i, test_data[i], read_value);
        if (read_value != test_data[i]) {
            verify_ok = 0;
        }
    }
    
    if (verify_ok) {
        printf("SUCCESS: Flash write verified!\r\n");
    } else {
        printf("ERROR: Verification failed!\r\n");
    }
}
