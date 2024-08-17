#include "mtag_stack.h"
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

// LUT for number of trailing bytes given the leading byte of a packed encoding
// Ranges in designated initializers is a GNU extension (that TCC also supports)
static const uint8_t e_len[256] = {
	[0b10000000 ... 0b10111111] = 1,
	[0b11000000 ... 0b11011111] = 2,
	[0b11100000 ... 0b11101111] = 3,
	[0b11110000 ... 0b11110111] = 4,
	[0b11111000 ... 0b11111011] = 5,
	[0b11111100 ... 0b11111101] = 6,
	[0b11111110]                = 7,
	[0b11111111]                = 8
};

static const uint8_t e_mask[256] = {
	[0b00000000 ... 0b01111111] = 0b01111111,
	[0b10000000 ... 0b10111111] = 0b00111111,
	[0b11000000 ... 0b11011111] = 0b00011111,
	[0b11100000 ... 0b11101111] = 0b00001111,
	[0b11110000 ... 0b11110111] = 0b00000111,
	[0b11111000 ... 0b11111011] = 0b00000011,
	[0b11111100 ... 0b11111101] = 0b00000001
};

static size_t mtag_stack_hist_ensure_avail(struct mtag_stack_hist*const restrict hist, const size_t len) //<<<
{
    const uint32_t newlen = hist->top + len;
    if (newlen > hist->avail) {
		size_t newavail = hist->avail * 2;
		while (newavail < newlen) newavail <<= 1;
		if (hist->str == hist->staticstorage) {
			uint8_t*const	newstr = malloc(newavail);
			memcpy(newstr, hist->str, hist->top);
			hist->str = newstr;
		} else {
			hist->str = realloc(hist->str, newavail);
		}
		hist->avail = newavail;
    }
	return hist->top;
}

//>>>
static void output_pack(struct mtag_stack_hist*const restrict hist, const uint64_t ofs, const int64_t dist) //<<<
{
	const size_t		res = mtag_stack_hist_ensure_avail(hist, 18);
	uint8_t*const		old = hist->str + res;
	uint8_t*restrict	o = old;
	uint64_t			n;

	n = ofs;
	if (n < 1UL<<7) { // Identity encoding
		*o++ = n;
	} else if (n < 1UL<<(6+1*8)) { // Two byte encoding
		*o++ = 0b10000000 | ((n >>  8) & 0b00111111);
		*o++ =              ( n        & 0b11111111);
	} else if (n < 1UL<<(5+2*8)) { // Three byte encoding
		*o++ = 0b11000000 | ((n >> 16) & 0b00011111);
		*o++ =              ((n >>  8) & 0b11111111);
		*o++ =              (n         & 0b11111111);
	} else if (n < 1UL<<(4+3*8)) { // Four byte encoding
		*o++ = 0b11100000 | ((n >> 24) & 0b00001111);
		*o++ =              ((n >> 16) & 0b11111111);
		*o++ =              ((n >>  8) & 0b11111111);
		*o++ =              ( n        & 0b11111111);
	} else if (n < 1UL<<(3+4*8)) { // Five byte encoding
		*o++ = 0b11110000 | ((n >> 32) & 0b00000111);
		*o++ =              ((n >> 24) & 0b11111111);
		*o++ =              ((n >> 16) & 0b11111111);
		*o++ =              ((n >>  8) & 0b11111111);
		*o++ =              ( n        & 0b11111111);
	} else if (n < 1UL<<(2+5*8)) { // Six byte encoding
		*o++ = 0b11111000 | ((n >> 40) & 0b00000011);
		*o++ =              ((n >> 32) & 0b11111111);
		*o++ =              ((n >> 24) & 0b11111111);
		*o++ =              ((n >> 16) & 0b11111111);
		*o++ =              ((n >>  8) & 0b11111111);
		*o++ =              ( n        & 0b11111111);
	} else if (n < 1UL<<(1+6*8)) { // Seven byte encoding
		*o++ = 0b11111100 | ((n >> 48) & 0b00000001);
		*o++ =              ((n >> 40) & 0b11111111);
		*o++ =              ((n >> 32) & 0b11111111);
		*o++ =              ((n >> 24) & 0b11111111);
		*o++ =              ((n >> 16) & 0b11111111);
		*o++ =              ((n >>  8) & 0b11111111);
		*o++ =              ( n        & 0b11111111);
	} else if (n < 1UL<<(7*8)) { // Eight byte encoding
		*o++ = 0b11111110;
		*o++ =              ((n >> 48) & 0b11111111);
		*o++ =              ((n >> 40) & 0b11111111);
		*o++ =              ((n >> 32) & 0b11111111);
		*o++ =              ((n >> 24) & 0b11111111);
		*o++ =              ((n >> 16) & 0b11111111);
		*o++ =              ((n >>  8) & 0b11111111);
		*o++ =              ( n        & 0b11111111);
	} else /*if (n <= 0xFFFFFFFFFFFFFFFFUL)*/ { // Eight byte encoding
		*o++ = 0b11111111;
		*o++ =              ((n >> 56) & 0b11111111);
		*o++ =              ((n >> 48) & 0b11111111);
		*o++ =              ((n >> 40) & 0b11111111);
		*o++ =              ((n >> 32) & 0b11111111);
		*o++ =              ((n >> 24) & 0b11111111);
		*o++ =              ((n >> 16) & 0b11111111);
		*o++ =              ((n >>  8) & 0b11111111);
		*o++ =              ( n        & 0b11111111);
	}

	int64_t	sn = dist;
	int		neg = sn<0;
	if (neg) sn = -sn;
	n = (sn<<1) | neg;	// Signed magnitude representation, sign bit in LSB

	if (n < 1UL<<7) { // Identity encoding
		*o++ = n;
	} else if (n < 1UL<<(6+1*8)) { // Two byte encoding
		*o++ = 0b10000000 | ((n >>  8) & 0b00111111);
		*o++ =              ( n        & 0b11111111);
	} else if (n < 1UL<<(5+2*8)) { // Three byte encoding
		*o++ = 0b11000000 | ((n >> 16) & 0b00011111);
		*o++ =              ((n >>  8) & 0b11111111);
		*o++ =              (n         & 0b11111111);
	} else if (n < 1UL<<(4+3*8)) { // Four byte encoding
		*o++ = 0b11100000 | ((n >> 24) & 0b00001111);
		*o++ =              ((n >> 16) & 0b11111111);
		*o++ =              ((n >>  8) & 0b11111111);
		*o++ =              ( n        & 0b11111111);
	} else if (n < 1UL<<(3+4*8)) { // Five byte encoding
		*o++ = 0b11110000 | ((n >> 32) & 0b00000111);
		*o++ =              ((n >> 24) & 0b11111111);
		*o++ =              ((n >> 16) & 0b11111111);
		*o++ =              ((n >>  8) & 0b11111111);
		*o++ =              ( n        & 0b11111111);
	} else if (n < 1UL<<(2+5*8)) { // Six byte encoding
		*o++ = 0b11111000 | ((n >> 40) & 0b00000011);
		*o++ =              ((n >> 32) & 0b11111111);
		*o++ =              ((n >> 24) & 0b11111111);
		*o++ =              ((n >> 16) & 0b11111111);
		*o++ =              ((n >>  8) & 0b11111111);
		*o++ =              ( n        & 0b11111111);
	} else if (n < 1UL<<(1+6*8)) { // Seven byte encoding
		*o++ = 0b11111100 | ((n >> 48) & 0b00000001);
		*o++ =              ((n >> 40) & 0b11111111);
		*o++ =              ((n >> 32) & 0b11111111);
		*o++ =              ((n >> 24) & 0b11111111);
		*o++ =              ((n >> 16) & 0b11111111);
		*o++ =              ((n >>  8) & 0b11111111);
		*o++ =              ( n        & 0b11111111);
	} else if (n < 1UL<<(7*8)) { // Eight byte encoding
		*o++ = 0b11111110;
		*o++ =              ((n >> 48) & 0b11111111);
		*o++ =              ((n >> 40) & 0b11111111);
		*o++ =              ((n >> 32) & 0b11111111);
		*o++ =              ((n >> 24) & 0b11111111);
		*o++ =              ((n >> 16) & 0b11111111);
		*o++ =              ((n >>  8) & 0b11111111);
		*o++ =              ( n        & 0b11111111);
	} else /*if (n <= 0xFFFFFFFFFFFFFFFFUL)*/ { // Eight byte encoding
		*o++ = 0b11111111;
		*o++ =              ((n >> 56) & 0b11111111);
		*o++ =              ((n >> 48) & 0b11111111);
		*o++ =              ((n >> 40) & 0b11111111);
		*o++ =              ((n >> 32) & 0b11111111);
		*o++ =              ((n >> 24) & 0b11111111);
		*o++ =              ((n >> 16) & 0b11111111);
		*o++ =              ((n >>  8) & 0b11111111);
		*o++ =              ( n        & 0b11111111);
	}

	hist->top += o-old;
}

//>>>

void mtag_stack_add(struct mtag_stack*const restrict tag, ptrdiff_t dist) //<<<
{
	struct mtag_stack_hist*const restrict	hist = tag->hist;
	const size_t					newofs = hist->top;

	if (tag->count) output_pack(hist, newofs - tag->ofs, dist - tag->dist);
	tag->ofs  = newofs;
	tag->dist = dist;
	tag->count++;
}

//>>>
void mtag_stack_prev(struct mtag_stack*const restrict tag) //<<<
{
	struct mtag_stack_hist*const restrict	hist = tag->hist;
	const uint8_t*restrict	o = hist->str + tag->ofs;
	uint8_t			ch;
	int				len;
	uint64_t		n;

#define BRANCHLESS	0

#if BRANCHLESS == 0
	int				neg;
#endif

	ch = *o++;
	len = e_len[ch];
	n = ch & e_mask[ch];
	for (int i=len; i>0; i--) n = (n<<8) | *o++; 
	tag->ofs -= n;

	ch = *o++;
	len = e_len[ch];
	n = ch & e_mask[ch];
	for (int i=len; i>0; i--) n = (n<<8) | *o++; 
#if BRANCHLESS
	n = (-2*(n&1)+1) * (n>>1);
#else
	neg = n&1;
	n >>= 1;
	if (neg) n = -n;
#endif
	tag->dist -= n;
#undef BRANCHLESS
}

//>>>
static inline void mtag_stack_prev_inline(struct mtag_stack*const restrict tag) //<<<
{
	struct mtag_stack_hist*const restrict	hist = tag->hist;
	const uint8_t*	o = hist->str + tag->ofs;
	uint8_t			ch;
	int				len;
	uint64_t		n;

#define BRANCHLESS	0

#if BRANCHLESS == 0
	int				neg;
#endif

	ch = *o++;
	len = e_len[ch];
	n = ch & e_mask[ch];
	for (int i=len; i>0; i--) n = (n<<8) | *o++; 
	tag->ofs -= n;

	ch = *o++;
	len = e_len[ch];
	n = ch & e_mask[ch];
	for (int i=len; i>0; i--) n = (n<<8) | *o++; 
#if BRANCHLESS
	n = (-2*(n&1)+1) * (n>>1);
#else
	neg = n&1;
	n >>= 1;
	if (neg) n = -n;
#endif
	tag->dist -= n;
#undef BRANCHLESS
}

//>>>
static inline int argused(struct mtag_stack* tag) //<<<
{
	return tag && tag->hist;
}

//>>>
static inline size_t argmatches(struct mtag_stack* tag) //<<<
{
	size_t	count = 0;
	for (struct mtag_stack*restrict t=tag; argused(t); t++) count += t->count;
	return count;
}

//>>>
static inline size_t tagcount(struct mtag_stack* tag) //<<<
{
	size_t	count = 0;
	for (struct mtag_stack*restrict t=tag; argused(t); t++) count++;
	return count;
}

//>>>
static inline size_t consume_highest(struct mtag_stack* tag) //<<<
{
	size_t	highest, i=0;
	size_t	max = 0;

	for (struct mtag_stack*restrict t=tag; argused(t); t++, i++) {
		if (!t->count) continue;
		if (t->dist > max) {
			highest = i;
			max = t->dist;
		}
	}
	mtag_stack_prev_inline(&tag[highest]);
	tag[highest].count--;

	return max;
}

//>>>
struct mtag_stack_harvest* mtag_stack_harvest_(struct mtag_stack_hist*const restrict hist, enum mtag_stack_status*const restrict rc, char*const restrict base, struct mtag_stack_harvest_args*const restrict args) //<<<
{
	const struct mtag_stack_harvest_args*restrict	op = args;

	// Scan through our arguments to determine how much space to allocate <<<
	size_t	slots_required = 0;		// struct mtag_stack entries required
	size_t	bytes_required = 0;

	if (hist->harvested) {
		*rc = MTAG_STACK_ALREADY_HARVESTED;
		return NULL;
	}

	for (op=args; ; op++) {
		if (argused(op->from) && argused(op->to)) {
			const int from_matches			= argmatches(op->from);
			const int to_matches			= argmatches(op->to);
			const int containing_matches	= argmatches(op->containing);
			const int segments				= from_matches >= to_matches ? from_matches : to_matches;

			if (from_matches != to_matches) {
				*rc = MTAG_STACK_SEGMENT_MISMATCH;
				return NULL;
			}

			bytes_required += segments * sizeof(size_t);	// Space for .len array
			bytes_required += segments * sizeof(char*);		// Space for .str array
			bytes_required += segments * sizeof(size_t);	// Space for containing_count[seg] array
			bytes_required += segments * sizeof(char**);	// Space for containing[seg] pointers
			if (segments && containing_matches)
				bytes_required += containing_matches * sizeof(char*);
		} else if (argused(op->loc)) {
			bytes_required += argmatches(op->loc) * sizeof(char*);
		} else {
			// Nothing supplied - end of ops
			break;
		}
		slots_required++;
	}

	if (slots_required == 0) {
		*rc = MTAG_STACK_NO_ARGS;
		return NULL;
	}

	bytes_required += sizeof(struct mtag_stack_harvest[slots_required]);

	// Need to ensure space is available for the entire result before taking
	// any pointers into the storage (asking for more could realloc and invalidate
	// our pointers)
	mtag_stack_hist_ensure_avail(hist, bytes_required);
	//>>>

	struct mtag_stack_harvest* res = (struct mtag_stack_harvest*)(hist->str+hist->top);	hist->top += sizeof(struct mtag_stack_harvest[slots_required]);
	//memset(res, 0, bytes_required);

	struct mtag_stack_harvest*restrict	harvest;

	for (op=args, harvest=res; ; op++, harvest++) {
		if (argused(op->from) && argused(op->to)) {
			const int from_matches			= argmatches(op->from);
			const int to_matches			= argmatches(op->to);
			const int containing_matches	= argmatches(op->containing);

			const int segments = from_matches >= to_matches ? from_matches : to_matches;
			harvest->count = segments;
			harvest->len = (size_t*)(hist->str+hist->top);				hist->top += segments * sizeof(size_t);
			harvest->str = (char**)(hist->str+hist->top);				hist->top += segments * sizeof(char*);
			harvest->containing_count = (size_t*)(hist->str+hist->top);	hist->top += segments * sizeof(size_t);
			harvest->containing = (char***)(hist->str+hist->top);		hist->top += segments * sizeof(char**);

			if (segments && containing_matches) {
				harvest->containing_storage = (char**)(hist->str+hist->top);	hist->top += containing_matches * sizeof(char*);

				for (ptrdiff_t i=containing_matches-1; i>=0; i--)
					harvest->containing_storage[i] = base + consume_highest(op->containing);
			}

			for (ptrdiff_t segment=segments-1; segment>=0; segment--) {
				char*	from = base + consume_highest(op->from);
				char*	to   = base + consume_highest(op->to);
				harvest->str[segment] = from;
				harvest->len[segment] = to-from;

				// Record the .containing matches within this segment
				for (size_t c=0; c<containing_matches; c++) {
					char*const restrict		cont = harvest->containing_storage[c];
					if (cont < from) continue;
					if (cont >= to) break;
					if (!harvest->containing[segment])
						harvest->containing[segment] = &harvest->containing_storage[c];
					harvest->containing_count++;
				}
			}
		} else if (argused(op->loc)) {
			const size_t	matches = argmatches(op->loc);
			harvest->count = matches;
			harvest->p = (char**)(hist->str+hist->top);		hist->top += matches * sizeof(char*);
			for (ptrdiff_t i=matches-1; i>=0; i--) harvest->p[i] = base + consume_highest(op->containing);
		} else {
			// Nothing supplied - end of ops
			break;
		}
	}

	hist->harvested = 1;
	*rc = MTAG_STACK_OK;

	return res;
}

//>>>

static const char* error_str[MTAG_STACK_size+1] = {
#define X(sym, str)	str,
MTAG_STACK_STATUS_CODES
#undef X
};

const char*	mtag_stack_error_str(enum mtag_stack_status status) //<<<
{
	if (status < 0 || status >= MTAG_STACK_size) return "Invalid status";
	return error_str[status];
}

//>>>

// vim: foldmethod=marker foldmarker=<<<,>>> ts=4 shiftwidth=4
