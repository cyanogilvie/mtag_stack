#define _POSIX_C_SOURCE 200809L

//#define MTAG_STACK_STATIC_STORAGE		512
#include "mtag_stack.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <errno.h>

/*!rules:re2c:common
	re2c:api:style			= free-form;
	re2c:yyfill:enable		= 0;
	re2c:define:YYCTYPE		= char;
	re2c:define:YYCURSOR	= cur;
	re2c:define:YYMARKER	= mar;
	re2c:define:YYMTAGP		= "mtag_stack_add(&@@, cur-str);";
	re2c:define:YYMTAGN		= "";
	re2c:tags				= 1;

	end			= [\x00];
	eol			= [\n];
	ws			= [ \t];
	digit		= [0-9];
	hexdigit	= [0-9A-Fa-f];
	decimal		= @d1 digit+ @d2;
	hex			= "0x" @h2 hexdigit+ @h1;
	num
		= decimal
		| hex;
*/

struct counts {
	size_t	segments;
	size_t	relpaths;
	size_t	abspaths;
	size_t	unsafe;
};

static int		g_it = 1;

uint8_t	declut['9'+1] = {['1'] = 1, 2, 3, 4, 5, 6, 7, 8, 9};
uint8_t	hexlut['f'+1] = {
	['1'] = 1, 2, 3, 4, 5, 6, 7, 8, 9,
	['A'] = 10, 11, 12, 13, 14, 15,
	['a'] = 10, 11, 12, 13, 14, 15
};

static int64_t parsenum(char* d1, char* d2, char* h1, char* h2, int64_t def) //<<<
{
	uint64_t		res = 0;
	const uint8_t*	lut;
	int				radix;
	uint8_t			*p, *e;
	if (d1) {
		lut = declut;
		p = (uint8_t*)d1;
		e = (uint8_t*)d2;
		radix = 10;
	} else if (h1) {
		lut = hexlut;
		p = (uint8_t*)h1;
		e = (uint8_t*)h2;
		radix = 16;
	} else {
		return res = def;
	}

	while (p<e)
		res = res*radix + lut[*p++];

	return res;
}

//>>>

static const char* parse(char* str, struct counts* counts) //<<<
{
	const char*				err = NULL;
    struct mtag_stack_hist	tag_hist;

	mtag_stack_hist_init(&tag_hist);

	char*	cur = str, *mar;
	for (;;) {
		struct mtag_stack	dotdot,dot,name,p1,p2;	dotdot=dot=name=p1=p2 = MTAG_STACK(&tag_hist);
		/*!mtags:re2c:parse format = "struct mtag_stack @@=MTAG_STACK(&tag_hist); "; */
		/*!local:re2c:parse
			!use:common;

			sep		= [/];
			pchar	= [^] \ end \ sep \ eol;
			name	= #name pchar*;
			pseg
				= #dotdot ".."
				| #dot "."
				| name;

			//relpath	= (#p1 pseg #p2)+;
			abspath	= (sep #p1 pseg #p2)*;

			end		{ break; }
			eol		{ continue; }

			abspath eol	{ counts->abspaths++; goto sawpath; }
			//relpath eol { counts->relpaths++; goto sawpath; }
			*	{
				cur--;
				fprintf(stderr, "Failed to match at %ld: %s\n", cur-str, cur);
				err = "Parse error"; goto finally;
			}
		*/
	sawpath:
		if (dotdot.count) counts->unsafe++;
		counts->segments += dotdot.count + dot.count + name.count;
		continue;
	}

finally:
	mtag_stack_hist_reset(&tag_hist);

	return err;
}

//>>>
static void mock_parse(char* str) //<<<
{
	struct timespec	start_ts;
	if (-1 == clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start_ts)) perror("clock_gettime");

	for (uint64_t i=0; i<g_it; i++) {
		struct mtag_stack_hist  tag_hist;
		mtag_stack_hist_init(&tag_hist);
		struct mtag_stack t1 = {.hist = &tag_hist};
		struct mtag_stack t2 = {.hist = &tag_hist};
		mtag_stack_add(&t1, 1);
		mtag_stack_add(&t2, 4);

		mtag_stack_add(&t1, 5);
		mtag_stack_add(&t2, 8);

		mtag_stack_add(&t1, 9);
		mtag_stack_add(&t2, 10);

		mtag_stack_add(&t1, 8);
		mtag_stack_add(&t2, 15);

		mtag_stack_add(&t1, 16);
		mtag_stack_add(&t2, 19);

		mtag_stack_add(&t1, 20);
		mtag_stack_add(&t2, 24);

		size_t	t1c;
		[[maybe_unused]]char**	t1v = mtag_stack_harvest(&t1, str, &t1c);
		size_t	t2c;
		[[maybe_unused]]char**	t2v = mtag_stack_harvest(&t2, str, &t2c);

		if (t1c != t2c) {
			fprintf(stderr, "tag stacks are different lengths: %" PRIuPTR " != %" PRIuPTR "\n", t1c, t2c);
			exit(EXIT_FAILURE);
		}

		/*
		printf("Parts:\n");
		for (int i=0; i<t1c; i++)
			printf("\t(%.*s)\n", (int)(t2v[i]-t1v[i]), t1v[i]);
			*/

		mtag_stack_hist_reset(&tag_hist);
	}

	struct timespec	end_ts;
	if (-1 == clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end_ts)) perror("clock_gettime");
	uint64_t elapsed_nsec = (end_ts.tv_sec-start_ts.tv_sec) * 1000000000 + end_ts.tv_nsec-start_ts.tv_nsec;
	if (g_it > 1) printf("\n%d iterations: %.3f nanoseconds/iteration\n", g_it, elapsed_nsec/(double)g_it);
}


static void test_simple() //<<<
{
	struct timespec	start_ts;
	if (-1 == clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start_ts)) perror("clock_gettime");

	struct counts	c = {0};
	for (uint64_t i=0; i<g_it; i++) {
		char*	str =
			"/foo/bar/x/quux/baz/asfd\n"
			"/foo/../x/quux/./asfd\n\n"
			"/../../x/quux/./asfd\n"
			;

		//mock_parse(str);

		const char*	parseerr = parse(str, &c);
		if (parseerr) {
			fprintf(stderr, "Parse failed: %s\n", parseerr);
			exit(EXIT_FAILURE);
		}
	}

#define TAB_FMT_H		"%15s %12s %12s\n"
#define TAB_FMT_D		"%15s %12" PRIdPTR " %12" PRIdPTR "\n"
	printf(TAB_FMT_H, "",			"Expect",	"Got");
#define REPORT(f, ex) printf(TAB_FMT_D, #f, (ex), c.f);
	REPORT(segments,	UINTMAX_C(6)*g_it);
	REPORT(relpaths,	UINTMAX_C(0)*g_it);
	REPORT(abspaths,	UINTMAX_C(1)*g_it);
	REPORT(unsafe,		UINTMAX_C(0)*g_it);
#undef REPORT
#undef TAB_FMT_H
#undef TAB_FMT_D

	struct timespec	end_ts;
	if (-1 == clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end_ts)) perror("clock_gettime");
	uint64_t elapsed_nsec = (end_ts.tv_sec-start_ts.tv_sec) * 1000000000 + end_ts.tv_nsec-start_ts.tv_nsec;
	if (g_it > 1) printf("\n%d iterations: %.3f nanoseconds/iteration\n", g_it, elapsed_nsec/(double)g_it);
}

//>>>
static void parse_arg(char* arg) //<<<
{
	char*	cur = arg, *mar;
	char	*d1, *d2, *h1, *h2;
	/*!stags:re2c:parse_arg format = "char* @@; "; */
	/*!local:re2c:parse_arg
		!use:common;

		"simple" end				{ test_simple();										return; }
		"mock" end					{ mock_parse("/foo/bar/x/quux/baz/asfd");				return; }
		"benchmark" (ws+ num)? end	{ g_it = parsenum(d1, d2, h1, h2, 1000000);				return; }
		*							{ fprintf(stderr, "Unrecognised command: %s\n", arg);	exit(EXIT_FAILURE); }
	*/
}

//>>>

int main(int argc, char** argv) //<<<
{
	for (int i=1; i<argc; i++) parse_arg(argv[i]);
	return EXIT_SUCCESS;
}

//>>>

// vim: ft=c foldmethod=marker foldmarker=<<<,>>> ts=4 shiftwidth=4
