CC=gcc
CFLAGS += $(shell pkg-config --cflags json-c) -fPIC
LDFLAGS += $(shell pkg-config --libs json-c)

all: daemon.o device_listener.o subscription.o cli.o hashmap.o test.o libdevice/device_api.o
	$(CC) -o message_collector_daemon device_listener.o daemon.o subscription.o cli.o hashmap.o $(LDFLAGS)
	$(CC) -o client-test test.o
	$(LINK.c) -shared -o libdevice/libdevice libdevice/device_api.o
	
clean:
	rm -f *.o 
	rm -f libdevice/*.so libdevice/*.o