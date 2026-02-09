NAME		=	ft_ping
CC			=	gcc
CFLAGS		=	-Wall -Wextra -Werror
INCLUDE		=	-Iinclude

SRC_DIR		=	src
OBJ_DIR		=	obj
INC_DIR		=	include

SRCS		=	$(wildcard ${SRC_DIR}/*.c)
OBJS		=	$(SRCS:${SRC_DIR}/%.c=${OBJ_DIR}/%.o)

.PHONY		:	all clean fclean re run

all			:	${NAME}

${NAME}		:	${OBJ_DIR} ${OBJS}
				${CC} ${CFLAGS} ${OBJS} -o ${NAME}

${OBJ_DIR}	:
				mkdir -p ${OBJ_DIR}

${OBJ_DIR}/%.o	:	${SRC_DIR}/%.c
					${CC} ${CFLAGS} ${INCLUDE} -c $< -o $@

clean		:
				rm -rf ${OBJ_DIR}

fclean		:	clean
				rm -f ${NAME}

re			:	fclean all
