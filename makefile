objects := $(patsubst %.c,%.o,$(wildcard *.c)) $(patsubst %.c,%.o,$(wildcard **/*.c)) $(patsubst %.c,%.o,$(wildcard include/**/*.c))
test_dir = "$(shell pwd)/test/"
GTEST_DIR = ./include/gtest/googletest

make : $(objects)
	cc -o igc_parser $(objects) -g -Og -lm -std=c99

%.o: %.c 
	cc -std=c99 -static -g -Og -c -o $@ $<

.PHONY: clean test format grind

grind:
	for i in {0..11}; do valgrind --tool=memcheck --log-file=$(test_dir)$$i/php.log --leak-check=full --track-origins=yes ./igc_parser "{\"source\": \"$(test_dir)$$i/test.igc\"}"; done

clean: 
	rm -rf *.o **/*.o test/**/*.kml test/**/*.js

format:
	clang-format -i -style="{BasedOnStyle: llvm, IndentWidth: 4, AllowShortFunctionsOnASingleLine: None, KeepEmptyLinesAtTheStartOfBlocks: false, ColumnLimit: 240, IndentCaseLabels: true}" *.c *.h **/*.c **/*.h

test:
	for i in {0..11}; do ./igc_parser "{\"source\": \"$(test_dir)$$i/test.igc\"}"; done

grind_source:
	valgrind --tool=memcheck --log-file=./grind.log --leak-check=full --track-origins=yes -v igc_parser "{\"source\": \"$(source)\"}";

gtest:
	g++ -isystem ${GTEST_DIR}/include -I${GTEST_DIR} -pthread -c ${GTEST_DIR}/src/gtest-all.cc
	ar -rv libgtest.a gtest-all.o
	g++ -isystem ${GTEST_DIR}/include -pthread ./test/tests.cpp libgtest.a -o tests
