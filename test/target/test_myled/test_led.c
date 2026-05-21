#include <stm32h7xx.h>
#include <led.h>
#include <unity.h>

void setUp() { led_enable(led_all); }
void tearDown() { led_disable(led_all); }

void test_led_green_toggle() {
    uint32_t state[3] = {0};
    state[0] = GPIOB->ODR & GPIO_ODR_OD0;
    led_toggle(led_green);
    state[1] = GPIOB->ODR & GPIO_ODR_OD0;
    TEST_ASSERT_NOT_EQUAL_UINT32_MESSAGE(state[0], state[1],
        "green led not changed after toggle once");
    led_toggle(led_green);
    state[2] = GPIOB->ODR & GPIO_ODR_OD0;
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(state[0], state[2],
        "green led changed after toggle twice");
}

void test_led_yellow_on_off() {
    led_off(led_yellow);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, GPIOE->ODR & GPIO_ODR_OD1,
        "yellow led should be off");
    led_on(led_yellow);
    TEST_ASSERT_NOT_EQUAL_UINT32_MESSAGE(0, GPIOE->ODR & GPIO_ODR_OD1,
        "yellow led should be on");
    led_off(led_yellow);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, GPIOE->ODR & GPIO_ODR_OD1,
        "yellow led should be off again");
}

void test_led_red_toggle() {
    uint32_t state[3] = {0};
    state[0] = GPIOB->ODR & GPIO_ODR_OD14;
    led_toggle(led_red);
    state[1] = GPIOB->ODR & GPIO_ODR_OD14;
    TEST_ASSERT_NOT_EQUAL_UINT32_MESSAGE(state[0], state[1],
        "red led not changed after toggle once");
    led_toggle(led_red);
    state[2] = GPIOB->ODR & GPIO_ODR_OD14;
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(state[0], state[2],
        "red led changed after toggle twice");
}

void test_led_all_on() {
    led_off(led_all);
    led_on(led_all);
    TEST_ASSERT_NOT_EQUAL_UINT32_MESSAGE(0, GPIOB->ODR & GPIO_ODR_OD0,
        "green should be on");
    TEST_ASSERT_NOT_EQUAL_UINT32_MESSAGE(0, GPIOE->ODR & GPIO_ODR_OD1,
        "yellow should be on");
    TEST_ASSERT_NOT_EQUAL_UINT32_MESSAGE(0, GPIOB->ODR & GPIO_ODR_OD14,
        "red should be on");
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_led_green_toggle);
    RUN_TEST(test_led_yellow_on_off);
    RUN_TEST(test_led_red_toggle);
    RUN_TEST(test_led_all_on);
    return UNITY_END();
}
