CC      ?= gcc
CFLAGS  ?= -O2 -static -s
SRC      = apt-compat.c apt-backends.c
HDR      = apt-backends.h

apt: $(SRC) $(HDR)
	$(CC) $(CFLAGS) -o apt $(SRC)

clean:
	rm -f apt

.PHONY: clean
