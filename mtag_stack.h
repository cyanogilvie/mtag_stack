#ifndef _MTAG_STACK_H
#define _MTAG_STACK_H
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#ifndef MTAG_STACK_STATIC_STORAGE
#	define MTAG_STACK_STATIC_STORAGE	256
#endif

#define MTAG_STACK_STATUS_CODES \
	X( OK,					"Ok"													) \
	X( ERROR,				"Unspecified error"										) \
	X( ALREADY_HARVESTED,	"Already harvested"										) \
	X( SEGMENT_MISMATCH,	".from and .to tag sets have different match counts"	) \
	X( NO_ARGS,				"No arguments supplied"									) \
	X( size,				""														)

enum mtag_stack_status {
#define X(sym, str)	MTAG_STACK_##sym,
MTAG_STACK_STATUS_CODES
#undef X
};

struct mtag_stack_hist {
	size_t		top;
	size_t		avail;
	uint8_t*	str;
	uint8_t		harvested;		// Flag to indicate that we've already allocated harvest storage in this space
	uint8_t		staticstorage[MTAG_STACK_STATIC_STORAGE];
};

struct mtag_stack {
	struct mtag_stack_hist*	hist;
	size_t		dist;
	uint32_t	ofs;
	uint32_t	count;
};

struct mtag_stack_harvest {
	size_t		count;
	union {
		struct {
			size_t*		len;					// Length of segment s: len[s]
			char**		str;					// String for segment s: str[s]
			size_t*		containing_count;		// Entries in segment s: containing_count[s]
			char***		containing;				// matches for .containing tags in segment s: containing[s][containing_count[s]]
			char**		containing_storage;
		};
		struct {
			char**		p;
		};
	};
};

#define MTAG_STACK(h)	(struct mtag_stack){.hist=(h)}

#define mtag_stack_hist_init(hist) \
	do { \
		struct mtag_stack_hist*	h = hist; \
		h->str = h->staticstorage; \
		h->top = 0; \
		h->avail = MTAG_STACK_STATIC_STORAGE; \
	} while(0)

#define mtag_stack_hist_reset(hist) \
	do { \
		struct mtag_stack_hist*	h = hist; \
		if (h->str != h->staticstorage) { \
			free(h->str); \
			h->str = h->staticstorage; \
			h->avail = MTAG_STACK_STATIC_STORAGE; \
			h->harvested = 0; \
		} \
		h->top = 0; \
	} while(0)

#define mtag_stack_hist_rewind(hist) \
	do { \
		struct mtag_stack_hist*	h = hist; \
		h->top = 0; \
		h->harvested = 0; \
	} while(0)

void	mtag_stack_add(struct mtag_stack*const tag, ptrdiff_t dist);
void	mtag_stack_prev(struct mtag_stack*const tag);
//char**	mtag_stack_harvest(struct mtag_stack*const tag, char* base, size_t* count);
const char*	mtag_stack_error_str(enum mtag_stack_status status);

struct mtag_stack_harvest_args {
	struct mtag_stack*		from;
	struct mtag_stack*		to;
	struct mtag_stack*		containing;
	struct mtag_stack*		loc;
};
struct mtag_stack_harvest* mtag_stack_harvest_(struct mtag_stack_hist* hist, enum mtag_stack_status* rc, char* base, struct mtag_stack_harvest_args* args);
#define mtag_stack_harvest(hist, rc, base, ...)  mtag_stack_harvest_(hist, rc, base, (struct mtag_stack_harvest_args[]){__VA_ARGS__,{0}})

#endif
