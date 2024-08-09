#ifndef _MTAG_STACK_H
#define _MTAG_STACK_H
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#ifndef MTAG_STACK_STATIC_STORAGE
#	define MTAG_STACK_STATIC_STORAGE	256
#endif

struct mtag_stack_hist {
    size_t		top;
    size_t		avail;
    uint8_t*	str;
    uint8_t		staticstorage[MTAG_STACK_STATIC_STORAGE];
};

struct mtag_stack {
	struct mtag_stack_hist*	b;
	size_t		ofs;
	size_t		dist;
	int			count;
};

#define mtag_stack_hist_init(b) \
	do { \
		(b)->str = (b)->staticstorage; \
		(b)->top = 0; \
		(b)->avail = MTAG_STACK_STATIC_STORAGE; \
	} while(0)

#define mtag_stack_hist_reset(b) \
	do { \
		if ((b)->str != (b)->staticstorage) { \
			free((b)->str); \
			(b)->str = (b)->staticstorage; \
			(b)->avail = MTAG_STACK_STATIC_STORAGE; \
		} \
		(b)->top = 0; \
	} while(0)

#define mtag_stack_hist_rewind(b) (b)->top = 0

void	mtag_stack_push(struct mtag_stack*const tag, ptrdiff_t dist);
void	mtag_stack_prev(struct mtag_stack*const tag);
char**	mtag_stack_harvest(struct mtag_stack*const tag, char* base, size_t* count);

#endif
