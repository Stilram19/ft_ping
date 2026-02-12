// #include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include "utils.h"
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

// (*) errorLogger

void errorLogger(char *program_name, char *message, int status) {
    if (message != NULL) {
        fprintf(stderr, "%s: %s\n", program_name, message);
    } else {
        fprintf(stderr, "%s: (NULL)\n", program_name);
    }
    exit(status);
}

// (*) messageLogger

void debugLogger(char *program_name, char *message) {
    if (message != NULL) {
        printf("%s: %s\n", program_name, message);
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

// (*) get_milliseconds

uint32_t get_milliseconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000) & 0xFFFFFFFF;
}
