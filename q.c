/**@file q.c
 * @brief Q-Number (Q16.16, signed) library
 * @copyright Richard James Howe (2018)
 * @license MIT
 * @email howe.r.j.89@gmail.com
 * @site <https://github.com/howerj/q>
 *
 *  TODO:
 * 	- basic mathematical functions (arcsin, arccosine, hyperbolic
 * 	functions, ..., ln, log, pow, ...)
 * 	- add modulo *and* remainder
 * 	  - <https://news.ycombinator.com/item?id=17817758>
 * 	  - <https://rob.conery.io/2018/08/21/mod-and-remainder-are-not-the-same/>
 * 	- unit tests, built in ones
 * 	- optional conversion to floats (compile time switch)
 * 	- remove dependency on stdio entirely
 * NOTES:
 * 	- <https://en.wikipedia.org/wiki/Q_%28number_format%29>
 * 	- <https://www.mathworks.com/help/fixedpoint/examples/calculate-fixed-point-sine-and-cosine.html>
 * 	- <http://www.coranac.com/2009/07/sines/>
 * 	- <https://en.wikipedia.org/wiki/CORDIC>
 *	- */

#include "q.h"
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>
/* NB. No 'FILE*' or used in the library, only the sscanf/sprint functions. */
#include <stdio.h>

#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))

typedef  int16_t hd_t; /* half Q width,      signed */
typedef uint32_t  u_t; /* same width as Q, unsigned, but not in Q format */
typedef  int32_t  d_t; /* same width as Q,   signed, but not in Q format */
typedef uint64_t lu_t; /* double Q width,  unsigned */

#define BITS  (16)
#define MASK  ((1ULL <<  BITS) - 1ULL)
#define HIGH   (1ULL << (BITS  - 1ULL))

#define UMAX  (UINT32_MAX)
#define DMIN  (INT32_MIN)
#define DMAX  (INT32_MAX)
#define LUMAX (UINT64_MAX)
#define LDMIN (INT64_MIN)
#define LDMAX (INT64_MAX)

const qinfo_t qinfo = {
	.whole      = BITS,
	.fractional = BITS,
	.zero = (u_t)0uL << BITS,
	.bit  = 1uL,
	.one  = (u_t)1uL << BITS,
	.min  = (u_t)(HIGH << BITS),
	.max  = (u_t)((HIGH << BITS) - 1uL),

	/**@warning these constants are dependent on the bit width due to how they are defined */
	.pi    = 0x3243FuL /* 3.243F6 A8885 A308D 31319 8A2E0... */,
	.e     = 0x2B7E1uL /* 2.B7E1 5162 8A... */ ,
	.sqrt2 = 0x16a09uL /* 1.6A09 E667 F3... */ ,
	.sqrt3 = 0x1BB67uL /* 1.BB67 AE85 84... */ ,
	.ln2   = 0x0B172uL /* 0.B172 17F7 D1... */ ,
	.ln10  = 0x24d76uL /* 2.4D76 3776 AA... */ ,
};

qconf_t qconf = { /**@warning global configuration options */
	.bound = qbound_saturate,
	.dp    = 5,
};

q_t qbound_saturate(ld_t s) { /**< default saturation handler */
	assert(s > DMAX || s < DMIN);
	if(s > DMAX) return DMAX;
	return DMIN;
}

q_t qbound_wrap(ld_t s) { /**< wrap numbers */
	assert(s > DMAX || s < DMIN);
	if(s > DMAX) return DMIN + (s % DMAX);
	return DMAX - ((-s) % DMAX);
}

/*q_t qbound_exception(ld_t s) { (void)(s); exit(EXIT_FAILURE); }*/

static inline q_t qsat(ld_t s) {
	/*@todo more static assertions relating to min/max numbers, BITS and
	 * MASK */
	BUILD_BUG_ON( sizeof(q_t) !=  sizeof(u_t));
	BUILD_BUG_ON( sizeof(u_t) !=  sizeof(d_t));
	BUILD_BUG_ON(sizeof(lu_t) !=  sizeof(ld_t));
	BUILD_BUG_ON(sizeof(d_t)  != (sizeof(hd_t) * 2));
	BUILD_BUG_ON(sizeof(lu_t) != (sizeof(u_t)  * 2));

	if(s > DMAX) return qconf.bound(s);
	if(s < DMIN) return qconf.bound(s);
	return s;
}

static inline d_t arshift(d_t v, unsigned p) {
	u_t vn = v;
	if(v >= 0)
		return vn >> p;
	const u_t mask = ((u_t)(-1L)) << ((sizeof(v)*CHAR_BIT) - p);
	return mask | (vn >> p);
}

static inline u_t qhigh(const q_t q) { return ((u_t)q) >> BITS; }
static inline u_t qlow(const q_t q)  { return ((u_t)q) & MASK; }
static inline q_t qcons(const u_t hi, const u_t lo) { return (hi << BITS) | (lo & MASK); }

int qnegative(q_t a)     { return !!(qhigh(a) & HIGH); }
int qless(q_t a, q_t b)  { return a < b; }
int qmore(q_t a, q_t b)  { return a > b; }
int qequal(q_t a, q_t b) { return a == b; }
int qtoi(q_t toi)        { return ((u_t)toi) >> BITS; /**@bug sizeof(int) > sizeof(q_t) */ }
q_t qint(int toq)        { return ((u_t)((d_t)toq)) << BITS; }
q_t qnegate(q_t a)       { return (~(u_t)a) + 1ULL; }
q_t qmin(q_t a, q_t b)   { return qless(a, b) ? a : b; }
q_t qmax(q_t a, q_t b)   { return qmore(a, b) ? a : b; }
q_t qabs(q_t a)          { return qnegative(a) ? qnegate(a) : a; }
q_t qadd(q_t a, q_t b)   { return qsat((ld_t)a + (ld_t)b); }
q_t qsub(q_t a, q_t b)   { return qsat((ld_t)a - (ld_t)b); }

q_t qtrunc(q_t q) {
	const q_t adj = qnegative(q) && qlow(q) ? qinfo.one : qinfo.zero;
	q = qadd(q, adj);
	return ((u_t)q) & (MASK << BITS);
}

q_t qmk(int integer, unsigned fractional) {
	const int negative = integer < 0;
	integer = negative ? -integer : integer;
	const q_t r = qcons((d_t)integer, fractional);
	return negative ? qnegate(r) : r;
}

q_t qmul(q_t a, q_t b) {
	const ld_t dd = ((ld_t)a * (ld_t)b) + (lu_t)HIGH;
	/* NB. portable version of "dd >> BITS" */
	const lu_t ud = dd < 0 ? (-1ULL << (2*BITS)) | ((lu_t)dd >> BITS) : ((lu_t)dd) >> BITS;
	return qsat(ud);
}

q_t qdiv(q_t a, q_t b) {
	ld_t dd = ((ld_t)a) << BITS;
	const d_t bd2 = b / 2;
	if((dd >= 0 && b >= 0) || (dd < 0 && b < 0)) {
		dd += bd2;
	} else {
		dd -= bd2;
	}
	return dd / b;
}

/*q_t qfloor(q_t q)
{
}

q_t qceil(q_t q)
{
}

q_t qmod(q_t a, q_t b)
{
}

q_t qremainder(q_t a, q_t b)
{
}
*/

q_t qround(q_t q) {
	const int negative = qnegative(q);
	const int round    = !!( qlow(q) & HIGH);
	return qsat(((ld_t)q) + (negative ? -round : round));
}

/* <https://codereview.stackexchange.com/questions/109212> */
int qsprint(q_t p, char *s, size_t length) { /**@todo different bases */
	assert(s);
	const int negative = qnegative(p);
	if(negative)
		p = qnegate(p);
	const u_t base = 10;
	const d_t hi = qhigh(p);
	char frac[BITS+1] = { 0 };
	u_t lo = qlow(p);
	for(size_t i = 0; lo; i++) {
		if(qconf.dp >= 0 && (int)i >= qconf.dp) /**@todo proper rounding*/
			break;
		lo *= base;
		frac[i] = '0' + (lo >> BITS);
		lo &= MASK;
	}
	return snprintf(s, length, "%s%ld.%s", negative ? "-" : "", (long)hi, frac);
}

/**@bug all allowed formats are not implemented
 *
 * Allowed example numbers; 2.0 -3 +0x5.Af -0377.77 1 0
 *
 */
int qnconv(q_t *q, char *s, size_t length) {
	assert(q);
	assert(s);
	int r = 0;
	long ld = 0, base = 10;
	char frac[BITS+1] = { 0 };
	memset(q, 0, sizeof *q);
	/**@todo replace scanf, and snprintf with own number conversion functions */
	(void)length; /* snscanf? */
	if(sscanf(s, "%ld.%16s", &ld, frac) != 2)
		return -1;
	if(ld > DMAX || ld < DMIN)
		r = -ERANGE;
	d_t hi = ld;
	u_t lo = 0, i = 0, j = 1;
	for(i = 0; frac[i]; i++) {
		/* NB. Could eek out more precision by rounding next digit */
		if(i > 5) /*max 5 decimal digits in a 16-bit number: trunc((ln(MAX-VAL+1)/ln(base)) + 1) */
			break;
		const int ch = frac[i] - '0';
		if(ch < 0)
			return -1;
		lo = (lo * base) + ch;
		j *= base;
	}
	lo = ((lo << BITS) / j);
	*q = qmk(hi, lo);
	return r;
}

int qconv(q_t *q, char *s) {
	assert(s);
	return qnconv(q, s, strlen(s));
}

/* See: <https://dspguru.com/dsp/faqs/cordic/> */
int qcordic(q_t theta, int iterations, q_t *sine, q_t *cosine) {
	assert(sine);
	assert(cosine);
	/**@todo improve precision, 18-bit lookup table? */
	static const u_t lookup[] = { /* atan(2^0), atan(2^-1), atan(2^-2), ... */
		0x3243uL, 0x1DACuL, 0x0FADuL, 0x07F5uL,
		0x03FEuL, 0x01FFuL, 0x00FFuL, 0x007FuL,
		0x003FuL, 0x001FuL, 0x000FuL, 0x0007uL,
		0x0003uL, 0x0001uL, 0x0000uL, 0x0000uL,
	};
	const size_t length = sizeof lookup / sizeof lookup[0];
	const d_t iscaling = 0x26DD; /* 1/scaling-factor */

	iterations = iterations > (int)length ? (int)length : iterations;
	iterations = iterations < 0           ? (int)length : iterations;

	/**@todo make these internal static constants */
	const q_t   pi = qinfo.pi;
	const q_t  npi = qnegate(qinfo.pi);
	const q_t  qpi = qdiv(pi, qint(4));
	const q_t qnpi = qnegate(qpi);
	const q_t  dpi = qmul(pi, qint(2));
	const q_t dnpi = qnegate(dpi);

	int negate = 0, shift = 0;

	/* convert to range -pi   to pi */
	while(qless(theta, npi)) theta = qadd(theta,  dpi);
	while(qmore(theta,  pi)) theta = qadd(theta, dnpi);

	/* convert to range -pi/2 to pi/2 */
	const q_t  hpi = qdiv(pi, qint(2));
	const q_t hnpi = qnegate(hpi);
	if(qless(theta, hnpi)) {
		theta = qadd(theta,  pi);
		negate = 1;
	} else if(qmore(theta, hpi)) {
		theta = qadd(theta, npi);
		negate = 1;
	}

	/* convert to range -pi/4 to pi/4 */
	if(qless(theta, qnpi)) {
		theta = qadd(theta,  hpi);
		shift = -1;
	} else if(qmore(theta, qpi)) {
		theta = qadd(theta, hnpi);
		shift =  1;
	}

	/* convert to internal scaling for CORDIC routine (not q16.16!) */

	/**@todo better scaling */
	d_t x = iscaling, y = 0, z = arshift(theta, 2);

	for(size_t i = 0; i < (unsigned)iterations; i++) {
		const d_t  d =   -!!(z < 0);
		const d_t xn = x - ((arshift(y, i) ^ d) - d);
		const d_t yn = y + ((arshift(x, i) ^ d) - d);
		const d_t zn = z - ((lookup[i] ^ d) - d);
		x = xn; /* cosine */
		y = yn; /* sine   */
		z = zn;
	}
	/* correct overflow in cosine */
	//if(x < 0) x = -x;

	/* shift */
	if(shift > 0) {
		const d_t yt = y;
		y =  x;
		x = -yt;
	} else if(shift < 0) {
		const d_t yt = y;
		y = -x;
		x =  yt;
	}
	/* quadrant */
	if(negate) {
		x = -x;
		y = -y;
	}
	/* rescale and set output */
	/* @todo better scaling */
	*cosine = x << 2;
	  *sine = y << 2;
	return 0;
}

void qsincos(q_t theta, q_t *sine, q_t *cosine) {
	const int r = qcordic(theta, -1, sine, cosine);
	assert(r == 0);
}

q_t qsin(q_t theta) {
	q_t sine = 0, cosine = 0;
	qsincos(theta, &sine, &cosine);
	return sine;
}

q_t qcos(q_t theta) {
	q_t sine = 0, cosine = 0;
	qsincos(theta, &sine, &cosine);
	return cosine;
}

