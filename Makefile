all: QRServer.c client.c
	gcc -g -o QRServer QRServer.c -lpthread
	gcc -g -o client client.c
	
QRServer: QRServer.c
	gcc -g -o QRServer QRServer.c -lpthread
	
client: client.c
	gcc -g -o client client.c

clean: QRServer.c client.c
	rm -f QRServer client
	gcc -g -o QRServer QRServer.c -lpthread
	gcc -g -o client client.c
