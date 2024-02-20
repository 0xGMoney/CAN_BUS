CC = avr-gcc
CFLAGS = -Wall

mcp2515:
	$(CC) -mmcu=atmega328  mcp2515.c -o mcp2515.o $(CFLAGS)

clean:
	rm -rf *.o
