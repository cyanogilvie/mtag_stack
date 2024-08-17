#define _POSIX_C_SOURCE 200809L

#define MTAG_STACK_STATIC_STORAGE		512
#include "mtag_stack.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <errno.h>
#include <string.h>

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

#if 0
		size_t	t1c;
		mtag_stack_harvest(&t1, str, &t1c);
		size_t	t2c;
		mtag_stack_harvest(&t2, str, &t2c);

		if (t1c != t2c) {
			fprintf(stderr, "tag stacks are different lengths: %" PRIuPTR " != %" PRIuPTR "\n", t1c, t2c);
			exit(EXIT_FAILURE);
		}

		/*
		printf("Parts:\n");
		for (int i=0; i<t1c; i++)
			printf("\t(%.*s)\n", (int)(t2v[i]-t1v[i]), t1v[i]);
			*/
#endif

		mtag_stack_hist_reset(&tag_hist);
	}

	struct timespec	end_ts;
	if (-1 == clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end_ts)) perror("clock_gettime");
	uint64_t elapsed_nsec = (end_ts.tv_sec-start_ts.tv_sec) * 1000000000 + end_ts.tv_nsec-start_ts.tv_nsec;
	if (g_it > 1) printf("\n%d iterations: %.3f nanoseconds/iteration\n", g_it, elapsed_nsec/(double)g_it);
}

//>>>
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
	REPORT(segments,	UINTMAX_C(18)*g_it);
	REPORT(relpaths,	UINTMAX_C(0)*g_it);
	REPORT(abspaths,	UINTMAX_C(3)*g_it);
	REPORT(unsafe,		UINTMAX_C(2)*g_it);
#undef REPORT
#undef TAB_FMT_H
#undef TAB_FMT_D

	struct timespec	end_ts;
	if (-1 == clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end_ts)) perror("clock_gettime");
	uint64_t elapsed_nsec = (end_ts.tv_sec-start_ts.tv_sec) * 1000000000 + end_ts.tv_nsec-start_ts.tv_nsec;
	if (g_it > 1) printf("\n%d iterations: %.3f nanoseconds/iteration\n", g_it, elapsed_nsec/(double)g_it);
}

//>>>
static void test_manyfiles2() //<<<
{
	struct timespec	start_ts;
	/*
	char	str[] = {
#embed "files.txt" suffix(0)
	};
	*/
	char*	str =
		"/home/cyan/git/mtag_stack\n"
		"/home/cyan/git/mtag_stack/.github\n"
		"/home/cyan/git/mtag_stack/.github/workflows\n"
		"/home/cyan/git/mtag_stack/.github/workflows/.vimrc\n"
		"/home/cyan/git/mtag_stack/.github/workflows/makefile.yml\n"
		"/home/cyan/git/mtag_stack/mtag_stack.o\n"
		"/home/cyan/git/mtag_stack/Makefile\n"
		"/home/cyan/git/mtag_stack/LICENSE\n"
		"/home/cyan/git/mtag_stack/README.md\n"
		"/home/cyan/git/mtag_stack/libmtag_stack.so\n"
		"/home/cyan/git/mtag_stack/generated\n"
		"/home/cyan/git/mtag_stack/generated/test.c\n"
		"/home/cyan/git/mtag_stack/generated/test.o\n"
		"/home/cyan/git/mtag_stack/mtag_stack.c\n"
		"/home/cyan/git/mtag_stack/.git\n"
		"/home/cyan/git/mtag_stack/.git/logs\n"
		"/home/cyan/git/mtag_stack/.git/logs/refs\n"
		"/home/cyan/git/mtag_stack/.git/logs/refs/heads\n"
		"/home/cyan/git/mtag_stack/.git/logs/refs/heads/master\n"
		"/home/cyan/git/mtag_stack/.git/logs/refs/remotes\n"
		"/home/cyan/git/mtag_stack/.git/logs/refs/remotes/origin\n"
		"/home/cyan/git/mtag_stack/.git/logs/refs/remotes/origin/master\n"
		"/home/cyan/git/mtag_stack/.git/logs/HEAD\n"
		"/home/cyan/git/mtag_stack/.git/branches\n"
		"/home/cyan/git/mtag_stack/.git/description\n"
		"/home/cyan/git/mtag_stack/.git/config\n"
		"/home/cyan/git/mtag_stack/.git/refs\n"
		"/home/cyan/git/mtag_stack/.git/refs/heads\n"
		"/home/cyan/git/mtag_stack/.git/refs/heads/master\n"
		"/home/cyan/git/mtag_stack/.git/refs/remotes\n"
		"/home/cyan/git/mtag_stack/.git/refs/remotes/origin\n"
		"/home/cyan/git/mtag_stack/.git/refs/remotes/origin/master\n"
		"/home/cyan/git/mtag_stack/.git/refs/tags\n"
		"/home/cyan/git/mtag_stack/.git/refs/tags/v1.0\n"
		"/home/cyan/git/mtag_stack/.git/refs/tags/v1.0.2\n"
		"/home/cyan/git/mtag_stack/.git/refs/tags/v1.0.3\n"
		"/home/cyan/git/mtag_stack/.git/refs/tags/v1.0.1\n"
		"/home/cyan/git/mtag_stack/.git/COMMIT_EDITMSG\n"
		"/home/cyan/git/mtag_stack/.git/ORIG_HEAD\n"
		"/home/cyan/git/mtag_stack/.git/HEAD\n"
		"/home/cyan/git/mtag_stack/.git/info\n"
		"/home/cyan/git/mtag_stack/.git/info/exclude\n"
		"/home/cyan/git/mtag_stack/.git/index\n"
		"/home/cyan/git/mtag_stack/.git/objects\n"
		"/home/cyan/git/mtag_stack/.git/objects/6d\n"
		"/home/cyan/git/mtag_stack/.git/objects/6d/9dfb623c0e28d9326010383e6c30584db7157a\n"
		"/home/cyan/git/mtag_stack/.git/objects/06\n"
		"/home/cyan/git/mtag_stack/.git/objects/06/d2429d994609c298d35872897bf0d3450e9404\n"
		"/home/cyan/git/mtag_stack/.git/objects/43\n"
		"/home/cyan/git/mtag_stack/.git/objects/43/b80f0069c1b9cfed09a9a295d938e6ef1f555b\n"
		"/home/cyan/git/mtag_stack/.git/objects/4a\n"
		"/home/cyan/git/mtag_stack/.git/objects/4a/00f2a49b9fdb55cc5a9e1cf9023f807bd23d89\n"
		"/home/cyan/git/mtag_stack/.git/objects/a1\n"
		"/home/cyan/git/mtag_stack/.git/objects/a1/6b73c8ea21949c4f39ad4c4a6282f5168e7187\n"
		"/home/cyan/git/mtag_stack/.git/objects/a4\n"
		"/home/cyan/git/mtag_stack/.git/objects/a4/85ad2dba7668729e2d8a4a5b08363d8c25e502\n"
		"/home/cyan/git/mtag_stack/.git/objects/6c\n"
		"/home/cyan/git/mtag_stack/.git/objects/6c/a88ccd1f5bc4767ffaa2281ae9bd0ad4ce0e7b\n"
		"/home/cyan/git/mtag_stack/.git/objects/54\n"
		"/home/cyan/git/mtag_stack/.git/objects/54/e7adae9360d08a5a12a37c29dcb5c46fe285d9\n"
		"/home/cyan/git/mtag_stack/.git/objects/b7\n"
		"/home/cyan/git/mtag_stack/.git/objects/b7/4d122e95b31742914b05ea9c9d4891462fe1dd\n"
		"/home/cyan/git/mtag_stack/.git/objects/27\n"
		"/home/cyan/git/mtag_stack/.git/objects/27/1e1b2aca99bd69d2727d0df1fb458372ba16ba\n"
		"/home/cyan/git/mtag_stack/.git/objects/95\n"
		"/home/cyan/git/mtag_stack/.git/objects/95/ee05b29b2d69c4febe8ab56417c1b992dd3a30\n"
		"/home/cyan/git/mtag_stack/.git/objects/95/b2e230e4f1fb45c0fc8daa933127dd7962efc0\n"
		"/home/cyan/git/mtag_stack/.git/objects/ba\n"
		"/home/cyan/git/mtag_stack/.git/objects/ba/383e9eb5c0a04d809b64d2c4cdb45a450fe58d\n"
		"/home/cyan/git/mtag_stack/.git/objects/ba/ed854098dc79a32b2650e3ac559ba38ee79df7\n"
		"/home/cyan/git/mtag_stack/.git/objects/pack\n"
		"/home/cyan/git/mtag_stack/.git/objects/f7\n"
		"/home/cyan/git/mtag_stack/.git/objects/f7/105c42e75e67445519fe2d5afaed2ddb4f935d\n"
		"/home/cyan/git/mtag_stack/.git/objects/b8\n"
		"/home/cyan/git/mtag_stack/.git/objects/b8/ee736c0c50b8f07c178860c1cd9f6a41affca1\n"
		"/home/cyan/git/mtag_stack/.git/objects/56\n"
		"/home/cyan/git/mtag_stack/.git/objects/56/64ea3477b8ced05da07eff46f700fdfc545c87\n"
		"/home/cyan/git/mtag_stack/.git/objects/14\n"
		"/home/cyan/git/mtag_stack/.git/objects/14/0d20a2f1d0e1715e0876d5c2217f3d1cf86083\n"
		"/home/cyan/git/mtag_stack/.git/objects/49\n"
		"/home/cyan/git/mtag_stack/.git/objects/49/0da0a28a4a3c4acdd535f391491a97b700c987\n"
		"/home/cyan/git/mtag_stack/.git/objects/49/0e0a16e986899bc1060bb021059b34369f9cc5\n"
		"/home/cyan/git/mtag_stack/.git/objects/00\n"
		"/home/cyan/git/mtag_stack/.git/objects/00/a2378a5a269e7469bad5a5945ab3ec48b8a691\n"
		"/home/cyan/git/mtag_stack/.git/objects/40\n"
		"/home/cyan/git/mtag_stack/.git/objects/40/c6d96bfe8566697e8ec23395ec24743e2af750\n"
		"/home/cyan/git/mtag_stack/.git/objects/9e\n"
		"/home/cyan/git/mtag_stack/.git/objects/9e/6718861794c5bb0a372552f476cb654deb637f\n"
		"/home/cyan/git/mtag_stack/.git/objects/42\n"
		"/home/cyan/git/mtag_stack/.git/objects/42/ba8de2dd4f8a9524546f037b2800389057de77\n"
		"/home/cyan/git/mtag_stack/.git/objects/23\n"
		"/home/cyan/git/mtag_stack/.git/objects/23/69cd1cfa548ef65e8e94e350a7c7acf90305c3\n"
		"/home/cyan/git/mtag_stack/.git/objects/c6\n"
		"/home/cyan/git/mtag_stack/.git/objects/c6/c85b961d756d58abf2b3db5273a6e0133b4542\n"
		"/home/cyan/git/mtag_stack/.git/objects/2b\n"
		"/home/cyan/git/mtag_stack/.git/objects/2b/509fbff75ff55abf70fa2a9bc88bc7e820a4ed\n"
		"/home/cyan/git/mtag_stack/.git/objects/f1\n"
		"/home/cyan/git/mtag_stack/.git/objects/f1/4cfb4057e6dcf9d1c7417bcc8b6ccc582ae680\n"
		"/home/cyan/git/mtag_stack/.git/objects/info\n"
		"/home/cyan/git/mtag_stack/.git/objects/fd\n"
		"/home/cyan/git/mtag_stack/.git/objects/fd/ddb29aa445bf3d6a5d843d6dd77e10a9f99657\n"
		"/home/cyan/git/mtag_stack/.git/objects/fd/4f141a2e2dc6aeb94a7dd8530b2034a042b147\n"
		"/home/cyan/git/mtag_stack/.git/objects/e5\n"
		"/home/cyan/git/mtag_stack/.git/objects/e5/fa2c93e216482b353680a6d8b18f3e5d31f14f\n"
		"/home/cyan/git/mtag_stack/.git/objects/d6\n"
		"/home/cyan/git/mtag_stack/.git/objects/d6/b6c4cff08ab1b009d5e75fae360ecfaf0c6380\n"
		"/home/cyan/git/mtag_stack/.git/objects/52\n"
		"/home/cyan/git/mtag_stack/.git/objects/52/61b572cd68a0a6ce50e2060a7e8ed47a900a86\n"
		"/home/cyan/git/mtag_stack/.git/FETCH_HEAD\n"
		"/home/cyan/git/mtag_stack/.git/hooks\n"
		"/home/cyan/git/mtag_stack/.git/hooks/pre-push.sample\n"
		"/home/cyan/git/mtag_stack/.git/hooks/pre-merge-commit.sample\n"
		"/home/cyan/git/mtag_stack/.git/hooks/post-update.sample\n"
		"/home/cyan/git/mtag_stack/.git/hooks/prepare-commit-msg.sample\n"
		"/home/cyan/git/mtag_stack/.git/hooks/applypatch-msg.sample\n"
		"/home/cyan/git/mtag_stack/.git/hooks/push-to-checkout.sample\n"
		"/home/cyan/git/mtag_stack/.git/hooks/pre-rebase.sample\n"
		"/home/cyan/git/mtag_stack/.git/hooks/fsmonitor-watchman.sample\n"
		"/home/cyan/git/mtag_stack/.git/hooks/update.sample\n"
		"/home/cyan/git/mtag_stack/.git/hooks/pre-receive.sample\n"
		"/home/cyan/git/mtag_stack/.git/hooks/pre-commit.sample\n"
		"/home/cyan/git/mtag_stack/.git/hooks/commit-msg.sample\n"
		"/home/cyan/git/mtag_stack/.git/hooks/pre-applypatch.sample\n"
		"/home/cyan/git/mtag_stack/mtag_stack.h\n"
		"/home/cyan/git/mtag_stack/test.re\n"
		"/home/cyan/git/mtag_stack/mtags.tcl\n"
		"/home/cyan/git/mtag_stack/libmtag_stack.1.0.so\n"
		"/home/cyan/git/mtag_stack/.gitignore\n"
		"/home/cyan/git/mtag_stack/test\n"
		;

	if (-1 == clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start_ts)) perror("clock_gettime");

	struct mtag_stack_hist	hist;
	mtag_stack_hist_init(&hist);

	for (uint64_t i=0; i<g_it; i++) {
		//mtag_stack_hist_rewind(&hist);
		char*	cur=str, *mar;
		for (;;) {
			char*	tok = cur;
			mtag_stack_hist_rewind(&hist);
			struct mtag_stack r1,r2,p1,p2,esc,dotdot,dot,dotdot2,dot2,esc2; r1=r2=p1=p2=esc=dotdot=dot=dotdot2=dot2=esc2=MTAG_STACK(&hist);
			char	*abs, *pend;
			/*!mtags:re2c:parse_manyfiles2 format = "struct mtag_stack @@=MTAG_STACK(&hist); "; */
			/*!stags:re2c:parse_manyfiles2 format = "char* @@; "; */
			/*!local:re2c:parse_manyfiles2
				re2c:yyfill:enable		= 0;
				re2c:api:style			= free-form;
				re2c:define:YYCTYPE		= char;
				re2c:define:YYCURSOR	= cur;
				re2c:define:YYMARKER	= mar;
				re2c:define:YYMTAGP		= "mtag_stack_add(&@@, cur-tok);";
				re2c:define:YYMTAGN		= "";
				re2c:tags				= 1;

				end			= [\x00];
				eol			= "\n";
				any			= [^] \ end;
				sep			= "/";
				esc			= "\\";
				pchar		= any \ eol \ sep \ esc;
				echar		= pchar | #esc  esc any;
				echar2		= pchar | #esc2 esc any;
				seg_plain	= pchar*;
				seg_esc		= echar+;
				seg_esc2	= echar2+;
				seg1
					= #dotdot ".."
					| #dot "."
					| seg_plain
					| seg_esc;
				seg2
					= #dotdot2 ".."
					| #dot2 "."
					| seg_plain
					| seg_esc2;
				path		= (sep @abs)? (#r1 seg1 #r2 (sep #p1 seg2 #p2)*)?;

				end			{ break; }

				path @pend eol	{
					enum mtag_stack_status	rc;
#if 0
#define BREAKPATH	"/home/cyan/git/mtag_stack/.git/logs/refs/heads/master"
					if (pend-tok == sizeof(BREAKPATH)-1 &&  memcmp(BREAKPATH, tok, pend-tok) == 0) {
						fprintf(stderr, "\n-- (%.*s)\n", (int)(pend-tok), tok);
					} else {
						fprintf(stderr, "\nxx (%.*s)\n", (int)(pend-tok), tok);
					}
#endif

					#define L(...) (struct mtag_stack[]){__VA_ARGS__, {0}}
					enum {SEG, ESC, REL};
					struct mtag_stack_harvest*	h = mtag_stack_harvest(&hist, &rc, tok,
						[SEG] = { .from=L(r1,p1), .to=L(r2,p2), .containing=L(esc,esc2) },
						[ESC] = { .loc=L(esc,esc2) },
						[REL] = { .loc=L(dotdot,dot,dotdot2,dot2) }
					);
					if (rc != MTAG_STACK_OK) {
						fprintf(stderr, "%s\n", mtag_stack_error_str(rc));
						exit(EXIT_FAILURE);
					}

					if (g_it <= 1) {
						//if (dotdot.count + dot.count + esc.count == 0)
						//	fprintf(stderr, "Canonical %s path: %.*s\n", abs ? "absolute" : "relative", (int)(pend-tok), tok);

						if (h[ESC].count + h[REL].count == 0)
							fprintf(stderr, "Canonical %s path: %.*s\n", abs ? "absolute" : "relative", (int)(pend-tok), tok);

						for (size_t i=0; i<h[SEG].count; i++)
							fprintf(stderr, "path segment: (%.*s)\n", (int)h[SEG].len[i], h[SEG].str[i]);
					}

					continue;
				}

				* {fprintf(stderr, "Parse failed"); goto finally;}
			*/
		}
	}

finally:
	mtag_stack_hist_reset(&hist);

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
		"manyfiles2" end			{ test_manyfiles2();									return; }
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
