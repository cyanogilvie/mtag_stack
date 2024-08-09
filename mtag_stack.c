#include "mtag_stack.h"
#include <limits.h>
#include <string.h>

static size_t mtag_stack_hist_ensure_avail(struct mtag_stack_hist*const b, const size_t len)
{
    const uint32_t newlen = b->top + len;
    if (newlen > b->avail) {
		const size_t newavail = b->avail * 2;
		if (b->str == b->staticstorage) {
			uint8_t*const	newstr = malloc(newavail);
			memcpy(newstr, b->str, b->top);
			b->str = newstr;
		} else {
			b->str = realloc(b->str, newavail);
		}
		b->avail = newlen;
    }
	return b->top;
}

static void output_pack(struct mtag_stack_hist*const b, const uint64_t ofs, const int64_t dist)
{
	const size_t res = mtag_stack_hist_ensure_avail(b, 18);
	uint8_t*const	old = b->str + res;
	uint8_t*		o = old;
	uint64_t		n;

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

	b->top += o-old;
}

//>>>

void mtag_stack_push(struct mtag_stack*const tag, ptrdiff_t dist)
{
	struct mtag_stack_hist*const	b = tag->b;
	const size_t 		newofs = b->top;

	if (tag->count) output_pack(b, newofs - tag->ofs, dist - tag->dist);
	tag->ofs  = newofs;
	tag->dist = dist;
	tag->count++;
}

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

void mtag_stack_prev(struct mtag_stack*const tag)
{
	struct mtag_stack_hist*const	b = tag->b;
	const uint8_t*	o = b->str + tag->ofs;
	uint8_t			ch;
	int				len;
	uint64_t		n;

#define BRANCHLESS	1

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

char** mtag_stack_harvest(struct mtag_stack*const tag, char* base, size_t* count)
{
	const size_t	matches	= tag->count;
	char**			arr = (char**)(mtag_stack_hist_ensure_avail(tag->b, matches * sizeof(char*)) + tag->b->str);

	if (matches) {
		ptrdiff_t i = matches;
		while (1) {
			arr[--i] = base+tag->dist;
			if (i==0) break;
			mtag_stack_prev(tag);
		}
	}

	tag->b->top += matches * sizeof(char*);
	*count = matches;
	return arr;
}

