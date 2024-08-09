#define _POSIX_C_SOURCE 200809L

#include "mtag_stack.h"
#include <stdio.h>
#include <inttypes.h>
#include <time.h>
#include <errno.h>

static void mock_parse(char* str)
{
    struct mtag_stack_hist  tag_storage;
	mtag_stack_hist_init(&tag_storage);
	/*!mtags:re2c format = "struct mtag_stack @@={.b=&tag_storage}; "; */
	/*!rules:re2c:common
		re2c:api:style		= free-form;
		re2c:define:YYMTAGP	= "mtag_stack_push(&@@, cur-str);";
		re2c:define:YYMTAGN	= "@@.count=0;";
	*/
    struct mtag_stack t1 = {.b = &tag_storage};
    struct mtag_stack t2 = {.b = &tag_storage};
    mtag_stack_push(&t1, 1);
	mtag_stack_push(&t2, 4);

    mtag_stack_push(&t1, 5);
	mtag_stack_push(&t2, 8);

	mtag_stack_push(&t1, 9);
	mtag_stack_push(&t2, 10);

	mtag_stack_push(&t1, 8);
	mtag_stack_push(&t2, 15);

	mtag_stack_push(&t1, 16);
	mtag_stack_push(&t2, 19);

	mtag_stack_push(&t1, 20);
	mtag_stack_push(&t2, 24);

	size_t	t1c;
	[[maybe_unused]]char**	t1v = mtag_stack_harvest(&t1, str, &t1c);
	size_t	t2c;
	[[maybe_unused]]char**	t2v = mtag_stack_harvest(&t2, str, &t2c);

	if (t1c != t2c) {
		fprintf(stderr, "tag stacks are different lengths: %" PRIuPTR " != %" PRIuPTR "\n", t1c, t2c);
		exit(EXIT_FAILURE);
	}

	mtag_stack_hist_reset(&tag_storage);
}

int main(int argc, char** argv)
{
	char*		str = "/foo/bar/x/quux/baz/asfd";
	const int	it = 5000000;

	struct timespec	start_ts;
	if (-1 == clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start_ts)) perror("clock_gettime");

	for (size_t i=0; i<it; i++) mock_parse(str);

	struct timespec	end_ts;
	if (-1 == clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end_ts)) perror("clock_gettime");
	uint64_t elapsed_nsec = (end_ts.tv_sec-start_ts.tv_sec) * 1000000000 + end_ts.tv_nsec-start_ts.tv_nsec;
	printf("%d iterations: %.3f nanoseconds/iteration\n", it, elapsed_nsec/(double)it);

	return 0;
}


