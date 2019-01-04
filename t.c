/**@file t.c
 * @brief Q-Number (Q16.16, signed) library test bench
 * @copyright Richard James Howe (2018)
 * @license MIT
 * @email howe.r.j.89@gmail.com
 * @site <https://github.com/howerj/q>
 *
 * This file contains a wrapper for testing the Q Number library,
 * it includes a command processor and some built in tests. View
 * the help string later on for more information */
#include "q.h"
#include "u.h"
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HIGH (1ULL << (qinfo.fractional - 1))
#define N    (16)

typedef q_t (*function_unary_arith_t)(q_t a);
typedef int (*function_unary_propery_t)(q_t a);
typedef q_t (*function_binary_arith_t)(q_t a, q_t b);
typedef int (*function_binary_compare_t)(q_t a, q_t b);

typedef enum {
	FUNCTION_UNARY_ARITHMETIC_E,
	FUNCTION_UNARY_PROPERY_E,
	FUNCTION_BINARY_ARITHMETIC_E,
	FUNCTION_BINARY_COMPARISON_E,
} function_e;

typedef struct {
	function_e type;
	int arity;
	union {
		function_binary_arith_t f;
		function_binary_compare_t c;
		function_unary_arith_t m;
		function_unary_propery_t p;
	} op;
	char *name;
} function_t;

static const function_t *lookup(char *op) {
	assert(op);
	static const function_t functions[] = {
		{ .op.f = qadd,    .arity = 2, .type = FUNCTION_BINARY_ARITHMETIC_E, .name = "+" },
		{ .op.f = qsub,    .arity = 2, .type = FUNCTION_BINARY_ARITHMETIC_E, .name = "-" },
		{ .op.f = qmul,    .arity = 2, .type = FUNCTION_BINARY_ARITHMETIC_E, .name = "*" },
		{ .op.f = qcordic_mul, .arity = 2, .type = FUNCTION_BINARY_ARITHMETIC_E, .name = "c*" },
		{ .op.f = qcordic_div, .arity = 2, .type = FUNCTION_BINARY_ARITHMETIC_E, .name = "c/" },
		{ .op.f = qdiv,    .arity = 2, .type = FUNCTION_BINARY_ARITHMETIC_E, .name = "/" },
		{ .op.f = qrem,    .arity = 2, .type = FUNCTION_BINARY_ARITHMETIC_E, .name = "rem" },
		{ .op.f = qmin,    .arity = 2, .type = FUNCTION_BINARY_ARITHMETIC_E, .name = "min" },
		{ .op.f = qmax,    .arity = 2, .type = FUNCTION_BINARY_ARITHMETIC_E, .name = "max" },
		{ .op.f = qcopysign,.arity = 2, .type = FUNCTION_BINARY_ARITHMETIC_E, .name = "copysign" },
		{ .op.f = qand,    .arity = 2, .type = FUNCTION_BINARY_ARITHMETIC_E, .name = "and" },
		{ .op.f = qor,     .arity = 2, .type = FUNCTION_BINARY_ARITHMETIC_E, .name = "or" },
		{ .op.f = qxor,    .arity = 2, .type = FUNCTION_BINARY_ARITHMETIC_E, .name = "xor" },
		{ .op.f = qars,    .arity = 2, .type = FUNCTION_BINARY_ARITHMETIC_E, .name = "arshift" },
		{ .op.f = qlrs,    .arity = 2, .type = FUNCTION_BINARY_ARITHMETIC_E, .name = "rshift" },
		{ .op.f = qals,    .arity = 2, .type = FUNCTION_BINARY_ARITHMETIC_E, .name = "alshift" },
		{ .op.f = qlls,    .arity = 2, .type = FUNCTION_BINARY_ARITHMETIC_E, .name = "lshift" },
		{ .op.f = qhypot,  .arity = 2, .type = FUNCTION_BINARY_ARITHMETIC_E, .name = "hypot" },
		//{ .op.f = qpow,  .arity = 2, .type = FUNCTION_BINARY_ARITHMETIC_E, .name = "pow" },

		{ .op.m = qnegate, .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E,  .name = "negate" },
		{ .op.m = qtrunc,  .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E,  .name = "trunc" },
		{ .op.m = qround,  .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E,  .name = "round" },
		{ .op.m = qfloor,  .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E,  .name = "floor" },
		{ .op.m = qceil,   .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E,  .name = "ceil" },
		{ .op.m = qabs,    .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E,  .name = "abs" },
		{ .op.m = qsin,    .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E,  .name = "sin" },
		{ .op.m = qcos,    .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E,  .name = "cos" },
		{ .op.m = qexp,    .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E,  .name = "exp" },
		{ .op.m = qsqrt,   .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E,  .name = "sqrt" },
		{ .op.m = qsign,   .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E,  .name = "sign" },
		{ .op.m = qsignum, .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E,  .name = "signum" },
		{ .op.m = qcordic_exp, .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E,  .name = "_exp" },

		{ .op.m = qnegate, .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E,  .name = "negate" },
		{ .op.m = qtrunc,  .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E,  .name = "trunc" },
		{ .op.m = qround,  .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E,  .name = "round" },
		{ .op.m = qabs,    .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E,  .name = "abs" },
		{ .op.m = qsin,    .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E,  .name = "sin" },
		{ .op.m = qcos,    .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E,  .name = "cos" },
		{ .op.m = qtan,    .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E,  .name = "tan" },
		{ .op.m = qatan,   .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E,  .name = "atan" },
		{ .op.m = qtanh,   .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E,  .name = "tanh" },
		{ .op.m = qsinh,   .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E,  .name = "sinh" },
		{ .op.m = qcosh,   .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E,  .name = "cosh" },
		{ .op.m = qcot,    .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E,  .name = "cot" },
		{ .op.m = qcordic_sqrt, .arity = 1,.type = FUNCTION_UNARY_ARITHMETIC_E, .name = "_sqrt" },
		{ .op.m = qcordic_ln, .arity = 1,.type = FUNCTION_UNARY_ARITHMETIC_E, .name = "_ln" },
		{ .op.m = qlog,    .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E, .name = "log" },
		{ .op.m = qdeg2rad, .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E, .name = "deg2rad" },
		{ .op.m = qrad2deg, .arity = 1, .type = FUNCTION_UNARY_ARITHMETIC_E, .name = "rad2deg" },

		{ .op.p = qisinteger,  .arity = 1, .type = FUNCTION_UNARY_PROPERY_E,  .name = "int?" },
		{ .op.p = qisnegative, .arity = 1, .type = FUNCTION_UNARY_PROPERY_E,  .name = "neg?" },
		{ .op.p = qispositive, .arity = 1, .type = FUNCTION_UNARY_PROPERY_E,  .name = "pos?" },
		{ .op.p = qisodd,      .arity = 1, .type = FUNCTION_UNARY_PROPERY_E,  .name = "odd?" },
		{ .op.p = qiseven,     .arity = 1, .type = FUNCTION_UNARY_PROPERY_E,  .name = "even?" },

		{ .op.c = qmore,   .arity = 2, .type = FUNCTION_BINARY_COMPARISON_E, .name = ">" },
		{ .op.c = qless,   .arity = 2, .type = FUNCTION_BINARY_COMPARISON_E, .name = "<" },
		{ .op.c = qequal,  .arity = 2, .type = FUNCTION_BINARY_COMPARISON_E, .name = "=" },
		{ .op.c = qeqmore, .arity = 2, .type = FUNCTION_BINARY_COMPARISON_E, .name = ">=" },
		{ .op.c = qeqless, .arity = 2, .type = FUNCTION_BINARY_COMPARISON_E, .name = "<=" },
		{ .op.c = qunequal,.arity = 2, .type = FUNCTION_BINARY_COMPARISON_E, .name = "<>" },

	};
	const size_t length = sizeof(functions) / sizeof(functions[0]);
	for (size_t i = 0; i < length; i++) {
		if (!strcmp(functions[i].name, op))
			return &functions[i];
	}
	return NULL;
}

static int qprint(FILE *out, q_t p) {
	assert(out);
	char buf[64+1] = { 0 };
	const int r = qsprint(p, buf, sizeof buf);
	return r < 0 ? r : fprintf(out, "%s", buf);
}

static void printq(FILE *out, q_t q, const char *msg) {
	fprintf(out, "%s = ", msg);
	qprint(out, q);
	fputc('\n', out);
}

static void print_sincos(FILE *out, q_t theta) {
	assert(out);
	q_t sine  = qinfo.zero, cosine = qinfo.zero;
	qsincos(theta, &sine, &cosine);
	qprint(out, theta);
	fprintf(out, ",");
	qprint(out, sine);
	fprintf(out, ",");
	qprint(out, cosine);
	fprintf(out, "\n");
}

static void print_sincos_table(FILE *out) {
	assert(out);
	const q_t tpi   = qdiv(qinfo.pi, qint(20));
	const q_t end   = qmul(qinfo.pi, qint(2));
	const q_t start = qnegate(end);
	fprintf(out, "theta,sine,cosine\n");
	for (q_t i = start; qless(i, end); i = qadd(i, tpi)) {
		print_sincos(out, i);
	}
}

static void qinfo_print(FILE *out, const qinfo_t *qi) {
	assert(out);
	assert(qi);
	fprintf(out, "Q%zu.%zu Info\n", qi->whole, qi->fractional);
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
	printq(out, qcordic_circular_gain(-1),   "circular-gain");
	printq(out, qcordic_hyperbolic_gain(-1), "hyperbolic-gain");
}

static void qconf_print(FILE *out, const qconf_t *qc) {
	assert(out);
	assert(qc);
	fprintf(out, "Q Configuration\n");
	const char *bounds = "unknown";
	qbounds_t bound = qc->bound;
	if (bound == qbound_saturate)
		bounds = "saturate";
	if (bound == qbound_wrap)
		bounds = "wrap";
	fprintf(out, "overflow handler: %s\n", bounds);
	fprintf(out, "input/output radix: %u (0 = special case)\n", qc->base);
	fprintf(out, "decimal places: %d\n", qc->dp);
}

static FILE *fopen_or_die(const char *file, const char *mode) {
	assert(file && mode);
	FILE *h = NULL;
	errno = 0;
	if (!(h = fopen(file, mode))) {
		fprintf(stderr, "file open \"%s\" (mode %s) failed: %s\n", file, mode, strerror(errno));
		exit(EXIT_FAILURE);
	}
	return h;
}

typedef enum {
	EVAL_OK_E,
	EVAL_COMMENT_E,
	EVAL_ERROR_SCAN_E,
	EVAL_ERROR_TYPE_E,
	EVAL_ERROR_CONVERT_E,
	EVAL_ERROR_OPERATION_E,
	EVAL_ERROR_ARG_COUNT_E,
	EVAL_ERROR_UNEXPECTED_RESULT_E,
	EVAL_ERROR_LIMIT_MODE_E,

	EVAL_ERROR_MAX_ERRORS_E, /**< not an error, but a count of errors */
} eval_errors_e;

static const char *eval_error(int e) {
	if (e < 0 || e >= EVAL_ERROR_MAX_ERRORS_E)
		return "unknown";
	const char *msgs[EVAL_ERROR_MAX_ERRORS_E] = {
		[EVAL_OK_E]                      = "ok",
		[EVAL_COMMENT_E]                 = "(comment)",
		[EVAL_ERROR_SCAN_E]              = "invalid input line",
		[EVAL_ERROR_TYPE_E]              = "unknown function type",
		[EVAL_ERROR_CONVERT_E]           = "numeric conversion failed",
		[EVAL_ERROR_OPERATION_E]         = "unknown operation",
		[EVAL_ERROR_ARG_COUNT_E]         = "too few arguments",
		[EVAL_ERROR_LIMIT_MODE_E]        = "unknown limit mode ('|' or '%' allowed)",
		[EVAL_ERROR_UNEXPECTED_RESULT_E] = "unexpected result",
	};
	return msgs[e] ? msgs[e] : "unknown";
}

static int qwithin(q_t v, q_t b1, q_t b2) {
	const q_t hi = qmax(b1, b2);
	const q_t lo = qmin(b1, b2);
	if (qequal(v, b1) || qequal(v, b2))
		return 1;
	return qless(v, hi) && qmore(v, lo) ? 1 : 0;
}

static int qwithin_bounds(q_t v, q_t expected, q_t allowance) {
	const q_t b1 = qadd(expected, allowance);
	const q_t b2 = qsub(expected, allowance);
	return qwithin(v, b1, b2);
}

static int eval_unary_property(function_unary_propery_t p, q_t expected, q_t bound, q_t arg1, q_t *result) {
	assert(p);
	assert(result);
	const int r = p(arg1);
	*result = qint(r);
	if (qwithin_bounds(qint(r), expected, bound))
		return 0;
	return -1;
}

static int eval_unary_arith(function_unary_arith_t m, q_t expected, q_t bound, q_t arg1, q_t *result) {
	assert(m);
	assert(result);
	const q_t r = m(arg1);
	*result = r;
	if (qwithin_bounds(r, expected, bound))
		return 0;
	return -1;
}

static int eval_binary_comp(function_binary_compare_t c, q_t expected, q_t arg1, q_t arg2, q_t *result) {
	assert(c);
	assert(result);
	const int r = c(arg1, arg2);
	*result = qint(r);
	if (r == qtoi(expected))
		return 0;
	return -1;
}

static int eval_binary_arith(function_binary_arith_t f, q_t expected, q_t bound, q_t arg1, q_t arg2, q_t *result) {
	assert(f);
	assert(result);
	const q_t r = f(arg1, arg2);
	*result = r;
	if (qwithin_bounds(r, expected, bound))
		return 0;
	return -1;
}

static int comment(char *line) {
	assert(line);
	const size_t length = strlen(line);
	for (size_t i = 0; i < length; i++) {
		if (line[i] == '#')
			return 1;
		if (!isspace(line[i]))
			break;
	}
	for (size_t i = 0; i < length; i++) {
		if (line[i] == '#') {
			line[i] = 0;
			return 0;
		}
	}
	return 0;
}

static void bounds_set(char method) {
	assert(method == '|' || method == '%');
	if (method == '|')
		qconf.bound = qbound_saturate;
	qconf.bound = qbound_wrap;
}

static int eval(char *line, q_t *result) {
	assert(line);
	assert(result);
	*result = qinfo.zero;
	if (comment(line))
		return EVAL_COMMENT_E;
	char operation[N] = { 0 }, expected[N] = { 0 }, bounds[N], arg1[N] = { 0 }, arg2[N] = { 0 };
	char limit = '|';
	const int count = sscanf(line, "%15s %15s +- %15s %c %15s %15s", operation, expected, bounds, &limit, arg1, arg2);
	if (limit != '|' && limit != '%')
		return -EVAL_ERROR_LIMIT_MODE_E;
	bounds_set(limit);
	if (count < 5)
		return -EVAL_ERROR_SCAN_E;
	const function_t *func = lookup(operation);
	if (!func)
		return -EVAL_ERROR_OPERATION_E;
	q_t e = qinfo.zero, b = qinfo.zero, a1 = qinfo.zero, a2 = qinfo.zero;
	const int argc = count - 4;
	if (func->arity != argc)
		return -EVAL_ERROR_ARG_COUNT_E;
	if (qconv(&e, expected) < 0)
		return -EVAL_ERROR_CONVERT_E;
	if (qconv(&b, bounds) < 0)
		return -EVAL_ERROR_CONVERT_E;
	if (qconv(&a1, arg1) < 0)
		return -EVAL_ERROR_CONVERT_E;
	switch (func->type) {
	case FUNCTION_UNARY_PROPERY_E:
		if (eval_unary_property(func->op.p, e, b, a1, result) < 0)
			return -EVAL_ERROR_UNEXPECTED_RESULT_E;
		break;
	case FUNCTION_UNARY_ARITHMETIC_E:
		if (eval_unary_arith(func->op.m, e, b, a1, result) < 0)
			return -EVAL_ERROR_UNEXPECTED_RESULT_E;
		break;
	case FUNCTION_BINARY_ARITHMETIC_E:
		if (qconv(&a2, arg2) < 0)
			return -EVAL_ERROR_CONVERT_E;
		if (eval_binary_arith(func->op.f, e, b, a1, a2, result) < 0)
			return -EVAL_ERROR_UNEXPECTED_RESULT_E;
		break;
	case FUNCTION_BINARY_COMPARISON_E:
		if (qconv(&a2, arg2) < 0)
			return -EVAL_ERROR_CONVERT_E;
		if (eval_binary_comp(func->op.f, e, a1, a2, result) < 0)
			return -EVAL_ERROR_UNEXPECTED_RESULT_E;
		break;
	default:
		return -EVAL_ERROR_TYPE_E;
	}
	return EVAL_OK_E;
}

static void trim(char *s) {
	const int size = strlen(s);
	for (int i = size - 1; i >= 0; i--) {
		if (!isspace(s[i]))
			break;
		s[i] = 0;
	}
}

static int eval_file(FILE *input, FILE *output) {
	assert(input);
	assert(output);
	char line[256] = { 0 };
	while (fgets(line, sizeof(line) - 1, input)) {
		q_t result = 0;
		const int r = eval(line, &result);
		if (r == EVAL_COMMENT_E)
			continue;
		if (r < 0) {
			const char *msg = eval_error(-r);
			trim(line);
			fprintf(output, "error: eval(\"%s\") = %d: %s\n", line, r, msg);
			fprintf(output, "\tresult = ");
			qprint(output, result);
			fputc('\n', output);
			return -1;
		}
		trim(line);
		char rstring[64+1] = { 0 };
		qsprint(result, rstring, sizeof(rstring) - 1);
		fprintf(output, "ok: %s | (%s)\n", line, rstring);
		memset(line, 0, sizeof line);
	}
	return 0;
}

/*static void polprn(FILE *out) {
	q_t i = 0, j = 0, mag = 0, theta = 0;
	for (i = -qint(1); qless(i, qint(2)); i = qadd(i, qint(1)))
		for (j = -qint(1); qless(j, qint(2)); j = qadd(j, qint(1))) {
			q_t in = 0, jn = 0;
			qrec2pol(i, j, &mag, &theta);
			printq(out, i,     "i");
			printq(out, j,     "j");
			printq(out, mag,   "mag");
			printq(out, qrad2deg(theta), "theta");
			qpol2rec(mag, theta, &in, &jn);
			printq(out, in,     "in");
			printq(out, jn,     "jn");

			fputc('\n', out);
		}
}*/


static int test_sanity(void) {
	unit_test_start();

	q_t t1 = 0, t2 = 0;
	unit_test_statement(t1 = qint(1));
	unit_test_statement(t2 = qint(2));
	unit_test(qtoi(qadd(t1, t2)) == 3);

	return unit_test_finish();
}

static int test_pack(void) {
	unit_test_start();
	q_t p1 = 0, p2 = 0;
	char buf[sizeof(p1)] = { 0 };
	unit_test_statement(p1 = qnegate(qinfo.pi));
	unit_test_statement(p2 = 0);
	unit_test(qunequal(p1, p2));
	unit_test(  qpack(&p1, buf, sizeof p1 - 1) < 0);
	unit_test(  qpack(&p1, buf, sizeof buf) == sizeof(p1));
	unit_test(qunpack(&p2, buf, sizeof buf) == sizeof(p2));
	unit_test(qequal(p1, p2));
	return unit_test_finish();
}

static int test_fma(void) {
	unit_test_start();

	q_t a, b, c, r;
	const q_t one_and_a_half = qdiv(qint(3), qint(2));

	/* incorrect, but expected, result due to saturation */
	unit_test_statement(a = qinfo.max);
	unit_test_statement(b = one_and_a_half);
	unit_test_statement(c = qinfo.min);
	unit_test_statement(r = qadd(qmul(a, b), c));
	unit_test(qwithin_bounds(r, qint(0), qint(1)));

	/* correct result using Fused Multiply Add */
	unit_test_statement(a = qinfo.max);
	unit_test_statement(b = one_and_a_half);
	unit_test_statement(c = qinfo.min);
	unit_test_statement(r = qfma(a, b, c));
	unit_test(qwithin_bounds(r, qdiv(qinfo.max, qint(2)), qint(1)));

	return unit_test_finish();
}

static int internal_tests(void) { /**@todo add more tests */
	typedef int (*unit_test_t)(void);
	unit_test_t tests[] = {
		test_sanity,
		test_pack,
		test_fma,
		NULL
	};
	for (size_t i = 0; tests[i]; i++)
		if (tests[i]() < 0)
			return -1;


	return 0;
}

static void help(FILE *out, const char *arg0) {
	const char *h = "\n\
Program: Q-Number (Q16.16, signed) library testbench\n\
Author:  Richard James Howe (2018)\n\
License: MIT\n\
E-mail:  howe.r.j.89@gmail.com\n\
Site:    https://github.com/howerj/q\n\n\
Options:\n\
\t-s\tprint a sine-cosine table\n\
\t-h\tprint this help message and exit\n\
\t-i\tprint library information\n\
\t-t\trun internal unit tests\n\
\t-c\tturn color on for unit tests\n\
\t-v\tprint version information and exit\n\
\tfile\texecute commands in file\n\n\
This program wraps up a Q-Number library and allows it to be tested by\n\
giving it commands, either from stdin, or from a file. The format is\n\
strict and the parser primitive, but it is only meant to be used to\n\
test that the library is doing the correct thing and not as a\n\
calculator.\n\n\
Commands evaluated consist of an operator with the require arguments\n\
(either one or two arguments) and compare the result with an expected\n\
value, which can within a specified bounds for the test to pass. If\n\
the test fails the program exits, indicating failure. The format is:\n\
\n\
\toperator expected +- allowance | arg1 arg2\n\n\
operators include '+', '-', '/', 'rem', '<', which require two\n\
arguments, and unary operators like 'sin', and 'negate', which require\n\
only one. 'expected', 'allowance', 'arg1' and 'arg2' are all fixed\n\
numbers of the form '-12.45'. 'expected' is the expected result,\n\
'allowance' the +/- amount the result is allowed to deviated by, and\n\
'arg1' and 'arg2' the operator arguments.\n\
\n\n";
	fprintf(out, "usage: %s -h -s -i -v -t -c file\n", arg0);
	fputs(h, out);
}

int main(int argc, char **argv) {
	bool ran = false;
	char *on = getenv("COLOR");
	unit_color_on = (on && !strcmp(on, "on"));

	for (int i = 1; i < argc; i++) {
		if (!strcmp("-h", argv[i])) {
			help(stdout, argv[0]);
			return 0;
		} else if (!strcmp("-s", argv[i])) {
			print_sincos_table(stdout);
			ran = true;
		} else if (!strcmp("-v", argv[i])) {
			fprintf(stdout, "version 1.0\n");
			return 0;
		} else if (!strcmp("-c", argv[i])) {
			unit_color_on = true;
		} else if (!strcmp("-t", argv[i])) {
			if (internal_tests() < 0)
				return -1;
			ran = true;
		} else if (!strcmp("-i", argv[i])) {
			qinfo_print(stdout, &qinfo);
			qconf_print(stdout, &qconf);
			ran = true;
		} else {
			FILE *input = fopen_or_die(argv[i], "rb");
			FILE *output = stdout;
			const int r = eval_file(input, output);
			ran = true;
			fclose(input);
			if (r < 0)
				return -1;
		}
	}
	if (!ran)
		return eval_file(stdin, stdout);
	return 0;

	/*
	qconf.bound = qbound_wrap;
	test_duo(out, "+", qinfo.max, qinfo.one);
	test_duo(out, "+", qinfo.max, qinfo.max);
	test_duo(out, "-", qinfo.min, qinfo.one);
	qconf.bound = qbound_saturate;
	
	return 0;*/
}

