prog : server.o registration.o broadcast.o bombing.o
	gcc -o prog $^
%.o : %.c 
	gcc -c $<
.depend : server.c registration.c  broadcast.c bombing.c
	gcc -MM $^ > .depend 
include .depend
clean : 
	rm -f *.o prog .depend
.PHONY: clean
