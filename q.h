/**@file q.h
 * @brief Q-Number (Q16.16, signed) library
 * @copyright Richard James Howe (2018)
 * @license MIT
 * @email howe.r.j.89@gmail.com
 * @site <https://github.com/howerj/q> */

#ifndef Q_H
#define Q_H

#include <stdint.h>
#include <stddef.h>

typedef int32_t q_t;  /**< Q Fixed Point Number, (Q16.16, Signed) */
typedef int64_t ld_t; /**< */

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
} qconf_t; /**< Q format configuration options */

extern const qinfo_t qinfo; /**< information about the format and constants */
extern qconf_t qconf;       /**< @warning GLOBAL Q configuration options */

int qnegative(q_t a);
int qless(q_t a, q_t b);
int qmore(q_t a, q_t b);
int qequal(q_t a, q_t b);
int qtoi(q_t toi);
q_t qint(int toq);
q_t qnegate(q_t a);
q_t qmin(q_t a, q_t b);
q_t qmax(q_t a, q_t b);
q_t qabs(q_t a);
q_t qmk(int integer, unsigned fractional);
q_t qadd(q_t a, q_t b);
q_t qsub(q_t a, q_t b);
q_t qmul(q_t a, q_t b);
q_t qdiv(q_t a, q_t b);
q_t qround(q_t q);
q_t qtrunc(q_t q);

int qsprint(q_t p, char *s, size_t length);
int qnconv(q_t *q, char *s, size_t length);
int qconv(q_t *q, char *s);
int qcordic(q_t theta, int iterations, q_t *sine, q_t *cosine);
void qsincos(q_t theta, q_t *sine, q_t *cosine);
q_t qsin(q_t theta);
q_t qcos(q_t theta);

#endif
