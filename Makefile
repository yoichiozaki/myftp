CLIENT = myftpc
SERVER = myftpd
CC = gcc

all: myftpc myftpd

myftpc: myftpc.o
		        $(CC) -o myftpc myftpc.o

myftpc.o: myftpc.c
		        $(CC) -o myftpc.o -c myftpc.c

myftpd: myftpd.o
		        $(CC) -o myftpd myftpd.o

myftpd.o: myftpd.c
		        $(CC) -o myftpd.o -c myftpd.c

clean:
		        rm *.o $(CLIENT) $(SERVER)


