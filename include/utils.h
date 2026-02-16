#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

char   *ft_strjoin(char const *s1, char const *s2);

// @brief logs the error message and exits with the exit status in 'status' argument
void   errorLogger(char *message, int status);

// @brief displays messages for debugging
void   debugLogger(char *message);

// @brief displays general info messages
void   infoLogger(char *message);

// @brief displays ping results
void   resultLogger(char *result);

// @brief returns current time in milliseconds
uint32_t get_milliseconds();

#endif
