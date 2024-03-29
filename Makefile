all: libs/helper.o libs/udp.o libs/tcp.o pd.o fs.o as.o user.o
	gcc -g -o pd libs/helper.o libs/udp.o libs/tcp.o pd.o
	gcc -g -pthread -o fs libs/helper.o libs/udp.o libs/tcp.o fs.o
	gcc -g -pthread -o as libs/helper.o libs/udp.o libs/tcp.o as.o
	gcc -g -o user libs/helper.o libs/udp.o libs/tcp.o user.o

user.o: User.c libs/helper.o libs/udp.o libs/tcp.o
	gcc -g -o user.o -c User.c

as.o: AS.c libs/helper.o libs/udp.o libs/tcp.o
	gcc -g -o as.o -c AS.c

fs.o: FS.c libs/helper.o libs/udp.o libs/tcp.o
	gcc -g -o fs.o -c FS.c

pd.o: PD.c libs/helper.o libs/udp.o libs/tcp.o
	gcc -g -o pd.o -c PD.c

libs/tcp.o: libs/tcp.c libs/tcp.h libs/data.h
	gcc -g -o libs/tcp.o -c libs/tcp.c

libs/udp.o: libs/udp.c libs/udp.h libs/data.h
	gcc -g -o libs/udp.o -c libs/udp.c

libs/helper.o: libs/helper.c libs/helper.h
	gcc -g -o libs/helper.o -c libs/helper.c

clear:
	rm *.o libs/*.o pd fs as user
