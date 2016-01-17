objects := $(patsubst %.c,%.o,$(wildcard *.c)) $(patsubst %.c,%.o,$(wildcard **/*.c)) $(patsubst %.c,%.o,$(wildcard include/**/*.c))
test_dir = /var/www/vhosts/uknxcl/extensions/include/igc_parser/test/

make : $(objects)
	cc -o igc_parser $(objects) -pg -lm -std=c99

%.o: %.c 
	cc -std=c99 -pg -c -o $@ $<

.PHONY: clean test format grind

grind:
	for i in {4..11}; do valgrind --tool=memcheck --log-file=$(test_dir)$$i/php.log --leak-check=full --track-origins=yes -v ./igc_parser $(test_dir)$$i/test.igc $(test_dir)$$i; done

clean: 
	rm -rf *.o **/*.o test/**/*.kml test/**/*.js

format:
	clang-format -i -style="{BasedOnStyle: llvm, IndentWidth: 4, AllowShortFunctionsOnASingleLine: None, KeepEmptyLinesAtTheStartOfBlocks: false, ColumnLimit: 240, IndentCaseLabels: true}" *.c *.h **/*.c **/*.h

test:
	for i in {0..11}; do ./igc_parser $(test_dir)$$i/test.igc $(test_dir)$$i; done
