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
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N    (16)
#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))

static int qprint(FILE *out, const q_t p) {
	assert(out);
	char buf[64+1] = { 0 };
	const int r = qsprint(p, buf, sizeof buf);
	return r < 0 ? r : fprintf(out, "%s", buf);
}

static void printq(FILE *out, q_t q, const char *msg) {
	assert(out);
	assert(msg);
	fprintf(out, "%s = ", msg);
	qprint(out, q);
	fputc('\n', out);
}

static void print_sincos(FILE *out, const q_t theta) {
	assert(out);
	q_t sine  = qinfo.zero, cosine = qinfo.zero;
	qsincos(theta, &sine, &cosine);
	qprint(out, theta);
	fputc(',', out);
	qprint(out, sine);
	fputc(',', out);
	qprint(out, cosine);
	fputc('\n', out);
}

static void print_sincos_table(FILE *out) {
	assert(out);
	const q_t tpi   = qdiv(qinfo.pi, qint(20));
	const q_t end   = qmul(qinfo.pi, qint(2));
	const q_t start = qnegate(end);
	fprintf(out, "theta,sine,cosine\n");
	for (q_t i = start; qless(i, end); i = qadd(i, tpi))
		print_sincos(out, i);
}

static void qinfo_print(FILE *out, const qinfo_t *qi) {
	assert(out);
	assert(qi);
	fprintf(out, "Q%u.%u Info\n", (unsigned)qi->whole, (unsigned)qi->fractional);
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
		[EVAL_ERROR_ARG_COUNT_E]         = "incorrect argument count",
		[EVAL_ERROR_LIMIT_MODE_E]        = "unknown limit mode ('|' or '%' allowed)",
		[EVAL_ERROR_UNEXPECTED_RESULT_E] = "unexpected result",
	};
	return msgs[e] ? msgs[e] : "unknown";
}

static int eval_unary_arith(q_t (*m)(q_t), q_t expected, q_t bound, q_t arg1, q_t *result) {
	assert(m);
	assert(result);
	const q_t r = m(arg1);
	*result = r;
	if (qwithin_interval(r, expected, bound))
		return 0;
	return -1;
}

static int eval_binary_arith(q_t (*f)(q_t, q_t), q_t expected, q_t bound, q_t arg1, q_t arg2, q_t *result) {
	assert(f);
	assert(result);
	const q_t r = f(arg1, arg2);
	*result = r;
	if (qwithin_interval(r, expected, bound))
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
	const qoperations_t *func = qop(operation);
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
	switch (func->arity) {
	case 1: if (eval_unary_arith(func->eval.unary, e, b, a1, result) < 0)
			return -EVAL_ERROR_UNEXPECTED_RESULT_E;
		break;
	case 2: if (qconv(&a2, arg2) < 0)
			return -EVAL_ERROR_CONVERT_E;
		if (eval_binary_arith(func->eval.binary, e, b, a1, a2, result) < 0)
			return -EVAL_ERROR_UNEXPECTED_RESULT_E;
		break;
	default:
		return -EVAL_ERROR_TYPE_E;
	}
	return EVAL_OK_E;
}

static void trim(char *s) {
	assert(s);
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

/* --- Unit Test Framework --- */

typedef struct {
	unsigned passed, 
		 run;
} unit_test_t;

static inline unit_test_t _unit_test_start(const char *file, const char *func, unsigned line) {
	assert(file);
	assert(func);
	unit_test_t t = { .run = 0, .passed = 0 };
	fprintf(stdout, "Start tests: %s in %s:%u\n\n", func, file, line);
	return t;
}

static inline void _unit_test_statement(const char *expr_str) {
	assert(expr_str);
	fprintf(stdout, "   STATE: %s\n", expr_str);
}

static inline void _unit_test(unit_test_t *t, int failed, const char *expr_str, const char *file, const char *func, unsigned line, int die) {
	assert(t);
	assert(expr_str);
	assert(file);
	assert(func);
	if(failed) {
		fprintf(stdout, "  FAILED: %s (%s:%s:%u)\n", expr_str, file, func, line);
		if(die) {
			fputs("VERIFY FAILED - EXITING\n", stdout);
			exit(EXIT_FAILURE);
		}
	} else {
		fprintf(stdout, "      OK: %s\n", expr_str);
		t->passed++;
	}
	t->run++;
}

static inline int unit_test_finish(unit_test_t *t) {
	assert(t);
	fprintf(stdout, "Tests passed/total: %u/%u\n", t->passed, t->run);
	if(t->run != t->passed) {
		fputs("[FAILED]\n", stdout);
		return -1;
	}
	fputs("[SUCCESS]\n", stdout);
	return 0;
}

#define unit_test_statement(T, EXPR) do { (void)(T); EXPR; _unit_test_statement(( #EXPR)); } while(0)
#define unit_test_start()         _unit_test_start(__FILE__, __func__, __LINE__)
#define unit_test(T, EXPR)        _unit_test((T), 0 == (EXPR), (# EXPR), __FILE__, __func__, __LINE__, 0)
#define unit_test_verify(T, EXPR) _unit_test((T), 0 == (EXPR), (# EXPR), __FILE__, __func__, __LINE__, 1)

static int test_sanity(void) {
	unit_test_t t = unit_test_start();
	q_t t1 = 0, t2 = 0;
	unit_test_statement(&t, t1 = qint(1));
	unit_test_statement(&t, t2 = qint(2));
	unit_test(&t, qtoi(qadd(t1, t2)) == 3);
	return unit_test_finish(&t);
}

static int test_pack(void) {
	unit_test_t t = unit_test_start();
	q_t p1 = 0, p2 = 0;
	char buf[sizeof(p1)] = { 0 };
	unit_test_statement(&t, p1 = qnegate(qinfo.pi));
	unit_test_statement(&t, p2 = 0);
	unit_test(&t, qunequal(p1, p2));
	unit_test(&t,  qpack(&p1, buf, sizeof p1 - 1) < 0);
	unit_test(&t,  qpack(&p1, buf, sizeof buf) == sizeof(p1));
	unit_test(&t, qunpack(&p2, buf, sizeof buf) == sizeof(p2));
	unit_test(&t, qequal(p1, p2));
	return unit_test_finish(&t);
}

static int test_fma(void) {
	unit_test_t t = unit_test_start();

	q_t a, b, c, r;
	const q_t one_and_a_half = qdiv(qint(3), qint(2));

	/* incorrect, but expected, result due to saturation */
	unit_test_statement(&t, a = qinfo.max);
	unit_test_statement(&t, b = one_and_a_half);
	unit_test_statement(&t, c = qinfo.min);
	unit_test_statement(&t, r = qadd(qmul(a, b), c));
	unit_test(&t, qwithin_interval(r, qint(0), qint(1)));

	/* correct result using Fused Multiply Add */
	unit_test_statement(&t, a = qinfo.max);
	unit_test_statement(&t, b = one_and_a_half);
	unit_test_statement(&t, c = qinfo.min);
	unit_test_statement(&t, r = qfma(a, b, c));
	unit_test(&t, qwithin_interval(r, qdiv(qinfo.max, qint(2)), qint(1)));

	return unit_test_finish(&t);
}

static inline int test_filter(void) {
	unit_test_t t = unit_test_start();
	qfilter_t lpf = { .raw = 0 }, hpf = { .raw = 0 };
	const q_t beta = qdiv(qint(1), qint(3));
	qfilter_init(&lpf, qint(0), beta, qint(0));
	qfilter_init(&hpf, qint(0), beta, qint(0));
	for (int i = 0; i < 100; i++) {
		char low[64+1] = { 0 }, high[64+1] = { 0 };
		const q_t step = qdiv(qint(i), qint(100));
		const q_t input = qint(1);
		qfilter_low_pass(&lpf, step, input);
		qfilter_high_pass(&hpf, step, input);
		qsprint(qfilter_value(&lpf),  low, sizeof(low)  - 1ull);
		qsprint(qfilter_value(&hpf), high, sizeof(high) - 1ull);
		fprintf(stdout, "%2d: %s\t%s\n", i, low, high);
	}
	return unit_test_finish(&t);
}

static int qmatrix_print(FILE *out, const q_t *m) {
	assert(out);
	assert(m);
	const size_t alloc = qmatrix_string_length(m);
	char *ms = malloc(alloc + 1);
	if (!ms)
		return -1;
	int r = qmatrix_sprintb(m, ms, alloc + 1, 10);
	if (r >= 0) 
		r = fprintf(out, "%s\n", ms);
	free(ms);
	return r;
}

#define QMATRIX(ROW, COLUMN, ...) { 0, ROW * COLUMN, ROW, COLUMN, __VA_ARGS__  }
#define QMATRIXZ(ROW, COLUMN)     QMATRIX((ROW), (COLUMN), 0)
#define QMATRIXSZ(ROW, COLUMN)    ((((ROW)*(COLUMN)) + 4)*sizeof(q_t))

static int test_matrix(void) {
	unit_test_t t = unit_test_start();
	FILE *out = stdout;
	q_t a[] = QMATRIX(2, 3, 
			QINT(1), QINT(2), QINT(3), 
			QINT(4), QINT(5), QINT(6)
	);
	q_t b[] = QMATRIX(3, 2, 
			QINT(2), QINT(3), 
			QINT(4), QINT(5), 
			QINT(6), QINT(7)
	);
	const q_t abr[] = QMATRIX(2, 2,
			QINT(28), QINT(34),
			QINT(64), QINT(79)
	);
	const q_t abrp[] = QMATRIX(2, 2,
			QINT(28), QINT(64),
			QINT(34), QINT(79)
	);
	q_t ab[QMATRIXSZ(2, 2)]   = QMATRIXZ(2, 2);
	q_t abp[QMATRIXSZ(2, 2)]  = QMATRIXZ(2, 2);
	unit_test_verify(&t, 0 == qmatrix_mul(ab, a, b));
	unit_test_verify(&t, 0 == qmatrix_transpose(abp, ab));
	unit_test(&t, qmatrix_equal(ab, abr));
	unit_test(&t, qmatrix_equal(ab, abrp));
	qmatrix_print(out, a);
	qmatrix_print(out, b);
	qmatrix_print(out, ab);
	qmatrix_print(out, abp);
	return unit_test_finish(&t);
}

static int test_matrix_trace(void) {
	unit_test_t t = unit_test_start();
	q_t a[] = QMATRIX(2, 2,
			QINT(1), QINT(2), 
			QINT(4), QINT(5), 
	);
	q_t b[] = QMATRIX(2, 2,
			QINT(2), QINT(3), 
			QINT(4), QINT(5), 
	);
	q_t ta[sizeof(a) / sizeof(a[0])] = QMATRIX(2, 2, 0);
	q_t tb[sizeof(b) / sizeof(b[0])] = QMATRIX(2, 2, 0);
	q_t apb[sizeof(a) / sizeof(a[0])] = QMATRIX(2, 2, 0);
	BUILD_BUG_ON(sizeof a != sizeof ta);
	BUILD_BUG_ON(sizeof a != sizeof tb);
	BUILD_BUG_ON(sizeof a != sizeof b);
	BUILD_BUG_ON(sizeof a != sizeof apb);
	unit_test_verify(&t, 0 == qmatrix_transpose(ta, a));
	unit_test_verify(&t, 0 == qmatrix_transpose(tb, b));
	unit_test_verify(&t, 0 == qmatrix_add(apb, a, b));
	unit_test(&t, qequal(qmatrix_trace(a), QINT(6)));
	unit_test(&t, qequal(qmatrix_trace(b), QINT(7)));
	unit_test(&t, qequal(qmatrix_trace(a), qmatrix_trace(ta)));
	unit_test(&t, qequal(qmatrix_trace(apb), qadd(qmatrix_trace(a), qmatrix_trace(b))));
	printq(stdout, qmatrix_determinant(a), "det(a)");
	return unit_test_finish(&t);
}

static q_t qid(q_t x) { return x; }
static q_t qsq(q_t x) { return qmul(x, x); }

static int test_simpson(void) {
	unit_test_t t = unit_test_start();
	unit_test(&t, qwithin_interval(qsimpson(qid, QINT(0), QINT(10), 100), QINT(50), QINT(1))); // (x^2)/2 + C
	unit_test(&t, qwithin_interval(qsimpson(qsq, qnegate(QINT(2)), QINT(5), 100), QINT(44), QINT(1))); // (x^3)/3 + C
	return unit_test_finish(&t);
}

static int internal_tests(void) {
	typedef int (*unit_test_t)(void);
	unit_test_t tests[] = {
		test_sanity,
		test_pack,
		test_fma,
		// test_filter,
		test_matrix,
		test_matrix_trace,
		test_simpson,
		NULL
	};
	for (size_t i = 0; tests[i]; i++)
		if (tests[i]() < 0)
			return -1;
	return 0;
}

static void help(FILE *out, const char *arg0) {
	assert(out);
	assert(arg0);
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

