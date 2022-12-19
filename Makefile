CC=gcc

http-server: http-server.o htable.o session.o
	$(CC) -o http-server http-server.o htable.o session.o

session.o: session.c htable.h
	$(CC) -o session.o -c session.c

htable.o: htable.c
	$(CC) -o htable.o -c htable.c

http-server.o: http-server.c htable.h session.h
	$(CC) -o http-server.o -c http-server.c

clean:
	rm -rf *.o

mrproper: clean
	rm -rf http-server htable

