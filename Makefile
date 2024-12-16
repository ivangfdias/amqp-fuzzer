CC=gcc
GLIB=`pkg-config --cflags glib-2.0`
AR=ar
ARFLAGS= -rcv
DOC=doxygen

PROGRAM=main.c
LIB= AMQP.a libpacket.a
OBJS= AMQP.o Packet.o utils.o grammar-parser.o packet-generator.o Connection.o Grammar_Interface.o

#AMQP.o 
#Consumer.o Basic.o Queue.o Channel.o Connection.o 


##################################
.SUFFIXES: .c .o .a

all: generator.o main.out 
	
.c.o:
	${CC} ${CFLAGS} ${GLIB} -c $<

generator.o:
	${CC} ${CFLAGS} ${GLIB} -c ../grammar/packet-generator.c ../grammar/grammar-parser.c -lglib-2.0


${LIB}: ${OBJS} ${generator.o}
	${AR} ${ARFLAGS} $@ ${OBJS}

main.out: ${PROGRAM} ${OBJS} ${LIB} ${generator.o}
	${CC} ${CFLAGS} ${GLIB} -o $@ ${PROGRAM} ${LIB} -pthread -lglib-2.0


clean:
	@rm -rf *.o html *.a main.out logs/*


test-5m-r0-p0:
	./monitor.sh ./main.out 127.0.0.1 -t 300 -R 0 -P 0

test-5m-r1-p0:
	./monitor.sh ./main.out 127.0.0.1 -t 300 -R 1 -P 0
test-5m-r2-p0:
	./monitor.sh ./main.out 127.0.0.1 -t 300 -R 2 -P 0
test-5m-r3-p0:
	./monitor.sh ./main.out 127.0.0.1 -t 300 -R 3 -P 0
