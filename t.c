#include "q.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>

#define HIGH (1ULL << (qinfo.fractional - 1))
#define N    (16)

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
	int arity;
	union { function_duo_arith_t f; function_duo_compare_t c; function_mono_arith_t m; };
	char *name;
} function_t;

static const function_t *lookup(char *op) {
	assert(op);
	static const function_t functions[] = {
		{ .f = qadd,    .arity = 2, .type = FUNCTION_BINARY_ARITHMETIC_E, .name = "+" },
		{ .f = qsub,    .arity = 2, .type = FUNCTION_BINARY_ARITHMETIC_E, .name = "-" },
		{ .f = qmul,    .arity = 2, .type = FUNCTION_BINARY_ARITHMETIC_E, .name = "*" },
		{ .f = qdiv,    .arity = 2, .type = FUNCTION_BINARY_ARITHMETIC_E, .name = "/" },
		{ .f = qmin,    .arity = 2, .type = FUNCTION_BINARY_ARITHMETIC_E, .name = "min" },
		{ .f = qmax,    .arity = 2, .type = FUNCTION_BINARY_ARITHMETIC_E, .name = "max" },
		{ .m = qnegate, .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E,  .name = "negate" },
		{ .m = qtrunc,  .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E,  .name = "trunc" },
		{ .m = qround,  .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E,  .name = "round" },
		{ .m = qabs,    .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E,  .name = "abs" },
		{ .m = qsin,    .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E,  .name = "sin" },
		{ .m = qcos,    .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E,  .name = "cos" },
		{ .c = qmore,   .arity = 2, .type = FUNCTION_BINARY_COMPARISON_E, .name = ">" },
		{ .c = qless,   .arity = 2, .type = FUNCTION_BINARY_COMPARISON_E, .name = "<" },
		{ .c = qequal,  .arity = 2, .type = FUNCTION_BINARY_COMPARISON_E, .name = "==" },
	};
	const size_t length = sizeof(functions) / sizeof(functions[0]);
	for(size_t i = 0; i < length; i++) {
		if(!strcmp(functions[i].name, op))
			return &functions[i];
	}
	return NULL;
}

static int qprint(FILE *out, q_t p) {
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

static void printsc(FILE *out, q_t theta) {
	q_t sine  = qinfo.zero, cosine = qinfo.zero;
	qcordic(theta, 16, &sine, &cosine);
	qprint(out, theta);
	fprintf(out, ",");
	qprint(out, sine);
	fprintf(out, ",");
	qprint(out, cosine);
	fprintf(out, "\n");
}

static void qinfo_print(FILE *out, const qinfo_t *qi) {
	fprintf(out, "Q Info\n");
	printq(out, qi->bit,   "bit");
	printq(out, qi->one,   "one");
	printq(out, qi->zero,  "zero");
	printq(out, qi->pi,    "pi");
	printq(out, qi->e,     "e");
	printq(out, qi->sqrt2, "sqrt2");
	printq(out, qi->sqrt3, "sqrt3");
	printq(out, qi->ln2,   "ln2");
	printq(out, qi->ln10,  "ln10");
	printq(out, qi->min,   "min");
	printq(out, qi->max,   "max");
}

static FILE *fopen_or_die(const char *file, const char *mode) {
	assert(file && mode);
	FILE *h = NULL;
	errno = 0;
	if(!(h = fopen(file, mode))) {
		fprintf(stderr, "file open %s (mode %s) failed: %s", file, mode, strerror(errno));
		exit(EXIT_FAILURE);
	}
	return h;
}

typedef enum {
	EVAL_OK_E,
	EVAL_ERROR_SCAN_E,
	EVAL_ERROR_TYPE_E,
	EVAL_ERROR_CONVERT_E,
	EVAL_ERROR_OPERATION_E,
	EVAL_ERROR_ARG_COUNT_E,
	EVAL_ERROR_UNEXPECTED_RESULT_E,

	EVAL_ERROR_MAX_ERRORS_E, /**< not an error, but a count of errors */
} eval_errors_e;

static const char *eval_error(int e) {
	if(e < 0 || e >= EVAL_ERROR_MAX_ERRORS_E)
		return "unknown";
	const char *msgs[EVAL_ERROR_MAX_ERRORS_E] = {
		[EVAL_OK_E]              = "ok?",
		[EVAL_ERROR_SCAN_E]      = "invalid input line",
		[EVAL_ERROR_TYPE_E]      = "unknown function type",
		[EVAL_ERROR_CONVERT_E]   = "numeric conversion failed",
		[EVAL_ERROR_OPERATION_E] = "unknown operation",
		[EVAL_ERROR_ARG_COUNT_E] = "too few arguments",
		[EVAL_ERROR_UNEXPECTED_RESULT_E] = "unexpected result",
	};
	return msgs[e] ? msgs[e] : "unknown";
}

static q_t eval_binary_arith(function_duo_arith_t f, q_t expected, q_t bound, q_t arg1, q_t arg2, q_t *result)
{
	assert(f);
	assert(result);
	const q_t r = f(arg1, arg2);
	*result = r;
	if(qequal(r, expected))
		return 0;
	return -1;
}

static int eval(char *line, q_t *result) {
	assert(line);
	assert(result);
	*result = qinfo.zero;
	char operation[N] = { 0 }, expected[N] = { 0 }, arg1[N] = { 0 }, arg2[N] = { 0 };
	const int count = sscanf(line, "%15s %15s %15s %15s", operation, expected, arg1, arg2);
	if(count < 3)
		return -EVAL_ERROR_SCAN_E;
	const function_t *func = lookup(operation);
	if(!func)
		return -EVAL_ERROR_OPERATION_E;
	if(func->type != FUNCTION_BINARY_ARITHMETIC_E) /**@todo implement other function types */
		return EVAL_ERROR_TYPE_E;
	q_t e = 0, a1 = 0, a2 = 0;
	const int argc = count - 2;
	if(func->arity != argc)
		return -EVAL_ERROR_ARG_COUNT_E;
	if(qconv(&e, expected) < 0)
		return -EVAL_ERROR_CONVERT_E;
	if(qconv(&a1, arg1) < 0)
		return -EVAL_ERROR_CONVERT_E;
	if(qconv(&a2, arg2) < 0)
		return -EVAL_ERROR_CONVERT_E;
	if(eval_binary_arith(func->f, e, qinfo.zero, a1, a2, result) < 0)
		return -EVAL_ERROR_UNEXPECTED_RESULT_E;
	return EVAL_OK_E;
}

void trim(char *s) {
	const int size = strlen(s);
	for(int i = size - 1; i >= 0; i--) {
		if(!isspace(s[i]))
			break;
		s[i] = 0;
	}
}

int main(int argc, char **argv) {
	/**@todo proper argument processing; help, info, generate sine table */
	if(argc > 2) {
		fprintf(stderr, "usage: %s file\n", argv[0]);
		return -1;
	}
	FILE *input = argc == 2 ? fopen_or_die(argv[1], "rb") : stdin;
	FILE *output = stdout;
	char line[256] = { 0 };
	//qinfo_print(output, &qinfo);
	while(fgets(line, sizeof(line) - 1, input)) {
		q_t result = 0;
		const int r = eval(line, &result);
		if(r < 0) {
			const char *msg = eval_error(-r);
			trim(line);
			fprintf(output, "error: eval(\"%s\") = %d: %s\n", line, r, msg);
			fprintf(output, "\tresult = ");
			qprint(output, result);
			fputc('\n', output);
			return -1;
		}
		fprintf(output, "ok: %s", line);
		memset(line, 0, sizeof line);
	}
	fclose(input);
	return 0;

	FILE *out = stdout;
	q_t p1 = qmk( 3, HIGH/5 );
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

	qinfo_print(out, &qinfo);

	test_duo(out, "+", qinfo.max, qinfo.one);
	test_duo(out, "+", qinfo.max, qinfo.max);
	test_duo(out, "-", qinfo.min, qinfo.one);

	/**@todo test different saturation handlers */

	test_duo(out, "/", qmk(2, (2*HIGH)/3), qmk(0, HIGH));
	q_t conv = qinfo.zero;
	qconv(&conv, "-12.06259");
	qprint(out, conv);
	fputc('\n', out);

	const q_t tpi   = qdiv(qinfo.pi, qint(20));
	const q_t start = qmul(qinfo.pi, qint(2));
	for(q_t i = qnegate(start); qless(i, start); i = qadd(i, tpi)) {
		printsc(out, i);
	}

	q_t theta = qinfo.zero;
	q_t sine  = qinfo.zero, cosine = qinfo.zero;
	qcordic(theta, 16, &sine, &cosine);

	qconf.bound = qbound_wrap;
	test_duo(out, "+", qinfo.max, qinfo.one);
	test_duo(out, "+", qinfo.max, qinfo.max);
	test_duo(out, "-", qinfo.min, qinfo.one);
	qconf.bound = qbound_saturate;

	/**@todo interactive calculator, or interactive test bench, with
	 * a format of:
	 *
	 * 	operation values expected +/-bound
	 * 	operation values integer
	 *
	 * */

	return 0;
}

