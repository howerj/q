/* Project: Q-Number (Q16.16, signed) library
 * Author:  Richard James Howe
 * License: The Unlicense
 * Email:   howe.r.j.89@gmail.com
 * Repo:    <https://github.com/q> */

#ifndef Q_H
#define Q_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#define QMAX_ID     (32)
#define QMAX_ERROR  (256)

#define QBITS       (16)
#define QMASK       ((1ULL <<  QBITS) - 1ULL)
#define QHIGH       (1ULL << (QBITS  - 1ULL))

#define QMK(QHIGH, LOW, SF) ((ld_t)((((lu_t)QHIGH) << QBITS) | (QMASK & ((((lu_t)LOW) << QBITS) >> (SF)))))
#define QINT(INT)           ((q_t)((u_t)(INT) << QBITS))
#define QPI                 (QMK(0x3, 0x243F, 16))

#ifndef PREPACK
#define PREPACK
#endif

#ifndef POSTPACK
#define POSTPACK
#endif

#ifndef RESTRICT
#ifdef __cplusplus
#define RESTRICT
#else
#define RESTRICT restrict
#endif
#endif

typedef int32_t  q_t; /* Q Fixed Point Number, (Q16.16, Signed) */
typedef int64_t ld_t; /* Double width of Q, signed, for internal calculations, not in Q format */
typedef int32_t  d_t; /* same width as Q,  signed, but not in Q format */
typedef uint32_t u_t; /* same width as Q, unsigned, but not in Q format */

typedef PREPACK struct {
	size_t whole,      /* number of bits for whole, or integer, part of Q number */
	       fractional; /* number of bits for fractional part of Q number */
	const q_t zero,    /* the constant '0' */
	      bit,         /* smallest 'q' number representable  */
	      one,         /* the constant '1' */
	      pi,          /* the constant 'pi' */
	      e,           /* the constant 'e' */
	      sqrt2,       /* the square root of 2 */
	      sqrt3,       /* the square root of 3 */
	      ln2,         /* the natural logarithm of 2 */
	      ln10,        /* the natural logarithm of 10 */
	      min,         /* most negative 'q' number */
	      max;         /* most positive 'q' number */
	const uint32_t version; /* version in X.Y.Z format (Z = lowest 8 bits) */
} POSTPACK qinfo_t;

typedef PREPACK struct {
	q_t rc,        /* time constant */
	    time,      /* time of previous measurement */
	    raw,       /* previous raw value */
	    filtered;  /* filtered value */
} POSTPACK qfilter_t;  /* High/Low Pass Filter */

typedef PREPACK struct {
	q_t d_gain, d_state;               /* differentiator; gain, state */
	q_t i_gain, i_state, i_min, i_max; /* integrator; gain, state, minimum and maximum */
	q_t p_gain;                        /* proportional gain */
} POSTPACK qpid_t; /* PID Controller <https://en.wikipedia.org/wiki/PID_controller> */

struct qexpr;
typedef struct qexpr qexpr_t;

typedef PREPACK struct {
	char *name;
	union {
		q_t (*unary)  (q_t a);
		q_t (*binary) (q_t a1, q_t a2);
	} eval;
	union {
		q_t (*unary)  (qexpr_t *e, q_t a);
		q_t (*binary) (qexpr_t *e, q_t a1, q_t a2);
	} check;
	int precedence, arity, assocativity, hidden;
} POSTPACK qoperations_t; /* use in the expression evaluator */

typedef PREPACK struct {
	char *name;
	q_t value;
} POSTPACK qvariable_t; /* Variable which can be used with the expression evaluator */

struct PREPACK qexpr {
	const qoperations_t **ops, *lpar, *rpar, *negate, *minus;
	qvariable_t **vars;
	char id[QMAX_ID];
	char error_string[QMAX_ERROR];
	q_t number;
	const qoperations_t *op;
	q_t *numbers;
	size_t ops_count, ops_max;
	size_t numbers_count, numbers_max;
	size_t id_count;
	size_t vars_max;
	int error;
	int initialized;
} POSTPACK; /* An expression evaluator for the Q library */

typedef q_t (*qbounds_t)(ld_t s);

q_t qbound_saturate(ld_t s); /* default over/underflow behavior, saturation */
q_t qbound_wrap(ld_t s);     /* over/underflow behavior, wrap around */

typedef PREPACK struct {
	qbounds_t bound; /* handles saturation when a number over or underflows */
	int dp;          /* decimal points to print, negative specifies maximum precision */
	unsigned base;   /* base to use for numeric number conversion */
} POSTPACK qconf_t; /* Q format configuration options */

extern const qinfo_t qinfo; /* information about the format and constants */
extern qconf_t qconf;       /* @warning GLOBAL Q configuration options */

int qtoi(q_t toi);
q_t qint(int toq);
signed char qtoc(const q_t q);
q_t qchar(signed char c);
short qtoh(const q_t q);
q_t qshort(short s);
long qtol(const q_t q);
q_t qlong(long l);
long long qtoll(const q_t q);
q_t qvlong(long long ll);

q_t qisnegative(q_t a);
q_t qispositive(q_t a);
q_t qisinteger(q_t a);
q_t qisodd(q_t a);
q_t qiseven(q_t a);

q_t qless(q_t a, q_t b);
q_t qmore(q_t a, q_t b);
q_t qeqless(q_t a, q_t b);
q_t qeqmore(q_t a, q_t b);
q_t qequal(q_t a, q_t b);
q_t qunequal(q_t a, q_t b);
q_t qapproxequal(q_t a, q_t b, q_t epsilon);
q_t qapproxunequal(q_t a, q_t b, q_t epsilon);
q_t qwithin(q_t v, q_t b1, q_t b2);
q_t qwithin_interval(q_t v, q_t expected, q_t allowance);

q_t qnegate(q_t a);
q_t qmin(q_t a, q_t b);
q_t qmax(q_t a, q_t b);
q_t qabs(q_t a);
q_t qcopysign(q_t a, q_t b);
q_t qsign(q_t a);
q_t qsignum(q_t a);

q_t qadd(q_t a, q_t b);
q_t qsub(q_t a, q_t b);
q_t qmul(q_t a, q_t b);
q_t qdiv(q_t a, q_t b);
q_t qrem(q_t a, q_t b);
q_t qmod(q_t a, q_t b);
q_t qfma(q_t a, q_t b, q_t c);
q_t qsqr(q_t x);
q_t qexp(q_t e);
q_t qlog(q_t n);
q_t qsqrt(q_t x);

q_t qround(q_t q);
q_t qceil(q_t q);
q_t qtrunc(q_t q);
q_t qfloor(q_t q);

q_t qand(q_t a, q_t b);
q_t qxor(q_t a, q_t b);
q_t qor(q_t a, q_t b);
q_t qinvert(q_t a);
q_t qnot(q_t a);
q_t qlogical(q_t a);

q_t qlls(q_t a, q_t b);
q_t qlrs(q_t a, q_t b);
q_t qals(q_t a, q_t b);
q_t qars(q_t a, q_t b);

q_t qpow(q_t n, q_t exp);

int qsprint(q_t p, char *s, size_t length);
int qsprintb(q_t p, char *s, size_t length, u_t base);
int qsprintbdp(q_t p, char *s, size_t length, u_t base, d_t idp);
int qnconv(q_t *q, const char *s, size_t length);
int qnconvb(q_t *q, const char *s, size_t length, d_t base);
int qconv(q_t *q, const char *s);
int qconvb(q_t *q, const char * const s, d_t base);
int qnconvbdp(q_t *q, const char *s, size_t length, d_t base, u_t idp);

void qsincos(q_t theta, q_t *sine, q_t *cosine);
q_t qsin(q_t theta);
q_t qcos(q_t theta);
q_t qtan(q_t theta);
q_t qcot(q_t theta);
q_t qhypot(q_t a, q_t b);

q_t qatan(q_t t);
q_t qatan2(q_t x, q_t y);
q_t qasin(q_t t);
q_t qacos(q_t t);
q_t qsinh(q_t a);
q_t qcosh(q_t a);
q_t qtanh(q_t a);

q_t qatanh(q_t t);
q_t qasinh(q_t t);
q_t qacosh(q_t t);

void qsincosh(q_t a, q_t *sinh, q_t *cosh);

q_t qcordic_ln(q_t d);         /* CORDIC testing only */
q_t qcordic_exp(q_t e);        /* CORDIC testing only; useless for large values */
q_t qcordic_sqrt(q_t a);       /* CORDIC testing only; do not use, a <= 2, a >= 0 */
q_t qcordic_mul(q_t a, q_t b); /* CORDIC testing only; do not use */
q_t qcordic_div(q_t a, q_t b); /* CORDIC testing only; do not use */
q_t qcordic_circular_gain(int n);
q_t qcordic_hyperbolic_gain(int n);

void qpol2rec(q_t magnitude, q_t theta, q_t *i, q_t *j);
void qrec2pol(q_t i, q_t j, q_t *magnitude, q_t *theta);

q_t qrad2deg(q_t rad);
q_t qdeg2rad(q_t deg);

d_t dpower(d_t b, unsigned e);
d_t dlog(d_t n, unsigned base);
d_t arshift(d_t v, unsigned p);
int qpack(const q_t *q, char *buffer, size_t length);
int qunpack(q_t *q, const char *buffer, size_t length);

q_t qsimpson(q_t (*f)(q_t), q_t x1, q_t x2, unsigned n); /* numerical integrator of f, between x1, x2, for n steps */

void qfilter_init(qfilter_t *f, q_t time, q_t rc, q_t seed);
q_t qfilter_low_pass(qfilter_t *f, q_t time, q_t data);
q_t qfilter_high_pass(qfilter_t *f, q_t time, q_t data);
q_t qfilter_value(const qfilter_t *f);

q_t qpid_update(qpid_t *pid, const q_t error, const q_t position);

/* A matrix consists of at least four elements, a meta data field, 
 * the length of the array (which must be big enough to store 
 * row*column, but may be * larger) and a row and a column count 
 * in unsigned integer format, and the array elements in Q format. 
 * This simplifies storage and declaration of matrices.
 *
 * An example, the 2x3 matrix:
 *
 * 	[ 1, 2, 3; 4, 5, 6 ]
 *
 * Should be defined as:
 *
 * 	q_t m[] = { 0, 2*3, 2, 3, QINT(1), QINT(2), QINT(3), QINT(4), QINT(5), QINT(6) };
 *
 */
int qmatrix_apply_unary(q_t *r, const q_t *a, q_t (*func)(q_t));
int qmatrix_apply_scalar(q_t *r, const q_t *a, q_t (*func)(q_t, q_t), const q_t c);
int qmatrix_apply_binary(q_t * RESTRICT r, const q_t *a, const q_t *b, q_t (*func)(q_t, q_t));
int qmatrix_sprintb(const q_t *m, char *str, size_t length, unsigned base);
int qmatrix_resize(q_t *m, const size_t row, const size_t column);
int qmatrix_copy(q_t *r, const q_t *a);
size_t qmatrix_string_length(const q_t *m);

q_t qmatrix_trace(const q_t *m);
q_t qmatrix_determinant(const q_t *m);
q_t qmatrix_equal(const q_t *a, const q_t *b);

int qmatrix_zero(q_t *r);
int qmatrix_one(q_t *r);
int qmatrix_identity(q_t *r); /* turn into identity matrix, r must be square */

int qmatrix_logical(q_t *r, const q_t *a);
int qmatrix_not(q_t *r, const q_t *a);
int qmatrix_signum(q_t *r, const q_t *a);
int qmatrix_invert(q_t *r, const q_t *a);

int qmatrix_is_valid(const q_t *m);

int qmatrix_transpose(q_t * RESTRICT r, const q_t * RESTRICT m);
int qmatrix_add(q_t * RESTRICT r, const q_t *a, const q_t *b);
int qmatrix_sub(q_t * RESTRICT r, const q_t *a, const q_t *b);
int qmatrix_mul(q_t * RESTRICT r, const q_t *a, const q_t *b);
int qmatrix_and(q_t *r, const q_t *a, const q_t *b);
int qmatrix_or (q_t *r, const q_t *a, const q_t *b);
int qmatrix_xor(q_t *r, const q_t *a, const q_t *b);

int qmatrix_scalar_add(q_t *r, const q_t *a, const q_t scalar);
int qmatrix_scalar_sub(q_t *r, const q_t *a, const q_t scalar);
int qmatrix_scalar_mul(q_t *r, const q_t *a, const q_t scalar);
int qmatrix_scalar_div(q_t *r, const q_t *a, const q_t scalar);
int qmatrix_scalar_mod(q_t *r, const q_t *a, const q_t scalar);
int qmatrix_scalar_rem(q_t *r, const q_t *a, const q_t scalar);
int qmatrix_scalar_and(q_t *r, const q_t *a, const q_t scalar);
int qmatrix_scalar_or (q_t *r, const q_t *a, const q_t scalar);
int qmatrix_scalar_xor(q_t *r, const q_t *a, const q_t scalar);

/* Expression evaluator */

int qexpr(qexpr_t *e, const char *expr);
int qexpr_init(qexpr_t *e);
int qexpr_error(qexpr_t *e);
q_t qexpr_result(qexpr_t *e);
const qoperations_t *qop(const char *op);

/* A better cosine/sine, not in Q format */

int16_t furman_sin(int16_t x); /* SINE:   1 Furman = 1/65536 of a circle */
int16_t furman_cos(int16_t x); /* COSINE: 1 Furman = 1/65536 of a circle */

#ifdef __cplusplus
}
#endif
#endif
