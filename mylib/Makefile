SRCS=utils.c
OBJ=${SRCS:.c=.o}
ARCHIVE=libmylib.a

${ARCHIVE}: ${OBJ}
	ar r $@ $^

${OBJ}: ${SRCS}
	gcc -c $^