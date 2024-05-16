CC=gcc
CFLAGS=
AR=ar
ARFLAGS= -rcv
DOC=doxygen

PROGRAM=main.c
LIB= AMQP.a
OBJS= AMQP.o Connectivity.o

#AMQP.o 
#Consumer.o Basic.o Queue.o Channel.o Connection.o 


##################################
.SUFFIXES: .c .o .a

.c.o:
	${CC} ${CFLAGS} -c $<

all: clear_temp main.out

${LIB}: ${OBJS}
	${AR} ${ARFLAGS} $@ ${OBJS}

clear_temp: 
	@rm -rf /tmp/redes-ep1-ivan

main.out: ${PROGRAM} ${OBJS} ${LIB}
	${CC} ${CFLAGS} -o $@ ${PROGRAM} ${LIB} -pthread


clean:
	@rm -rf *.o html *.a main.out

