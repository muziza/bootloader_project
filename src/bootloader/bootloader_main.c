#include <stdint.h>
#include "stm32h7xx.h"

#define APP_FLASH_BASE       0x08020000UL
#define APP_FLASH_END        0x08100000UL
#define APP_FLASH_SIZE       (APP_FLASH_END - APP_FLASH_BASE)
#define APP_FIRST_SECTOR     1UL
#define APP_LAST_SECTOR      7UL

#define SRAM_BASE_ADDR       0x20000000UL
#define SRAM_END_ADDR        0x20020000UL
#define AXI_SRAM_BASE_ADDR   0x24000000UL
#define AXI_SRAM_END_ADDR    0x24080000UL

#define UART_BAUDRATE        115200UL
#define FLASH_WORD_SIZE      32UL

#define CMD_ERASE            0x01U
#define CMD_WRITE            0x02U
#define CMD_READ             0x03U
#define CMD_JUMP             0x04U

#define ACK_OK               'O'
#define ACK_ERROR            'E'
#define ACK_UNKNOWN          '?'

#define GPIO_MODE_INPUT      0x0UL
#define GPIO_MODE_OUTPUT     0x1UL
#define GPIO_MODE_AF         0x2UL

#define LED_PORT             GPIOE
#define LED_PIN              1UL
#define BUTTON_PORT          GPIOC
#define BUTTON_PIN           13UL

#define FLASH_KEY1           0x45670123UL
#define FLASH_KEY2           0xCDEF89ABUL
#define BL_FLASH_CR_LOCK     (1UL << 0)
#define BL_FLASH_CR_PG       (1UL << 1)
#define BL_FLASH_CR_SER      (1UL << 2)
#define BL_FLASH_CR_START    (1UL << 7)
#define BL_FLASH_CR_SNB_Pos  8UL
#define BL_FLASH_CR_SNB_Msk  (7UL << BL_FLASH_CR_SNB_Pos)
#define BL_FLASH_SR_BSY      (1UL << 0)
#define BL_FLASH_SR_QW       (1UL << 2)
#define BL_FLASH_SR_EOP      (1UL << 16)
#define BL_FLASH_ERROR_FLAGS ((1UL << 17) | (1UL << 18) | (1UL << 19) | \
                              (1UL << 21) | (1UL << 22) | (1UL << 23) | \
                              (1UL << 24) | (1UL << 25) | (1UL << 26))
#define BL_FLASH_CLEAR_FLAGS (BL_FLASH_SR_EOP | BL_FLASH_ERROR_FLAGS)

#ifndef USART_ISR_RXNE_RXFNE
#define USART_ISR_RXNE_RXFNE USART_ISR_RXNE
#endif

#ifndef USART_ISR_TXE_TXFNF
#define USART_ISR_TXE_TXFNF USART_ISR_TXE
#endif

static void gpio_init(void);
static void uart3_init(void);
static void bootloader_mode(void);
static void process_command(uint8_t cmd);
static void cmd_erase(void);
static void cmd_write(void);
static void cmd_read(void);
static void cmd_jump(void);
static int flash_erase_application(void);
static int flash_write_block(uint32_t address, const uint8_t data[FLASH_WORD_SIZE]);
static int app_is_valid(void);
static void jump_to_application(void);

static void delay(volatile uint32_t count)
{
    while (count-- != 0U) {
        __NOP();
    }
}

static void gpio_set_mode(GPIO_TypeDef *port, uint32_t pin, uint32_t mode)
{
    port->MODER = (port->MODER & ~(3UL << (pin * 2UL))) | (mode << (pin * 2UL));
}

static void gpio_init(void)
{
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOCEN | RCC_AHB4ENR_GPIODEN | RCC_AHB4ENR_GPIOEEN;
    (void)RCC->AHB4ENR;

    gpio_set_mode(LED_PORT, LED_PIN, GPIO_MODE_OUTPUT);

    gpio_set_mode(BUTTON_PORT, BUTTON_PIN, GPIO_MODE_INPUT);
    BUTTON_PORT->PUPDR = (BUTTON_PORT->PUPDR & ~(3UL << (BUTTON_PIN * 2UL))) |
                         (1UL << (BUTTON_PIN * 2UL));

    gpio_set_mode(GPIOD, 8UL, GPIO_MODE_AF);
    gpio_set_mode(GPIOD, 9UL, GPIO_MODE_AF);
    GPIOD->AFR[1] = (GPIOD->AFR[1] & ~((0xFUL << 0) | (0xFUL << 4))) |
                   (7UL << 0) | (7UL << 4);
}

static uint8_t button_pressed(void)
{
    return ((BUTTON_PORT->IDR & (1UL << BUTTON_PIN)) == 0U) ? 1U : 0U;
}

static void led_toggle(void)
{
    LED_PORT->ODR ^= (1UL << LED_PIN);
}

static void uart3_init(void)
{
    RCC->APB1LENR |= RCC_APB1LENR_USART3EN;
    (void)RCC->APB1LENR;

    USART3->CR1 = 0U;
    USART3->BRR = SystemCoreClock / UART_BAUDRATE;
    USART3->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

static uint8_t uart_rx(void)
{
    while ((USART3->ISR & USART_ISR_RXNE_RXFNE) == 0U) {
    }

    return (uint8_t)USART3->RDR;
}

static uint8_t uart_try_rx(uint8_t *byte)
{
    if ((USART3->ISR & USART_ISR_RXNE_RXFNE) == 0U) {
        return 0U;
    }

    *byte = (uint8_t)USART3->RDR;
    return 1U;
}

static void uart_tx(uint8_t byte)
{
    while ((USART3->ISR & USART_ISR_TXE_TXFNF) == 0U) {
    }

    USART3->TDR = byte;
}

static void uart_write(const uint8_t *data, uint32_t length)
{
    for (uint32_t i = 0; i < length; ++i) {
        uart_tx(data[i]);
    }
}

static void uart_puts(const char *text)
{
    while (*text != '\0') {
        uart_tx((uint8_t)*text++);
    }
}

static uint32_t uart_rx_u32(void)
{
    uint32_t value = uart_rx();
    value |= (uint32_t)uart_rx() << 8;
    value |= (uint32_t)uart_rx() << 16;
    value |= (uint32_t)uart_rx() << 24;
    return value;
}

static void flash_wait(void)
{
    while ((FLASH->SR1 & (BL_FLASH_SR_BSY | BL_FLASH_SR_QW)) != 0U) {
    }
}

static void flash_unlock(void)
{
    if ((FLASH->CR1 & BL_FLASH_CR_LOCK) != 0U) {
        FLASH->KEYR1 = FLASH_KEY1;
        FLASH->KEYR1 = FLASH_KEY2;
    }
}

static void flash_lock(void)
{
    FLASH->CR1 |= BL_FLASH_CR_LOCK;
}

static void flash_clear_errors(void)
{
    FLASH->CCR1 = BL_FLASH_CLEAR_FLAGS;
}

static int flash_erase_sector(uint32_t sector)
{
    if (sector > APP_LAST_SECTOR) {
        return -1;
    }

    flash_wait();
    flash_clear_errors();
    FLASH->CR1 = (FLASH->CR1 & ~BL_FLASH_CR_SNB_Msk) |
                 BL_FLASH_CR_SER |
                 (sector << BL_FLASH_CR_SNB_Pos);
    FLASH->CR1 |= BL_FLASH_CR_START;
    flash_wait();
    FLASH->CR1 &= ~BL_FLASH_CR_SER;

    return ((FLASH->SR1 & BL_FLASH_ERROR_FLAGS) == 0U) ? 0 : -1;
}

static int flash_erase_application(void)
{
    int status = 0;

    __disable_irq();
    flash_unlock();

    for (uint32_t sector = APP_FIRST_SECTOR; sector <= APP_LAST_SECTOR; ++sector) {
        if (flash_erase_sector(sector) != 0) {
            status = -1;
            break;
        }
    }

    flash_lock();
    __enable_irq();

    SCB_InvalidateICache();
    return status;
}

static int flash_write_block(uint32_t address, const uint8_t data[FLASH_WORD_SIZE])
{
    if ((address < APP_FLASH_BASE) ||
        ((address + FLASH_WORD_SIZE) > APP_FLASH_END) ||
        ((address % FLASH_WORD_SIZE) != 0U)) {
        return -1;
    }

    flash_wait();
    flash_clear_errors();
    FLASH->CR1 |= BL_FLASH_CR_PG;

    for (uint32_t i = 0; i < FLASH_WORD_SIZE; i += 4U) {
        *(volatile uint32_t *)(address + i) =
            ((uint32_t)data[i]) |
            ((uint32_t)data[i + 1U] << 8) |
            ((uint32_t)data[i + 2U] << 16) |
            ((uint32_t)data[i + 3U] << 24);
        __DSB();
    }

    flash_wait();
    FLASH->CR1 &= ~BL_FLASH_CR_PG;

    for (uint32_t i = 0; i < FLASH_WORD_SIZE; ++i) {
        if (*(volatile uint8_t *)(address + i) != data[i]) {
            return -1;
        }
    }

    return ((FLASH->SR1 & BL_FLASH_ERROR_FLAGS) == 0U) ? 0 : -1;
}

static uint8_t stack_in_ram(uint32_t stack)
{
    if ((stack >= SRAM_BASE_ADDR) && (stack <= SRAM_END_ADDR)) {
        return 1U;
    }

    if ((stack >= AXI_SRAM_BASE_ADDR) && (stack <= AXI_SRAM_END_ADDR)) {
        return 1U;
    }

    return 0U;
}

static int app_is_valid(void)
{
    uint32_t stack = *(volatile uint32_t *)APP_FLASH_BASE;
    uint32_t entry = *(volatile uint32_t *)(APP_FLASH_BASE + 4U);

    if ((stack == 0xFFFFFFFFUL) || (entry == 0xFFFFFFFFUL)) {
        return 0;
    }

    if (stack_in_ram(stack) == 0U) {
        return 0;
    }

    if ((entry < APP_FLASH_BASE) || (entry >= APP_FLASH_END) || ((entry & 1UL) == 0U)) {
        return 0;
    }

    return 1;
}

static void jump_to_application(void)
{
    uint32_t stack = *(volatile uint32_t *)APP_FLASH_BASE;
    uint32_t entry = *(volatile uint32_t *)(APP_FLASH_BASE + 4U);
    void (*reset_handler)(void) = (void (*)(void))entry;

    __disable_irq();

    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL = 0U;

    for (uint32_t i = 0; i < 8U; ++i) {
        NVIC->ICER[i] = 0xFFFFFFFFUL;
        NVIC->ICPR[i] = 0xFFFFFFFFUL;
    }

    SCB->VTOR = APP_FLASH_BASE;
    SCB_CleanInvalidateDCache();
    SCB_InvalidateICache();
    __set_MSP(stack);
    __DSB();
    __ISB();

    reset_handler();
}

static void cmd_erase(void)
{
    uart_tx((flash_erase_application() == 0) ? ACK_OK : ACK_ERROR);
}

static void cmd_write(void)
{
    uint32_t offset = uart_rx_u32();
    uint32_t length = uart_rx_u32();
    uint32_t address = APP_FLASH_BASE + offset;
    uint8_t block[FLASH_WORD_SIZE];
    int status = 0;

    if ((offset >= APP_FLASH_SIZE) || (length > (APP_FLASH_SIZE - offset))) {
        status = -1;
    }

    __disable_irq();
    flash_unlock();

    for (uint32_t written = 0; written < length; written += FLASH_WORD_SIZE) {
        for (uint32_t i = 0; i < FLASH_WORD_SIZE; ++i) {
            if ((written + i) < length) {
                block[i] = uart_rx();
            } else {
                block[i] = 0xFFU;
            }
        }

        if ((status == 0) && (flash_write_block(address + written, block) != 0)) {
            status = -1;
        }
    }

    flash_lock();
    __enable_irq();
    SCB_InvalidateICache();

    uart_tx((status == 0) ? ACK_OK : ACK_ERROR);
}

static void cmd_read(void)
{
    uint32_t offset = uart_rx_u32();
    uint32_t length = uart_rx_u32();

    if ((offset >= APP_FLASH_SIZE) || (length > (APP_FLASH_SIZE - offset))) {
        uart_tx(ACK_ERROR);
        return;
    }

    uart_tx(ACK_OK);
    uart_write((const uint8_t *)(APP_FLASH_BASE + offset), length);
}

static void cmd_jump(void)
{
    if (app_is_valid() == 0) {
        uart_tx(ACK_ERROR);
        return;
    }

    uart_tx(ACK_OK);
    while ((USART3->ISR & USART_ISR_TC) == 0U) {
    }
    jump_to_application();
}

static void process_command(uint8_t cmd)
{
    switch (cmd) {
    case CMD_ERASE:
        cmd_erase();
        break;
    case CMD_WRITE:
        cmd_write();
        break;
    case CMD_READ:
        cmd_read();
        break;
    case CMD_JUMP:
        cmd_jump();
        break;
    default:
        uart_tx(ACK_UNKNOWN);
        break;
    }
}

static void bootloader_mode(void)
{
    uint8_t cmd;

    uart_puts("BOOTLOADER READY\r\n");

    while (1) {
        led_toggle();
        for (uint32_t i = 0; i < 60U; ++i) {
            if (uart_try_rx(&cmd) != 0U) {
                process_command(cmd);
            }
            delay(50000U);
        }
    }
}

int main(void)
{
    gpio_init();
    uart3_init();

    if ((button_pressed() == 0U) && (app_is_valid() != 0)) {
        jump_to_application();
    }

    bootloader_mode();
}
