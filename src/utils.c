// #include <ctype.h>
# include <stdlib.h>
# include <stdio.h>
# include <string.h>
// # include "macros.h"

// @brief function copies up to dstsize - 1 characters 
// from the NUL-terminated string src to dest, NUL-terminating the result
static size_t	ft_strlcpy(char *dest, const char *src, size_t dstsize)
{
	size_t	i;

	i = 0;
	if (!dstsize)
		return (strlen(src));
	while (i < dstsize - 1 && *(src + i))
	{
		*(dest + i) = *(src + i);
		i++;
	}
	*(dest + i) = '\0';
	return (strlen(src));
}

// (*) split:

// static size_t	ft_arrlen(char const *s, char c) {
// 	size_t	i;
// 	size_t	size;
//
// 	i = 0;
// 	size = 0;
// 	while (*(s + i))
// 	{
// 		while (*(s + i) && *(s + i) == c)
// 			i++;
// 		if (*(s + i))
// 			size++;
// 		while (*(s + i) && *(s + i) != c)
// 			i++;
// 	}
// 	return (size);
// }
//
// static void	cleanup(void	*ptr, size_t j) {
// 	while (j--)
// 		free((void *)ptr + j);
// 	free(ptr);
// }
//
// static char	*ft_allocate_word(char **arr, const char **s, char c, int j) {
// 	size_t		i;
// 	char		*str;
// 	const char	*s1;
//
// 	i = 0;
// 	s1 = *s;
// 	while (*(s1 + i) && *(s1 + i) != c)
// 		i++;
// 	*s = s1 + i;
// 	str = malloc((i + 1) * sizeof(char));
// 	if (!str)
// 	{
// 	    cleanup(arr, j);
// 		return (0);
// 	}
// 	*(str + i) = 0;
// 	while (i > 0)
// 	{
// 		i--;
// 		*(str + i) = *(s1 + i);
// 	}
// 	return (str);
// }
//
// // @brief splits the given string with the character c
// // @param c the separator
// char	**ft_split(char const *s, char c) {
// 	size_t	j;
// 	char	**arr;
//
// 	j = 0;
// 	if (!s)
// 		return (0);
// 	arr = malloc((ft_arrlen(s, c) + 1) * sizeof(char *));
// 	if (!arr)
// 		return (0);
// 	while (*s)
// 	{
// 		while (*s && *s == c)
// 			s++;
// 		if (*s)
// 		{
// 			*(arr + j) = ft_allocate_word(arr, &s, c, j);
// 			if (!*(arr + j))
// 				return (0);
// 			j++;
// 		}
// 	}
// 	*(arr + j) = 0;
// 	return (arr);

// (*) doubleDArrayLength

// @brief returns the length of the given 2D array
// size_t doubleDArrayLength(void **arr) {
//     if (arr == NULL) {
//         return (0);
//     }
//
//     int i = 0;
//
//     while (arr[i] != NULL) {
//         i += 1;
//     }
//
//     return (i);
// }

// (*) errorLogger

void errorLogger(char *message, int status) {
    if (message != NULL) {
        printf("ping: %s\n", message);
    }

    exit(status);
}

// (*) messageLogger

void debugLogger(char *message) {
    if (message != NULL) {
        printf("ping: %s\n", message);
        return ;
    }
    printf("ping: (NULL)\n");
}


// (*) strjoin
// @brief joins the two strings into a new place in memory
char	*ft_strjoin(char const *s1, char const *s2) {
	size_t	i;
	size_t	j;
	char	*join;

	if (!(s1 && s2))
		return (0);
	i = strlen(s1);
	j = strlen(s2);
	join = malloc((i + j + 1) * sizeof(char));
	if (!join)
		return (0);
	ft_strlcpy(join, s1, i + 1);
	ft_strlcpy(join + i, s2, j + 1);
	return (join);
}

// (*) areAllDigits

// @brief given arg, checks that all characters are digits (no '-' or '+' signs)
// int areAllDigits(char *arg) {
//     if (!arg) {
//         return (FALSE);
//     }
//
//     for (int i = 0; arg[i] != '\0'; i++) {
//         if (!isdigit(arg[i])) {
//             return (FALSE);
//         }
//     }
//
//     return (TRUE);
// }
//
//
// // @brief given a postive integer or zero, calculates the length of the string representation of the integer
// static size_t numberLen(size_t num) {
//     size_t ret = 0;
//
//     while (num) {
//         num /= 10;
//         ret += 1;
//     }
//
//     return (ret);
// }
//
// // (*) isWithinRange
//
// // @brief checks if arg (string representation of a positive integer or zero) is within the range [start, end]
// // @param arg must be the string representation of an integer (all digits)
// int isWithinRange(char *arg, int start, int end) {
//     if (!arg || start < 0 || end < 0) {
//         return (FALSE);
//     }
//
//     size_t argLen = strlen(arg);
//
//     if (argLen > numberLen(end) || argLen > numberLen(start)) {
//         return (FALSE);
//     }
//
//     int argInt = atoi(arg);
//
//     return (argInt >= start && argInt <= end);
// }
//
// // (*) strlenWithoutLeadingZeros
//
// // @brief returns the string length but without counting leading zeros
// size_t strlenWithoutLeadingZeros(char *str) {
//     size_t i = 0;
//     size_t j = 0;
//
//     // skipping trailing spaces
//     while (str[i] != '\0' && str[i] == '0') {
//         i += 1;
//     }
//
//     while (str[i + j] != '\0') {
//         j += 1;
//     }
//
//     return (j);
// }
