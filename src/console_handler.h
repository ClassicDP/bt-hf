#ifndef CONSOLE_HANDLER_H
#define CONSOLE_HANDLER_H

/**
 * @brief Инициализация консольного обработчика
 */
void console_handler_init(void);

/**
 * @brief Обработка команды из консоли
 * @param command Команда для обработки
 */
void console_handler_process_command(const char *command);

#endif // CONSOLE_HANDLER_H
