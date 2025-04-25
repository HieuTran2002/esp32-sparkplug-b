#ifndef ESP_LOG_EXPANSION_H
#define ESP_LOG_EXPANSION_H

// Normal colors
#define ESP_LOGI_Black(tag, fmt, ...)   ESP_LOGI(tag, "\033[30m" fmt "\033[0m", ##__VA_ARGS__)
#define ESP_LOGI_Red(tag, fmt, ...)     ESP_LOGI(tag, "\033[31m" fmt "\033[0m", ##__VA_ARGS__)
#define ESP_LOGI_Green(tag, fmt, ...)   ESP_LOGI(tag, "\033[32m" fmt "\033[0m", ##__VA_ARGS__)
#define ESP_LOGI_Yellow(tag, fmt, ...)  ESP_LOGI(tag, "\033[33m" fmt "\033[0m", ##__VA_ARGS__)
#define ESP_LOGI_Blue(tag, fmt, ...)    ESP_LOGI(tag, "\033[34m" fmt "\033[0m", ##__VA_ARGS__)
#define ESP_LOGI_Magenta(tag, fmt, ...) ESP_LOGI(tag, "\033[35m" fmt "\033[0m", ##__VA_ARGS__)
#define ESP_LOGI_Cyan(tag, fmt, ...)    ESP_LOGI(tag, "\033[36m" fmt "\033[0m", ##__VA_ARGS__)
#define ESP_LOGI_White(tag, fmt, ...)   ESP_LOGI(tag, "\033[37m" fmt "\033[0m", ##__VA_ARGS__)

// Bold/bright variants
#define ESP_LOGI_BrightRed(tag, fmt, ...)     ESP_LOGI(tag, "\033[1;31m" fmt "\033[0m", ##__VA_ARGS__)
#define ESP_LOGI_BrightGreen(tag, fmt, ...)   ESP_LOGI(tag, "\033[1;32m" fmt "\033[0m", ##__VA_ARGS__)
#define ESP_LOGI_BrightYellow(tag, fmt, ...)  ESP_LOGI(tag, "\033[1;33m" fmt "\033[0m", ##__VA_ARGS__)
#define ESP_LOGI_BrightBlue(tag, fmt, ...)    ESP_LOGI(tag, "\033[1;34m" fmt "\033[0m", ##__VA_ARGS__)
#define ESP_LOGI_BrightMagenta(tag, fmt, ...) ESP_LOGI(tag, "\033[1;35m" fmt "\033[0m", ##__VA_ARGS__)
#define ESP_LOGI_BrightCyan(tag, fmt, ...)    ESP_LOGI(tag, "\033[1;36m" fmt "\033[0m", ##__VA_ARGS__)
#define ESP_LOGI_BrightWhite(tag, fmt, ...)   ESP_LOGI(tag, "\033[1;37m" fmt "\033[0m", ##__VA_ARGS__)



#endif /* end of include guard: ESP_LOG_EXPANSION_H */
