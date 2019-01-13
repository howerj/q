/**@file q.h
 * @brief Q-Number (Q16.16, signed) library
 * @copyright Richard James Howe (2018)
 * @license MIT
 * @email howe.r.j.89@gmail.com
 * @site <https://github.com/howerj/q> */

#ifndef Q_H
#define Q_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

typedef int32_t q_t;  /**< Q Fixed Point Number, (Q16.16, Signed) */
typedef int64_t ld_t; /**< Double width of Q, signed, for internal calculations, not in Q format */
typedef int32_t  d_t; /* same width as Q,  signed, but not in Q format */

typedef struct {
	size_t whole,      /**< number of bits for whole, or integer, part of Q number */
	       fractional; /**< number of bits for fractional part of Q number */
	const q_t zero,  /**< the constant '0' */
	      bit,       /**< smallest 'q' number representable  */
	      one,       /**< the constant '1' */
	      pi,        /**< the constant 'pi' */
	      e,         /**< the constant 'e' */
	      sqrt2,     /**< the square root of 2 */
	      sqrt3,     /**< the square root of 3 */
	      ln2,       /**< the natural logarithm of 2 */
	      ln10,      /**< the natural logarithm of 10 */
	      min,       /**< most negative 'q' number */
	      max;       /**< most positive 'q' number */
	/**@todo document type of division, rounding behavior, etcetera */
} qinfo_t;

typedef q_t (*qbounds_t)(ld_t s);

q_t qbound_saturate(ld_t s); /**< default over/underflow behavior, saturation */
q_t qbound_wrap(ld_t s);     /**< over/underflow behavior, wrap around */

typedef struct {
	qbounds_t bound; /**< handles saturation when a number over or underflows */
	int dp;          /**< decimal points to print, negative specifies maximum precision will allow */
	unsigned base;   /**< base to use for numeric number conversion */
} qconf_t; /**< Q format configuration options */

extern const qinfo_t qinfo; /**< information about the format and constants */
extern qconf_t qconf;       /**< @warning GLOBAL Q configuration options */

int qisnegative(q_t a);
int qispositive(q_t a);
int qisinteger(q_t a);
int qisodd(q_t a);
int qiseven(q_t a);

int qless(q_t a, q_t b);
int qmore(q_t a, q_t b);
int qeqless(q_t a, q_t b);
int qeqmore(q_t a, q_t b);
int qequal(q_t a, q_t b);
int qunequal(q_t a, q_t b);

int qtoi(q_t toi);
q_t qint(int toq);

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
q_t qfma(q_t a, q_t b, q_t c);
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
q_t qlls(q_t a, q_t b);
q_t qlrs(q_t a, q_t b);
q_t qals(q_t a, q_t b);
q_t qars(q_t a, q_t b);

/**@todo refactor/simplify these functions, there does not need to be so many of them */
int qsprint(q_t p, char *s, size_t length);
int qsprintb(q_t p, char *s, size_t length, unsigned long base);
int qnconv(q_t *q, const char *s, size_t length);
int qnconvb(q_t *q, const char *s, size_t length, long base);
int qconv(q_t *q, const char *s);
int qconvb(q_t *q, const char * const s, long base);

void qsincos(q_t theta, q_t *sine, q_t *cosine);
q_t qsin(q_t theta);
q_t qcos(q_t theta);
q_t qtan(q_t theta);
q_t qcot(q_t theta);
q_t qhypot(q_t a, q_t b);

//q_t qatan(q_t t);
//q_t qasin(q_t t);
//q_t qacos(q_t t);
q_t qsinh(q_t a);
q_t qcosh(q_t a);
q_t qtanh(q_t a);
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

#ifdef __cplusplus
}
#endif
#endif
