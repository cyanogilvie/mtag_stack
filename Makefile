DESTDIR =
PREFIX = /usr/local
OBJS = mtag_stack.o
CC = cc
CFLAGS = -O3 -g -std=c11 -Wall -Werror -flto $(PGO)
LDFLAGS = -flto $(PGO)
VER = 1.0
PGOGEN_BUILD = -fprofile-generate=prof
PGO_BUILD = -fprofile-use=prof -fprofile-partial-training
PGO =
VALGRIND = valgrind
VALGRINDARGS_EXTRA =
VALGRINDARGS = --tool=memcheck --num-callers=8 --leak-resolution=high \
			   --leak-check=yes -v --keep-debuginfo=yes \
			   --trace-children=yes $(VALGRINDARGS_EXTRA)
CACHEGRINDARGS	= --tool=cachegrind --cachegrind-out-file=cachegrind.out --cache-sim=yes --branch-sim=no

all: library

library: libmtag_stack.so

libmtag_stack.so: $(OBJS)
	$(CC) -shared -fPIC -o libmtag_stack.$(VER).so $< $(LDFLAGS)
	ln -sf libmtag_stack.$(VER).so libmtag_stack.so

mtag_stack.o: mtag_stack.c mtag_stack.h
	$(CC) $(CFLAGS) -c $<

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/lib
	mkdir -p $(DESTDIR)$(PREFIX)/include
	cp mtag_stack.h $(DESTDIR)$(PREFIX)/include
	cp libmtag_stack.so.$(VER) $(DESTDIR)$(PREFIX)/lib
	ln -s libmtag_stack.so.$(VER) $(DESTDIR)$(PREFIX)/lib/libmtag_stack.so

test: test.c library
	$(CC) $(CFLAGS) -Wl,-rpath "$(CURDIR)" -o $@ $< -L. -I. -lmtag_stack

benchmark: benchmark.c library
	$(CC) $(CFLAGS) -Wl,-rpath "$(CURDIR)" -o $@ $< -L. -I. -lmtag_stack

valgrind: test
	$(VALGRIND) $(VALGRINDARGS) ./test

cachegrind: benchmark
	-rm cachegrind.out
	$(VALGRIND) $(CACHEGRINDARGS) ./$<
	cg_annotate cachegrind.out

pgo:
	rm -rf prof
	make -C . PGO="$(PGOGEN_BUILD)" clean benchmark
	./benchmark
	make -C . PGO="$(PGO_BUILD)" clean_bins benchmark

coverage: all
	make -C . PGO="--coverage" clean test
	./test

vim-gdb: all
	vim -c "set number" -c "set mouse=a" -c "set foldlevel=100" -c "Termdebug -ex set\ print\ pretty\ on --args ./test" -c "2windo set nonumber" -c "1windo set nonumber" test.c

perf: benchmark
	sudo perf record ./benchmark
	sudo perf report

perf-stat: benchmark
	sudo perf stat -d -d -d --repeat=10 --table ./benchmark

clean_bins:
	-rm -rf *.o *.so.* *.so test benchmark

clean: clean_bins
	-rm -rf core prof *.gcda *.gcno perf.data perf.data.old cachegrind.out

.PHONY: all clean test valgrind dist vim-gdb library install pgo coverage perf perf-stats clean_bins
