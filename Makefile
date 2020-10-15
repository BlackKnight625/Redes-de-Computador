all: libs/helper.o libs/udp.o libs/tcp.o pd.o
	gcc -g -o pd libs/helper.o libs/udp.o libs/tcp.o pd.o

pd.o: PD.c libs/helper.h libs/udp.h libs/tcp.h
	gcc -g -o pd.o -c PD.c

libs/tcp.o: libs/tcp.c libs/tcp.h libs/data.h
	gcc -g -o libs/tcp.o -c libs/tcp.c

libs/udp.o: libs/udp.c libs/udp.h libs/data.h
	gcc -g -o libs/udp.o -c libs/udp.c

libs/helper.o: libs/helper.c libs/helper.h
	gcc -g -o libs/helper.o -c libs/helper.c

clear:
	rm *.o libs/*.o pd
