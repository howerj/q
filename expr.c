/**@brief   Small Expression Evaluator for Q library
 * @license MIT
 * @author Richard James Howe
 * See: <https://en.wikipedia.org/wiki/Shunting-yard_algorithm> */

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "q.h"

#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))
#define DEFAULT_STACK_SIZE      (64)

enum { ASSOCIATE_NONE, ASSOCIATE_LEFT, ASSOCIATE_RIGHT };
enum { LEX_NUMBER, LEX_OPERATOR, LEX_END };

void expr_delete(qexpr_t *e) {
	if (!e)
		return;
	free(e->ops);
	free(e->numbers);
	for (size_t i = 0; i < e->vars_max; i++) {
		free(e->vars[i]->name);
		free(e->vars[i]);
	}
	free(e->vars);
	free(e);
}

qexpr_t *expr_new(size_t max) {
	max = max ? max : 64;
	qexpr_t *e = calloc(sizeof(*e), 1);
	if (!e)
		goto fail;
	e->ops     = calloc(sizeof(**e->ops), max);
	e->numbers = calloc(sizeof(*(e->numbers)), max);
	if (!(e->ops) || !(e->numbers))
		goto fail;
	e->ops_max     = max;
	e->numbers_max = max;
	qexpr_init(e);
	return e;
fail:
	expr_delete(e);
	return NULL;
}

static qvariable_t *variable_lookup(qexpr_t *e, const char *name) {
	assert(e);
	for (size_t i = 0; i < e->vars_max; i++) {
		qvariable_t *v = e->vars[i];
		if (!strcmp(v->name, name))
			return v;
	}
	return NULL;
}

static char *estrdup(const char *s) {
	assert(s);
	const size_t l = strlen(s) + 1;
	char *r = malloc(l);
	return memcpy(r, s, l);
}

static int variable_name_is_valid(const char *n) {
	assert(n);
	if (!isalpha(*n) && !(*n == '_'))
		return 0;
	for (n++; *n; n++)
		if (!isalnum(*n) && !(*n == '_'))
			return 0;
	return 1;
}

static qvariable_t *variable_add(qexpr_t *e, const char *name, q_t value) {
	assert(e);
	assert(name);
	qvariable_t *v = variable_lookup(e, name), **vs = e->vars;
	if (v) {
		v->value = value;
		return v;
	}
	if (!variable_name_is_valid(name))
		return NULL;
	char *s = estrdup(name);
	vs = realloc(e->vars, (e->vars_max + 1) * sizeof(*v));
	v = calloc(1, sizeof(*v));
	if (!vs || !v || !s)
		goto fail;
	v->name = s;
	v->value = value;
	vs[e->vars_max++] = v;
	e->vars = vs;
	return v;
fail:
	free(v);
	free(s);
	free(vs);
	return NULL;
}

static inline int tests(FILE *out) {
	assert(out);
	int report = 0;
	static const struct test {
		int r;
		q_t result;
		const char *expr;
	} tests[] = { // NB. Variables defined later.
		{  -1,  QINT( 0),   ""            },
		{  -1,  QINT( 0),   "("           },
		{  -1,  QINT( 0),   ")"           },
		{  -1,  QINT( 0),   "2**3"        },
		{   0,  QINT( 0),   "0"           },
		{   0,  QINT( 2),   "1+1"         },
		{   0, -QINT( 1),   "-1"          },
		{   0,  QINT( 1),   "--1"         },
		{   0,  QINT(14),   "2+(3*4)"     },
		{   0,  QINT(23),   "a+(b*5)"     },
		{  -1,  QINT(14),   "(2+(3* 4)"   },
		{  -1,  QINT(14),   "2+(3*4)("    },
		{   0,  QINT(14),   "2+3*4"       },
		{   0,  QINT( 0),   "  2==3 "     },
		{   0,  QINT( 1),   "2 ==2"       },
		{   0,  QINT( 1),   "2== (1+1)"   },
		//{   0,  QINT( 8),   "2 pow 3"     },
		//{  -1,  QINT( 0),   "2pow3"       },
		{   0,  QINT(20),   "(2+3)*4"     },
		{   0, -QINT( 4),   "(2+(-3))*4"  },
		{  -1,  QINT( 0),   "1/0"         },
		{  -1,  QINT( 0),   "1%0"         },
		{   0,  QINT(50),   "100/2"       },
		{   0,  QINT( 2),   "1--1",       },
		{   0,  QINT( 0),   "1---1",      },
	};

	fputs("Running Built In Self Tests:\n", out);
	const size_t length = sizeof (tests) / sizeof (tests[0]);
	for (size_t i = 0; i < length; i++) {
		qexpr_t *e = expr_new(64);
		const struct test *test = &tests[i];
		if (!e) {
			fprintf(out, "test failed (unable to allocate)\n");
			report = -1;
			goto end;
		}

		qvariable_t *v1 = variable_add(e, "a",  QINT(3));
		qvariable_t *v2 = variable_add(e, "b",  QINT(4));
		qvariable_t *v3 = variable_add(e, "c", -QINT(5));
		if (!v1 || !v2 || !v3) {
			fprintf(out, "test failed (unable to assign variable)\n");
			report = -1;
			goto end;
		}

		const int r = qexpr(e, test->expr);
		const q_t tos = e->numbers[0];
		const int pass = (r == test->r) && (r != 0 || tos == test->result);
		fprintf(out, "%s: r(%2d), eval(\"%s\") = %lg \n",
				pass ? "   ok" : " FAIL", r, test->expr, (double)tos);
		if (!pass) {
			report = -1;
			fprintf(out, "\tExpected: r(%2d), %lg\n",
				test->r, (double)(test->result));
		}
		expr_delete(e);
	}
end:
	fprintf(out, "Tests Complete: %s\n", report == 0 ? "pass" : "FAIL");
	return report;
}

static int usage(FILE *out, const char *arg0) {
	assert(out);
	assert(arg0);
	return fprintf(out, "usage: %s expr\n", arg0);
}

int main(int argc, char *argv[]) {
	int r = 0;
	qexpr_t *e = expr_new(0);

	if (!e) {
		fprintf(stderr, "allocate failed\n");
		r = 1;
		goto end;
	}

	if (argc == 1) {
		usage(stderr, argv[0]);
		return tests(stderr);
	}

	if (argc < 2) {
		fprintf(stderr, "usage: %s expr\n", argv[0]);
		r = 1;
		goto end;
	}

	if (qexpr(e, argv[1]) == 0) {
		char n[64] = { 0 };
		qsprint(e->numbers[0], n, sizeof n);
		printf("%s\n", n);
		r = 0;
		goto end;
	} else {
		fprintf(stderr, "error: %s\n", e->error_string);
	}
end:
	expr_delete(e);
	return r;
}

