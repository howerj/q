#include "q.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define HIGH (1ULL << (qinfo.fractional - 1))

typedef q_t (*function_mono_arith_t)(q_t a);
typedef q_t (*function_duo_arith_t)(q_t a, q_t b);
typedef int (*function_duo_compare_t)(q_t a, q_t b);

typedef enum {
	FUNCTION_UNARY_ARITHMETIC_E,
	FUNCTION_BINARY_ARITHMETIC_E,
	FUNCTION_BINARY_COMPARISON_E,
} function_e;

typedef struct {
	function_e type;
	union { function_duo_arith_t f; function_duo_compare_t c; function_mono_arith_t m; };
	char *name;
} function_t;

static const function_t functions[] = {
	{ .f = qadd,    .type = FUNCTION_BINARY_ARITHMETIC_E, .name = "+" },
	{ .f = qsub,    .type = FUNCTION_BINARY_ARITHMETIC_E, .name = "-" },
	{ .f = qmul,    .type = FUNCTION_BINARY_ARITHMETIC_E, .name = "*" },
	{ .f = qdiv,    .type = FUNCTION_BINARY_ARITHMETIC_E, .name = "/" },
	{ .f = qmin,    .type = FUNCTION_BINARY_ARITHMETIC_E, .name = "min" },
	{ .f = qmax,    .type = FUNCTION_BINARY_ARITHMETIC_E, .name = "max" },
	{ .m = qnegate, .type = FUNCTION_UNARY_ARITHMETIC_E,  .name = "negate" },
	{ .m = qtrunc,  .type = FUNCTION_UNARY_ARITHMETIC_E,  .name = "trunc" },
	{ .c = qmore,   .type = FUNCTION_BINARY_COMPARISON_E, .name = ">" },
	{ .c = qless,   .type = FUNCTION_BINARY_COMPARISON_E, .name = "<" },
	{ .c = qequal,  .type = FUNCTION_BINARY_COMPARISON_E, .name = "==" },
};

static const function_t *lookup(char *op) {
	assert(op);
	const size_t length = sizeof(functions) / sizeof(functions[0]);
	for(size_t i = 0; i < length; i++) {
		if(!strcmp(functions[i].name, op))
			return &functions[i];
	}
	return NULL;
}

int qprint(FILE *out, q_t p) {
	assert(out);
	char buf[64+1] = { 0 }; /**@todo find out max length */
	const int r = qsprint(p, buf, sizeof buf);
	return r < 0 ? r : fprintf(out, "%s", buf);
}

static void test_mono(FILE *out, char *op, q_t a) {
	const function_t *func = lookup(op);
	if(!func || func->type != FUNCTION_UNARY_ARITHMETIC_E)
		return;
	const function_mono_arith_t m = func->m;
	fprintf(out, "%s(", op);
	qprint(out, a); 
	fprintf(out, ") = ");
	qprint(out, m(a)); 
	fputc('\n', out);
}

static void test_duo(FILE *out, char *op, q_t a, q_t b) {
	const function_t *func = lookup(op);
	if(!func || func->type != FUNCTION_BINARY_ARITHMETIC_E)
		return;
	const function_duo_arith_t f = func->f;
	qprint(out, a); 
	fprintf(out, " %s ", op);
	qprint(out, b); 
	fprintf(out, " = ");
	qprint(out, f(a, b)); 
	fputc('\n', out);
}

static void test_comp(FILE *out, char *op, q_t a, q_t b) {
	const function_t *func = lookup(op);
	if(!func || func->type != FUNCTION_BINARY_COMPARISON_E)
		return;
	const function_duo_compare_t c = func->c;
	qprint(out, a); 
	fprintf(out, " %s ", op);
	qprint(out, b); 
	fprintf(out, " = %s", c(a, b) ? "true" : "false");
	fputc('\n', out);
}

static void printq(FILE *out, q_t q, const char *msg) {
	fprintf(out, "%s = ", msg);
	qprint(out, q);
	fputc('\n', out);
}


int main(void) {
	/**@todo more static assertions relating to min/max numbers, BITS and
	 * MASK 
	 * @todo Turn this into a proper test bench, where results are
	 * asserted as being correct */

	FILE *out = stdout;
	q_t p1 = qmk( 3, qinfo.fractional );
       	q_t p2 = qmk(-3, HIGH/5);
	q_t p3 = qint(2);
	test_duo(out, "+", p1, p2); /**@todo replace '+' with lookup based on function name */
	test_duo(out, "+", p1, p1);
	test_duo(out, "-", p1, p2);
	test_duo(out, "-", p1, p1);
	test_duo(out, "*", p1, p2);
	test_duo(out, "*", p1, p1);
	test_duo(out, "/", p1, p2);
	test_duo(out, "/", p1, p1);
	test_duo(out, "/", p1, p3);
	test_duo(out, "/", p2, p3);

	test_mono(out, "negate", qinfo.pi);
	test_mono(out, "negate", p2);
	test_mono(out, "trunc",  p1);
	test_mono(out, "trunc",  p2);
	test_mono(out, "trunc",  qint(2));
	test_mono(out, "trunc",  qint(-2));

	test_comp(out, "==", p1, p1);
	test_comp(out, "==", p1, p2);

	test_comp(out, ">", p1, p2);
	test_comp(out, ">", p2, p1);
	test_comp(out, ">", p1, p1);

	test_comp(out, "<", p1, p2);
	test_comp(out, "<", p2, p1);
	test_comp(out, "<", p1, p1);

	printq(out, qinfo.bit,  "bit");
	printq(out, qinfo.one,  "one");
	printq(out, qinfo.zero, "zero");
	printq(out, qinfo.pi,   "pi");
	printq(out, qinfo.min,  "min");
	printq(out, qinfo.max,  "max");

	test_duo(out, "+", qinfo.max, qinfo.one);
	test_duo(out, "+", qinfo.max, qinfo.max);
	test_duo(out, "-", qinfo.min, qinfo.one);

	/**@todo test different saturation handlers */

	q_t conv = qinfo.zero;
	qconv(&conv, "-12.34"); /**@todo fix this */
	qprint(out, conv);
	fputc('\n', out);

	test_duo(out, "/", qmk(2, (2*HIGH)/3), qmk(0, HIGH));

	/**@todo interactive calculator */

	return 0;
}

