

#include "vterm.h"
#include "stm32h7xx.h"
#include <stdio.h>

#ifndef HSI_VALUE
#define HSI_VALUE 64000000U
#endif
#define BLOCK_ON(COND) do{}while(COND)

// ======================= Private variables and functions =============================
static int lib_init_flag = 0;

/**
 * @brief Инициализации UART3, подключенного к STLinkv3 CDC
 * @note  PD8(AF7)->USART3_TX, PD9(AF7)->USART3_RX; kernel сlock = HSI
 * @param baudrate скорость работы  
 * */
static void uart_init(int baudrate) {
  // Назначаем USART3 ядру CM7 и включаем тактирование
  SET_BIT(RCC_C1->APB1LENR, RCC_APB1LENR_USART3EN);
  // Выбираем HSI для Kernel clock
  MODIFY_REG(RCC->D2CCIP2R, RCC_D2CCIP2R_USART28SEL_0,
             RCC_D2CCIP2R_USART28SEL_0 | RCC_D2CCIP2R_USART28SEL_1);
  // Включаем тактирование GPIOD
  SET_BIT(RCC_C1->AHB4ENR, RCC_AHB4ENR_GPIODEN_Msk);
  // Режим альт. функции для PD8 и PD9
  MODIFY_REG(GPIOD->MODER, GPIO_MODER_MODE8_Msk | GPIO_MODER_MODE9_Msk,
             GPIO_MODER_MODE8_1 | GPIO_MODER_MODE9_1);
  // Назначем номер альт. функции PD8 и PD9, соотв. USART3
  MODIFY_REG(GPIOD->AFR[1], GPIO_AFRH_AFSEL8 | GPIO_AFRH_AFSEL9,
    GPIO_AFRH_AFSEL8_0 | GPIO_AFRH_AFSEL8_1 | GPIO_AFRH_AFSEL8_2 | 
    GPIO_AFRH_AFSEL9_0 | GPIO_AFRH_AFSEL9_1 | GPIO_AFRH_AFSEL9_2);
  // Режим "PushPull" для TX
  CLEAR_BIT(GPIOD->OTYPER, GPIO_OTYPER_OT8_Msk);
  // Без подтягивающих резисторов pull-up/pull-down на TX и RX
  MODIFY_REG(GPIOD->PUPDR, GPIO_PUPDR_PUPD8_Msk | GPIO_PUPDR_PUPD9_Msk, 0);
  // "Medium speed" для порта TX
  MODIFY_REG(GPIOD->OSPEEDR, GPIO_OSPEEDR_OSPEED8_Msk, GPIO_OSPEEDR_OSPEED8_0);
  // Настройка UART: 8-bit data, no parity, 1 stop bit, oversampling; enablingtx+rx
  MODIFY_REG(USART3->CR1, USART_CR1_TE | USART_CR1_RE, USART_CR1_TE | USART_CR1_RE);
  // Пределитель клока HSI/8 (8 МГц)
  MODIFY_REG(USART3->PRESC, USART_PRESC_PRESCALER, USART_PRESC_PRESCALER_2);
  // Конфигурация скорости приёмопередатчика
  USART3->BRR = HSI_VALUE / 8 / baudrate;
  // Отключаем детекцию и ошибки переполнения.
  USART3->CR3 |= USART_CR3_OVRDIS;
  // Включаем USART3
  USART3->CR1 |= USART_CR1_UE;

  BLOCK_ON((USART3->ISR & USART_ISR_TEACK_Msk) == 0);
  BLOCK_ON((USART3->ISR & USART_ISR_REACK_Msk) == 0);
}

/**
 * @brief Передача одного символа в UART3  */
static inline void uart_putch(char ch) {
  // ожидание готовности сдвигового регистра
  BLOCK_ON((USART3->ISR & USART_ISR_TXFE) != 0);
  USART3->TDR = ch;
}
/**
 * @brief Ожидание и приём одного символа
 */
static inline char uart_getch() {
  BLOCK_ON((USART3->ISR & USART_ISR_RXNE_RXFNE) == 0);
  return (USART3->RDR) & 0xFF;
}

// ======================= Public functions =============================

void vterm_init(int baudrate) {
  if (lib_init_flag == 0){
    uart_init(baudrate);
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    lib_init_flag = 1;
  }
}

unsigned char vterm_keypressed(void){
  if ((USART3->ISR & USART_ISR_RXNE_RXFNE) == 0)
    return 0;
  return (USART3->RDR) & 0xFF;
}

void vterm_write(char *ptr, int len) {
  if (!lib_init_flag)
    return;
  for (int i = 0; i < len; i++)
    uart_putch(ptr[i]);
  BLOCK_ON((USART3->ISR & USART_ISR_TC) == 0);
}

int vterm_gets(char *buf, int size, int echo) {
  if (!lib_init_flag || !buf || size == 0)
    return -1;
  int i = 0;
  while (i < size - 1) {
    char ch = uart_getch();
    if (echo)
      uart_putch(ch); // Echo to terminal
    if (ch == '\n' || ch == '\r') {
      break;
    }
    buf[i++] = ch;
  }
  buf[i] = '\0';
  return i;
}

// ======================= Newlib IO functions =============================

int _write(int file, char *ptr, int len) {
  if ((file == 1 || file == 2) && lib_init_flag) {
    vterm_write(ptr, len);
    return len;
  }
  return -1;
}

int _read(int file, char *ptr, int len) {
  if (lib_init_flag && len > 0 && file == 0) {
    int i = 0;
    // Блокировка на ожидании первого символа
    BLOCK_ON((USART3->ISR & USART_ISR_RXNE_RXFNE) == 0);
    ptr[i++] = (USART3->RDR) & 0xFF;
    while (i < len) {
      // Корткое ожидание следующего символа
      volatile int wait_timer = 1000;
      while ((USART3->ISR & USART_ISR_RXNE_RXFNE) == 0 && wait_timer-- > 0);
      if (wait_timer == 0)
        break;
      ptr[i++] = (USART3->RDR) & 0xFF;
    }
    return i;
  }
  return -1;
}
