CC = gcc -pthread
CFLAGS = -g -Wall
TARGET = loadbalancer

all:$(TARGET)

$(TARGET): $(TARGET).o
	$(CC) -o $(TARGET) $(TARGET).o

$(TARGET).o: $(TARGET).c
	$(CC) $(CFLAGS) -c $(TARGET).c

clean:
	rm $(TARGET) $(TARGET).o
