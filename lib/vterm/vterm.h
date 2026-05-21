/** @brief Vterm library for Nucleo STM32H75ZI-Q
 *  Предназначена для ввода/вывода на хост-терминал через StlinkV3.
 *  @version 0.3 
 *  @author ssbdex@yandex.ru
 * */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 *@brief Инициализация семихостинга по StLinkV3 (UART3). */
void vterm_init(int baudrate);

/********* Функции вывода в терминал ********************/

/**
 * @brief Передача пакета символов длинной len */
void vterm_write(char *ptr, int len);

// Также доступна <stdio.h>: putchar()

// Также доступна <stdio.h>: puts()

// Также доступна <stdio.h>: printf()

/********* Функции ввода из терминала ********************/

/**
 * @brief Чтение строки из терминала в буфер
 * @note Чтение завершается при считывании символа перевода строки или возврата
 * каретки или при заполнении буфера. Данные в буфере всегда дополняются символом конца строки.
 * @param buf - буфер
 * @param size - размер буфера
 * @param echo - если не 0, то выводить все принимаемые символы 
 * @return длина считаной строки или -1 (ошибка)
 */
int vterm_gets(char* buf, int len, int echo);

// Также доступна <stdio.h>: getchar

// Также доступна <stdio.h>: getc

// Также доступна<stdio.h>: gets

/**
 * @brief Проверяет (без блокировки) не был ли получен символ
 * @return 0, если нет символа,  иначе - код полученного символа  */
unsigned char vterm_keypressed(void);

#ifdef __cplusplus
}
#endif
