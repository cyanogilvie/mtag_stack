#include "mtag_stack.h"
#include <stdio.h>
#include <inttypes.h>

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
	char**	t1v = mtag_stack_harvest(&t1, str, &t1c);
	size_t	t2c;
	char**	t2v = mtag_stack_harvest(&t2, str, &t2c);

	if (t1c != t2c) {
		fprintf(stderr, "tag stacks are different lengths: %" PRIuPTR " != %" PRIuPTR "\n", t1c, t2c);
		exit(EXIT_FAILURE);
	}

	printf("Parts:\n");
	for (int i=0; i<t1c; i++)
		printf("\t(%.*s)\n", (int)(t2v[i]-t1v[i]), t1v[i]);

	mtag_stack_hist_reset(&tag_storage);
}

int main(int argc, char** argv)
{
	char*		str = "/foo/bar/x/quux/baz/asfd";

	mock_parse(str);

	return 0;
}


