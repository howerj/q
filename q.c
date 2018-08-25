/**@brief Q-Number library
 * @copyright Richard James Howe (2018)
 * @license MIT
 *
 *  TODO:
 * 	- implement basic functions (+, -, /, %, >, <, ...)
 * 	- basic mathematical functions (sin, cos, log, pow, ...)
 * 	- add modulo *and* remainder
 * 	  - <https://news.ycombinator.com/item?id=17817758>
 * 	  - <https://rob.conery.io/2018/08/21/mod-and-remainder-are-not-the-same/>
 * 	- printing/string operations
 * 	- different behaviors (saturation, modulo, exception)
 * 		- select behaviors by build system
 * 	- unit tests
 * 	- optional conversion to floats (compile time switch)
 * NOTES:
 * 	- <https://en.wikipedia.org/wiki/Q_%28number_format%29>
 * 	- <https://www.mathworks.com/help/fixedpoint/examples/calculate-fixed-point-sine-and-cosine.html>
 * 	- <http://www.coranac.com/2009/07/sines/>
 * 	- <https://en.wikipedia.org/wiki/CORDIC>
 *	- <https://dspguru.com/dsp/faqs/cordic/>
 *
 * Perhaps it would be best to use the 'qt_t' structure internally but for
 * external use a type defined integer type could be used. This could help
 * make the manipulation, storage, etcetera, for these variable easier.
 */

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

typedef uint16_t hu_t; /* half width unsigned */
typedef  int16_t hd_t; /* half width   signed */

#define BITS  (16)
#define MASK  ((1ULL << BITS) - 1ULL)
#define HIGH  (1ULL << (BITS - 1ULL))

#define UMAX  (UINT32_MAX)
#define DMIN  (INT32_MIN)
#define DMAX  (INT32_MAX)
#define LUMAX (UINT64_MAX)
#define LDMIN (INT64_MIN)
#define LDMAX (INT64_MAX)

const qinfo_t qinfo = {
	.behavior   = QBEHAVE_SATURATE_E, /**@todo Remove? Use callback instead? */
	.whole      = BITS,
	.fractional = BITS,
	.zero = { .u = 0 << BITS },
	.bit  = { .u = 1 },
	.one  = { .u = 1 << BITS },
	.min  = { .u = (HIGH << BITS) },
	.max  = { .u = (HIGH << BITS) - 1 },

	/**@warning these constants are dependent on the bit width due to how they are defined */
	.pi    = { .u = 0x3243F /* 3.243F6 A8885 A308D 31319 8A2E0... */},
	.e     = { .u = 0x2B7E1 /* 2.B7E1 5162 8A... */ },
	.sqrt2 = { .u = 0x16a09 /* 1.6A09 E667 F3... */ },
	.sqrt3 = { .u = 0x1BB67 /* 1.BB67 AE85 84... */ },
	.ln2   = { .u = 0x0B172 /* 0.B172 17F7 D1... */ },
	.ln10  = { .u = 0x24d76 /* 2.4D76 3776 AA... */ },
};

qconf_t qconf = { /**@warning global configuration options */
	.bound = qbound_saturate,
	.dp    = 5,
};

d_t qbound_saturate(ld_t s) { /**< default saturation handler */
	assert(s > DMAX || s < DMIN);
	if(s > DMAX) return DMAX;
	return DMIN;
}

static inline d_t qsat(ld_t s) {
	BUILD_BUG_ON( sizeof(u_t) !=  sizeof(d_t));
	BUILD_BUG_ON(sizeof(lu_t) !=  sizeof(ld_t));
	BUILD_BUG_ON(sizeof(hu_t) !=  sizeof(hd_t));
	BUILD_BUG_ON(sizeof(u_t)  != (sizeof(hu_t) * 2));
	BUILD_BUG_ON(sizeof(lu_t) != (sizeof(u_t)  * 2));

	if(s > DMAX) return qconf.bound(s);
	if(s < DMIN) return qconf.bound(s);
	return s;
}

static inline u_t qhigh(const q_t q) { return q.u >> BITS; }
static inline u_t qlow(const q_t q)  { return q.u & MASK; }
static inline q_t qcons(const u_t hi, const u_t lo) { return (q_t){ .u = (hi << BITS) | (lo & MASK) }; }

int qnegative(q_t a)     { return !!(qhigh(a) & HIGH); }
int qless(q_t a, q_t b)  { return a.d < b.d; }
int qmore(q_t a, q_t b)  { return a.d > b.d; }
int qequal(q_t a, q_t b) { return a.d == b.d; }
int qtoi(q_t toi)        { return (toi.u >> BITS); /**@bug sizeof(int) > sizeof(q_t) */ }
q_t qint(int toq)        { return (q_t){ .u = ((u_t)((d_t)toq)) << BITS }; }
q_t qnegate(q_t a)       { return (q_t){ .u = (~a.u) + 1ULL }; }
q_t qmin(q_t a, q_t b)   { return qless(a, b) ? a : b; }
q_t qmax(q_t a, q_t b)   { return qmore(a, b) ? a : b; }
q_t qabs(q_t a)          { return qnegative(a) ? qnegate(a) : a; }

q_t qtrunc(q_t q) { 
	const q_t adj = qnegative(q) && qlow(q) ? qinfo.one : qinfo.zero;
	q = qadd(q, adj);
	return (q_t){ .u = q.u & (MASK << BITS) }; 
} 

q_t qmk(int integer, unsigned fractional) {
	const int negative = integer < 0;
	integer = negative ? -integer : integer;
	const q_t r = qcons((d_t)integer, fractional);
	return negative ? qnegate(r) : r;
}

q_t qadd(q_t a, q_t b) {
	const ld_t dd = (ld_t)a.d + (ld_t)b.d;
	return (q_t){ .d = qsat(dd) };
}

q_t qsub(q_t a, q_t b) {
	const ld_t dd = (ld_t)a.d - (ld_t)b.d;
	return (q_t){ .d = qsat(dd) };
}

q_t qmul(q_t a, q_t b) {
	const ld_t dd = ((ld_t)a.d * (ld_t)b.d) + (lu_t)HIGH;
	/* NB. portable version of "dd >> BITS" */
	const lu_t ud = dd < 0 ? (-1ULL << (2*BITS)) | ((lu_t)dd >> BITS) : ((lu_t)dd) >> BITS;
	return (q_t){ .d = qsat(ud) };
}

q_t qdiv(q_t a, q_t b) {
	ld_t dd = ((ld_t)a.d) << BITS;
	d_t bd2 = b.d / 2;
	if((dd >= 0 && b.d >= 0) || (dd < 0 && b.d < 0)) {
		dd += bd2;
	} else {
		dd -= bd2;
	}
	return (q_t) { .d = dd / b.d };
}

/*q_t qfloor(q_t q)
{
}

q_t qceil(q_t q)
{
}*/

q_t qround(q_t q) {
	const int negative = qnegative(q);
	const int round    = !!( qlow(q) & HIGH);
	q_t r = { .d =  qsat(((ld_t)q.d) + (negative ? -round : round)) };
	return r;
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

static inline int32_t arshift(int32_t v, unsigned p) {
	uint32_t vn = v;
	if(v >= 0)
		return vn >> p;
	const uint32_t mask = ((uint32_t)(-1L)) << ((sizeof(v)*CHAR_BIT) - p);
	return mask | (vn >> p);
}

int qcordic(q_t theta, unsigned iterations, q_t *sine, q_t *cosine) {
	assert(sine);
	assert(cosine);
	static const uint32_t lookup[] = { /* atan(2^0), atan(2^-1), atan(2^-2), ... */
		0x3243, 0x1DAC, 0x0FAD, 0x07F5, 
		0x03FE, 0x01FF, 0x00FF, 0x007F, 
		0x003F, 0x001F, 0x000F, 0x0007, 
		0x0003, 0x0001, 0x0000, 0x0000,
	};
	const size_t n = sizeof lookup / sizeof lookup[0];
	const int32_t iscaling = 0x26DD; /* 1/scaling-factor */
	iterations     = iterations > n ? n : iterations;
	*sine          = qinfo.zero;
	*cosine        = qinfo.zero;

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
	int32_t x = iscaling, y = 0, z = theta.d >> 2;

	for(size_t i = 0; i < iterations; i++) {
		const int32_t  d =   -!!(z < 0);
		const int32_t xn = x - ((arshift(y, i) ^ d) - d);
		const int32_t yn = y + ((arshift(x, i) ^ d) - d);
		const int32_t zn = z - ((lookup[i] ^ d) - d);
		x = xn; /* cosine */
		y = yn; /* sine   */
		z = zn;
	}
	/* correct overflow in cosine */
	//if(x < 0) x = -x;
	/* shift */
	if(shift > 0) {
		const int32_t yt = y;
		y =  x;
		x = -yt;
	} else if(shift < 0) {
		const int32_t yt = y;
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
	cosine->d = x << 2;
	  sine->d = y << 2;
	return 0;
}


