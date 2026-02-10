#ifndef UTILS_H
# define UTILS_H

# include <stddef.h> // size_t is declared here

char   *ft_strjoin(char const *s1, char const *s2);

// @brief logs the error message and exits with the exit status in 'status' argument
void   errorLogger(char *message, int status);

// @brief displays messages for debugging
void   debugLogger(char *message);

#endif
