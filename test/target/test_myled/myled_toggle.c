#include <stm32h7xx.h>
#include <myled.h>
#include <unity.h>

void setUp() {
    myled_enable();
}

void tearDown() {
    myled_disable();
}

void test_myled_toggle() {
  uint32_t state[3] = {0};
  state[0] = GPIOE->ODR & GPIO_ODR_OD1;
  myled_toggle();
  state[1] = GPIOE->ODR & GPIO_ODR_OD1;
  TEST_ASSERT_NOT_EQUAL_UINT32_MESSAGE(state[0], state[1],
                            "led state not changed after toggle once");
  myled_toggle();
  state[2] = GPIOE->ODR & GPIO_ODR_OD1;
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(state[0], state[2],
                            "led state changed after toggle twice");
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_myled_toggle);
  return UNITY_END();
}
