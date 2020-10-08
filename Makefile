all: libs/helper.o pd.o libs/udp.o
	gcc -g -o pd libs/helper.o pd.o libs/udp.o

libs/udp.o: libs/udp.c libs/udp.h
	gcc -g -o libs/udp.o -c libs/udp.c

pd.o: PD.c libs/helper.h libs/udp.h
	gcc -g -o pd.o -c PD.c

libs/helper.o: libs/helper.c libs/helper.h
	gcc -g -o libs/helper.o -c libs/helper.c

clear:
	rm *.o libs/*.o pd
