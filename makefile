objects := $(patsubst %.c,%.o,$(wildcard *.c)) $(patsubst %.c,%.o,$(wildcard **/*.c)) $(patsubst %.c,%.o,$(wildcard include/**/*.c)) $(patsubst %.c,%.o,$(wildcard include/**/**/*.c))
test_dir = $(shell pwd)/test/flight/
comp_dir = $(shell pwd)/test/comp/0/
GTEST_DIR = ./include/gtest/googletest

make : $(objects)
	cc -o igc_parser $(objects) -g -Og -lm -lcurl -lpthread -std=gnu99

%.o: %.c 
	cc -std=gnu99 -static -g -Og -c -o $@ $<

.PHONY: clean test format grind testComp

grind:
	for i in {0..11}; do valgrind --tool=memcheck --log-file=$(test_dir)$$i/memcheck.log --leak-check=full --xml=yes --xml-file=$(test_dir)$$i/memcheck.xml --track-origins=yes ./igc_parser "{\"source\": \"$(test_dir)$$i/test.igc\"}"; done

clean: 
	rm -rf *.o **/*.o test/**/*.kml test/**/*.js

format:
	clang-format -i -style="{BasedOnStyle: llvm, IndentWidth: 4, AllowShortFunctionsOnASingleLine: None, KeepEmptyLinesAtTheStartOfBlocks: false, ColumnLimit: 240, IndentCaseLabels: true}" *.c *.h **/*.c **/*.h

test:
	for i in {0..11}; do ./igc_parser "{\"source\": \"$(test_dir)$$i/test.igc\"}"; done

testComp:
	./igc_parser "{\"type\": \"comp\",\"sources\": [\"$(comp_dir)tracks/01.igc\" ,\"$(comp_dir)tracks/02.igc\", \"$(comp_dir)tracks/03.igc\"], \"destination\":\"$(comp_dir)\", \"task\": {\"coordinates\": [\"SU823188\",\"SU820164\",\"TQ256109\",\"SU823188\"]}}"

gtest:
	g++ -isystem ${GTEST_DIR}/include -I${GTEST_DIR} -pthread -c ${GTEST_DIR}/src/gtest-all.cc
	ar -rv libgtest.a gtest-all.o
	g++ -isystem ${GTEST_DIR}/include -pthread -lcurl ./test/tests.cpp libgtest.a -o tests
	./tests
