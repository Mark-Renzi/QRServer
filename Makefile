all: QRServer.c connect.c
	gcc -g -o QRServer QRServer.c
	gcc -g -o connect connect.c
	
QRServer: QRServer.c
	gcc -g -o QRServer QRServer.c
	
connect: connect.c
	gcc -g -o connect connect.c

clean: QRServer.c connect.c
	rm -f QRServer connect
	gcc -g -o QRServer QRServer.c
