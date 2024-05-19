CC=gcc
CFLAGS=
AR=ar
ARFLAGS= -rcv
DOC=doxygen

PROGRAM=main.c
LIB= AMQP.a
OBJS= AMQP.o Connectivity.o Packet.o utils.o

#AMQP.o 
#Consumer.o Basic.o Queue.o Channel.o Connection.o 


##################################
.SUFFIXES: .c .o .a

.c.o:
	${CC} ${CFLAGS} -c $<

all: main.out

${LIB}: ${OBJS}
	${AR} ${ARFLAGS} $@ ${OBJS}

main.out: ${PROGRAM} ${OBJS} ${LIB}
	${CC} ${CFLAGS} -o $@ ${PROGRAM} ${LIB} -pthread


clean:
	@rm -rf *.o html *.a main.out

test:
	./main.out 127.0.0.1

retest: main.out test
