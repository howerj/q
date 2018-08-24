#ifndef Q_H
#define Q_H

#include <stdint.h>
#include <stddef.h>

typedef uint32_t  u_t;
typedef  int32_t  d_t;
typedef uint64_t lu_t;
typedef  int64_t ld_t;

typedef struct {
	union { u_t u; d_t d; };
} q_t;

typedef enum {
	QBEHAVE_SATURATE_E,   /**< saturating arithmetic (default) */
	QBEHAVE_MODULO_E,     /**< modulo arithmetic */
	QBEHAVE_EXCEPTION_E,  /**< exception */
} qbehave_e; /**< high level q behavior, what happens on over/under flow? */

typedef struct {
	size_t whole,      /**< number of bits for whole, or integer, part of Q number */
	       fractional; /**< number of bits for fractional part of Q number */
	/**@todo add more constants: from <http://www.numberworld.org/constants.html>  */
	const q_t zero,  /**< the constant '0' */
	      bit,       /**< smallest 'q' number representable  */
	      one,       /**< the constant '1' */
	      pi,        /**< the constant 'pi' */
	      min,       /**< most negative 'q' number */
	      max;       /**< most positive 'q' number */
	qbehave_e behavior;
	/**@todo document type of division, rounding behavior, etcetera */
} qinfo_t; 

typedef d_t (*qbounds_t)(ld_t s);

d_t qbound_saturate(ld_t s);

typedef struct {
	qbounds_t bound; /**< handles saturation when a number over or underflows */
	int dp;          /**< decimal points to print, negative specifies maximum precision will allow */
} qconf_t; /**< global Q format configuration options */

extern const qinfo_t qinfo;
extern qconf_t qconf;

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


#endif
