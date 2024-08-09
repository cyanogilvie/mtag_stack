DESTDIR =
PREFIX = /usr/local
OBJS = mtag_stack.o
CC = cc
RE2C = re2c
#RE2CARGS = -W -Werror --case-ranges
RE2CARGS = -W -Werror -Wno-nondeterministic-tags --case-ranges
CFLAGS_OPTIMIZE = -O3 -g
CFLAGS_DEBUG = -Og -g
LTO = -flto
CFLAGS = $(CFLAGS_OPTIMIZE) -g -std=c11 -Wall -Werror $(LTO) $(PGO) -I$(CURDIR)
LDFLAGS = $(LTO) $(PGO)
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

all: library generated test

library: libmtag_stack.so

libmtag_stack.so: $(OBJS)
	$(CC) -shared -fPIC -o libmtag_stack.$(VER).so $< $(LDFLAGS)
	ln -sf libmtag_stack.$(VER).so libmtag_stack.so

mtag_stack.o: mtag_stack.c mtag_stack.h
	$(CC) $(CFLAGS) -c $<

generated/%.c: %.re
	mkdir -p generated
	$(RE2C) $(RE2CARGS) -o $@ $<
	chmod a-w+r $@

generated/%.o: generated/%.c
	$(CC) $(CFLAGS) -c $< -o $@

generated: generated/test.c

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/lib
	mkdir -p $(DESTDIR)$(PREFIX)/include
	cp mtag_stack.h $(DESTDIR)$(PREFIX)/include
	cp libmtag_stack.$(VER).so $(DESTDIR)$(PREFIX)/lib
	ln -s libmtag_stack.$(VER).so $(DESTDIR)$(PREFIX)/lib/libmtag_stack.so

test: generated/test.o library
	$(CC) $(CFLAGS) -Wl,-rpath="$(CURDIR)" -o $@ $< -L. -I. -lmtag_stack

valgrind: test
	$(VALGRIND) $(VALGRINDARGS) ./test

cachegrind: test
	-rm cachegrind.out
	$(VALGRIND) $(CACHEGRINDARGS) ./test "benchmark 100000" simple
	cg_annotate cachegrind.out

pgo:
	rm -rf prof
	make -C . PGO="$(PGOGEN_BUILD)" clean all
	./test "benchmark 1000000" simple
	make -C . PGO="$(PGO_BUILD)" clean_bins all

coverage: all
	make -C . PGO="--coverage" clean test
	./test

vim-gdb: all
	vim -c "set number" -c "set mouse=a" -c "set foldlevel=100" -c "Termdebug -ex set\ print\ pretty\ on --args ./test" -c "2windo set nonumber" -c "1windo set nonumber" test.re

perf: test
	sudo perf record ./test "benchmark 5000000" simple
	sudo perf report

perf-stat: test
	sudo perf stat -d -d -d --repeat=10 --table ./test "benchmark 5000000" simple

clean_bins:
	-rm -rf *.o *.so.* *.so test

clean: clean_bins
	-rm -rf core prof *.gcda *.gcno perf.data perf.data.old cachegrind.out generated


PKG_DIR		= mtag_stack$(VER)
DIST_ROOT	= /tmp/dist
DIST_DIR	= $(DIST_ROOT)/$(PKG_DIR)

dist-clean:
	rm -rf $(DIST_DIR) $(DIST_ROOT)/$(PKG_DIR).tar.*

dist: generated
	mkdir -p $(DIST_DIR)
	mkdir -p $(DIST_DIR)/generated
	cp Makefile *.c *.h LICENSE README.md $(DIST_DIR)/
	cp generated/*.c $(DIST_DIR)/generated/
	(cd $(DIST_ROOT); tar czf $(PKG_DIR).tar.gz $(PKG_DIR))

.PHONY: all clean test valgrind dist vim-gdb library install pgo coverage perf perf-stats clean_bins dist dist-clean generated
