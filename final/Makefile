TARGET=meibo_server
SRCS=${TARGET}.c
LOP=-L../mylib/ -lmylib
COP=-Wall -g

${TARGET}: ${SRCS}
	gcc -o $@ $^ ${LOP} ${COP}
