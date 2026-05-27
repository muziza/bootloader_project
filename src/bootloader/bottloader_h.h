#ifndef  __STM32H745xx_H__
#define  __STM32H745xx_H__
#include <stdint.h>
#include <stdio.h>


#define GPIOB_BASE (0x58020400U)
#define GPIOC_BASE (0x58020800U)
#define GPIOE_BASE (0x58021000U)

typedef struct {
  volatile uint32_t MODER;
  volatile uint32_t OTYPER;
  volatile uint32_t OSPEEDR; 
  volatile uint32_t PUPDR;
  volatile uint32_t IDR;
  volatile uint32_t ODR;
  volatile uint32_t BSRR;
  volatile uint32_t LCKR;
  volatile uint32_t AFRL; 
  volatile uint32_t AFRH;
} GPIO_TypeDef;

#define GPIOB ((GPIO_TypeDef*) GPIOB_BASE)
#define GPIOC ((GPIO_TypeDef*) GPIOC_BASE)
#define GPIOE ((GPIO_TypeDef*) GPIOE_BASE)

#define GPIO_MODE_INPUT      (0x0) 
#define GPIO_MODE_OUTPUT     (0x1) 
#define GPIO_MODE_ALTERNATE  (0x2) 
#define GPIO_MODE_ANALOG     (0x3)  


#define RCC_BASE (0x58024400U)

typedef struct {
  volatile uint32_t CR;
  volatile uint32_t HSICFGR;
  volatile uint32_t CRRCR;
  volatile uint32_t CSICFGR;
  volatile uint32_t CFGR;
  uint32_t RESERVED1;
  volatile uint32_t D1CFGR;
  volatile uint32_t D2CFGR;
  volatile uint32_t D3CFGR;
  uint32_t RESERVED2;
  volatile uint32_t PLLCKSELR;
  volatile uint32_t PLLCFGR;
  volatile uint32_t PLL1DIVR;
  volatile uint32_t PLL1FRACR;
  volatile uint32_t PLL2DIVR;
  volatile uint32_t PLL2FRACR;
  volatile uint32_t PLL3DIVR;
  volatile uint32_t PLL3FRACR;
  uint32_t RESERVED3;
  volatile uint32_t D1CCIPR;
  volatile uint32_t D2CCIP1R;
  volatile uint32_t D2CCIP2R;
  volatile uint32_t D3CCIPR;
  uint32_t RESERVED4;
  volatile uint32_t CIER;
  volatile uint32_t CIFR;
  volatile uint32_t CICR;
  uint32_t RESERVED5;
  volatile uint32_t BDCR;
  volatile uint32_t CSR;
  uint32_t RESERVED6;
  volatile uint32_t AHB3RSTR;
  volatile uint32_t AHB1RSTR;
  volatile uint32_t AHB2RSTR;
  volatile uint32_t AHB4RSTR;
  volatile uint32_t APB3RSTR;
  volatile uint32_t APB1LRSTR;
  volatile uint32_t APB1HRSTR;
  volatile uint32_t APB2RSTR;
  volatile uint32_t APB4RSTR;
  volatile uint32_t GCR;
  uint32_t RESERVED8;
  volatile uint32_t D3AMR;
  uint32_t RESERVED9[9];
  volatile uint32_t RSR;
  volatile uint32_t AHB3ENR;
  volatile uint32_t AHB1ENR;
  volatile uint32_t AHB2ENR;
  volatile uint32_t AHB4ENR;
  volatile uint32_t APB3ENR;
  volatile uint32_t APB1LENR;
  volatile uint32_t APB1HENR;
  volatile uint32_t APB2ENR;
  volatile uint32_t APB4ENR;
  uint32_t RESERVED10;
  volatile uint32_t AHB3LPENR;
  volatile uint32_t AHB1LPENR;
  volatile uint32_t AHB2LPENR;
  volatile uint32_t AHB4LPENR;
  volatile uint32_t APB3LPENR;
  volatile uint32_t APB1LLPENR;
  volatile uint32_t APB1HLPENR;
  volatile uint32_t APB2LPENR;
  volatile uint32_t APB4LPENR;
  uint32_t RESERVED11[4];
} RCC_TypeDef;

#define RCC ((RCC_TypeDef*) RCC_BASE)

#define RCC_AHB4ENR_GPIOBEN (0x1UL << 1U)
#define RCC_AHB4ENR_GPIOCEN (0x1UL << 2U)
#define RCC_AHB4ENR_GPIOEEN (0x1UL << 4U)


#define LED_RED_PIN    (14U)
#define LED_GREEN_PIN  (0U )
#define LED_YELLOW_PIN (1U )
#define BUTTON_PIN     (13U)

#define LED_RED_PORT    (GPIOB)
#define LED_GREEN_PORT  (GPIOB)
#define LED_YELLOW_PORT (GPIOE)
#define BUTTON_PORT     (GPIOC)

#endif //  __STM32H745xx_H__