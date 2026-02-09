#ifndef UTILS_H
# define UTILS_H

# include <stddef.h> // size_t is declared here

char   **ft_split(char const *s, char c);
size_t doubleDArrayLength(void **arr);
char   *ft_strjoin(char const *s1, char const *s2);
int    isWithinRange(char *arg, size_t start, size_t end);
int    areAllDigits(char *arg);
size_t strlenWithoutLeadingZeros(char *str);
char   *ipv4Cleanup(char *arg);

void   errorLogger(char *message, int status);
void   debugLogger(char *message);

#endif
