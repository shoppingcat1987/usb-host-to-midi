CC=$(CROSS_COMPILE)gcc

all:convertor

convertor:convertor.o uart_util.o
	$(CC) convertor.o  uart_util.o -o convertor -lpthread

convertor.o:convertor.c
	$(CC) -c convertor.c -o convertor.o -lpthread
	
uart_util.o:uart_util.c
	$(CC) -c uart_util.c -o uart_util.o -lpthread
	

install:
	rm /sbin/convertor
	cp convertor /sbin/
	sync
	
clean:
	rm *.o
	rm convertor
