objects := $(patsubst %.c,%.o,$(wildcard *.c)) $(patsubst %.c,%.o,$(wildcard **/*.c)) $(patsubst %.c,%.o,$(wildcard include/**/*.c))

make : $(objects)
	cc -o igc_parser $(objects) -lm -std=c99

%.o: %.c 
	cc -std=c99 -c -o $@ $<	

.PHONY: clean test

clean: 
	rm -rf *.o **/*.o

test:
	./igc_parser