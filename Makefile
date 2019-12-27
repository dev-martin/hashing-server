all:
	gcc	-Wall	-lcrypto	-Iincludes  -std=c99 -ggdb	-g	-o	client	src/client.c	src/diewithmessage.c	src/optparser.c	src/hash.c
	gcc	-Wall	-lcrypto	-Iincludes  -std=c99 -ggdb	-g	-o	server	src/server.c	src/diewithmessage.c	src/optparser.c	src/hash.c
 
