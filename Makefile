CC=g++
RELEASE=-DCAM_TABLE_SIZE=1024 -DDEFAULT_MIN_TTL=270 -DDEFAULT_CAM_CLEANUP=60
CFLAGS=-W -Wall -Wextra -pedantic -DNDEBUG -O2 $(RELEASE)
LDFLAGS=-lpthread -lnet -lpcap
PROG=switch

all: $(PROG)

$(PROG): mac.o cam.o port.o main.o
	$(CC) -o $@ $^ $(LDFLAGS)

mac.o: mac.cc mac.h
	$(CC) $(CFLAGS) -c -o $@ $<

cam.o: cam.cc cam.h mac.h port.h
	$(CC) $(CFLAGS) -c -o $@ $<

port.o: port.cc port.h
	$(CC) $(CFLAGS) -c -o $@ $<

main.o: main.cc mac.h port.h cam.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o $(PROG)

