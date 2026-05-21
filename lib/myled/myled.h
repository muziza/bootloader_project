#pragma once
// добавляем свой макрос
#define TOGGLE_BIT(REG, BIT)   ((REG) ^= (BIT))

void myled_enable();
void myled_toggle();
void myled_disable();
