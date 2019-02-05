/**@file q.c
 * @brief Q-Number (Q16.16, signed) library
 * @copyright Richard James Howe (2018)
 * @license MIT
 * @email howe.r.j.89@gmail.com
 * @site <https://github.com/howerj/q>
 *
 *  TODO:
 * 	- basic mathematical functions (arcsin, arccosine, pow, ...)
 * 	- add modulo *and* remainder
 * 	  - <https://news.ycombinator.com/item?id=17817758>
 * 	  - <https://rob.conery.io/2018/08/21/mod-and-remainder-are-not-the-same/>
 * 	- optional conversion to floats (compile time switch?)
 * 	- work out bounds for all functions, especially for CORDIC
 * 	functions
 * 	- GNUPlot scripts would help in visualizing results, and
 * 	errors, which can be calculated by comparing to comparing to
 * 	the C library.
 * 	- Assert inputs are in correct domain, better unit tests
 * 	- Configuration options for different methods of evaluation;
 * NOTES:
 * 	- <https://en.wikipedia.org/wiki/Modulo_operation>
 *	- move many of the smaller functions to the header, make inline with 
 *	extern inline definitions here
 *	- clean up API, removing redundant functions
 *	- improve number conversion functions; allow exponential format,
 *	'%g' format equivalent, allow custom base to be passed to functions
 *	instead of via a global.
 *	- https://stackoverflow.com/questions/17504316/what-happens-with-an-extern-inline-function
 *	- https://stackoverflow.com/questions/25415197/should-i-deliberately-inline-functions-across-translation-units-in-c99?noredirect=1&lq=1
 *	- An much simpler way of calculating sine/cosine is available at: http://files.righto.com/calculator/sinclair_scientific_simulator.html,
 *	which was used in a cheap Sinclair scientific calculator
 */

#include "q.h"
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>

#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))
#define IMPLIES(X, Y)           (!(X) || (Y))

#define BITS  (16)
#define MASK  ((1ULL <<  BITS) - 1ULL)
#define HIGH   (1ULL << (BITS  - 1ULL))

#define MULTIPLIER (INT16_MAX)
#define UMAX  (UINT32_MAX)
#define DMIN  (INT32_MIN)
#define DMAX  (INT32_MAX)
#define LUMAX (UINT64_MAX)
#define LDMIN (INT64_MIN)
#define LDMAX (INT64_MAX)

#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X, Y) ((X) < (Y) ? (Y) : (X))
#define QMK(HIGH, LOW, SF) ((ld_t)((((lu_t)HIGH) << BITS) | (MASK & ((((lu_t)LOW) << BITS) >> (SF)))))
#define QINT(INT)          ((q_t)((u_t)(INT) << BITS))
#define QPI (QMK(0x3, 0x243F, 16))

typedef  int16_t hd_t; /* half Q width,      signed */
typedef uint32_t  u_t; /* same width as Q, unsigned, but not in Q format */
typedef uint64_t lu_t; /* double Q width,  unsigned */

const qinfo_t qinfo = {
	.whole      = BITS,
	.fractional = BITS,
	.zero = (u_t)0uL << BITS,
	.bit  = 1uL,
	.one  = (u_t)1uL << BITS,
	.min  = (u_t)(HIGH << BITS),
	.max  = (u_t)((HIGH << BITS) - 1uL),

	.pi    = QPI, /* 3.243F6 A8885 A308D 31319 8A2E0... */
	.e     = QMK(0x2, 0xB7E1, 16), /* 2.B7E1 5162 8A... */
	.sqrt2 = QMK(0x1, 0x6A09, 16), /* 1.6A09 E667 F3... */
	.sqrt3 = QMK(0x1, 0xBB67, 16), /* 1.BB67 AE85 84... */
	.ln2   = QMK(0x0, 0xB172, 16), /* 0.B172 17F7 D1... */
	.ln10  = QMK(0x2, 0x4D76, 16), /* 2.4D76 3776 AA... */
};

qconf_t qconf = { /* Global Configuration Options */
	.bound = qbound_saturate,
	.dp    = 5,
	.base  = 10,
};

/********* Basic Library Routines ********************************************/

static void static_assertions(void) {
	/*@todo more static assertions relating to min/max numbers, and MASK */
	BUILD_BUG_ON(CHAR_BIT != 8);
	BUILD_BUG_ON((sizeof(q_t)*CHAR_BIT) != (BITS * 2));
	BUILD_BUG_ON( sizeof(q_t) !=  sizeof(u_t));
	BUILD_BUG_ON( sizeof(u_t) !=  sizeof(d_t));
	BUILD_BUG_ON(sizeof(lu_t) !=  sizeof(ld_t));
	BUILD_BUG_ON(sizeof(d_t)  != (sizeof(hd_t) * 2));
	BUILD_BUG_ON(sizeof(lu_t) != (sizeof(u_t)  * 2));
}

q_t qbound_saturate(const ld_t s) { /**< default saturation handler */
	assert(s > DMAX || s < DMIN);
	if (s > DMAX) return DMAX;
	return DMIN;
}

q_t qbound_wrap(const ld_t s) { /**< wrap numbers on overflow */
	assert(s > DMAX || s < DMIN);
	if (s > DMAX) return DMIN + (s % DMAX);
	return DMAX - ((-s) % DMAX);
}

static inline q_t qsat(const ld_t s) {
	static_assertions();
	if (s > DMAX) return qconf.bound(s);
	if (s < DMIN) return qconf.bound(s);
	return s;
}

inline d_t arshift(const d_t v, const unsigned p) {
	u_t vn = v;
	if (v >= 0l)
		return vn >> p;
	const u_t leading = ((u_t)(-1l)) << ((sizeof(v)*CHAR_BIT) - p);
	return leading | (vn >> p);
}

inline d_t divn(const d_t v, const unsigned p) {
	return v / (1l << p); /**@todo replace with portable bit shifting version */
	///*const d_t m = -!!(v < 0);
	//return (v + (m & (((d_t)1 << p) + (d_t)-1UL))) >> p;*/
}

static inline u_t qhigh(const q_t q) { return ((u_t)q) >> BITS; }
static inline u_t qlow(const q_t q)  { return ((u_t)q) & MASK; }
static inline q_t qcons(const u_t hi, const u_t lo) { return (hi << BITS) | (lo & MASK); }

int qisnegative(const q_t a)            { return !!(qhigh(a) & HIGH); }
int qispositive(const q_t a)            { return  !(qhigh(a) & HIGH); }
int qisinteger(const q_t a)             { return  !qlow(a); }
int qisodd(const q_t a)                 { return qisinteger(a) &&  (qhigh(a) & 1); }
int qiseven(const q_t a)                { return qisinteger(a) && !(qhigh(a) & 1); }
int qless(const q_t a, const q_t b)     { return a < b; }
int qeqless(const q_t a, const q_t b)   { return a <= b; }
int qmore(const q_t a, const q_t b)     { return a > b; }
int qeqmore(const q_t a, const q_t b)   { return a >= b; }
int qequal(const q_t a, const q_t b)    { return a == b; }
int qunequal(const q_t a, const q_t b)  { return a != b; }
int qtoi(const q_t toi)                 { return ((lu_t)((ld_t)toi)) >> BITS; }
q_t qint(const int toq)                 { return ((u_t)((d_t)toq)) << BITS; }
signed char qtoc(const q_t q)           { return qtoi(q); }
q_t qchar(signed char c)                { return qint(c); }
short qtoh(const q_t q)                 { return qtoi(q); }
q_t qshort(short s)                     { return qint(s); }
long qtol(const q_t q)                  { return qtoi(q); }
q_t qlong(long l)                       { return qint(l); }
long long qtoll(const q_t q)            { return qtoi(q); }
q_t qvlong(long long ll)                { return qint(ll); }
q_t qnegate(const q_t a)                { return (~(u_t)a) + 1ULL; }
q_t qmin(const q_t a, const q_t b)      { return qless(a, b) ? a : b; }
q_t qmax(const q_t a, const q_t b)      { return qmore(a, b) ? a : b; }
q_t qabs(const q_t a)                   { return qisnegative(a) ? qnegate(a) : a; }
q_t qadd(const q_t a, const q_t b)      { return qsat((ld_t)a + (ld_t)b); }
q_t qsub(const q_t a, const q_t b)      { return qsat((ld_t)a - (ld_t)b); }
q_t qcopysign(const q_t a, const q_t b) { return qisnegative(b) ? qnegate(qabs(a)) : qabs(a); }
q_t qand(const q_t a, const q_t b)      { return a & b; }
q_t qxor(const q_t a, const q_t b)      { return a ^ b; }
q_t qor(const q_t a, const q_t b)       { return a | b; }
q_t qinvert(const q_t a)                { return ~a; }
/** @note Should the shifting functions accept an integer instead for their shift param? */
q_t qlrs(const q_t a, const q_t b)      { /* assert low bits == 0? */ return (u_t)a >> (u_t)qtoi(b); }
q_t qlls(const q_t a, const q_t b)      { return (u_t)a << b; }
q_t qars(const q_t a, const q_t b)      { return arshift(a, qtoi(b)); }
q_t qals(const q_t a, const q_t b)      { return qsat((lu_t)a << b); }
q_t qsign(const q_t a)                  { return qisnegative(a) ? -QINT(1) : QINT(1); }
q_t qsignum(const q_t a)                { return a ? qsign(a) : QINT(0); }

int qapproxequal(const q_t a, const q_t b, const q_t epsilon) { /**@todo test this, compare with qwithin in 't.c'*/
	assert(qeqmore(epsilon, qint(0))); 
	return qless(qabs(qsub(a, b)), epsilon); 
}

int qapproxunequal(const q_t a, const q_t b, const q_t epsilon) { 
	return !qapproxequal(a, b, epsilon); 
}


/*int qwithin(const q_t value, const q_t lower, const q_t upper) {
	return qeqmore(value, lower) && qeqless(value, upper);
}*/

/** @todo create a conversion routine that takes three numbers:
     integer, fractional, divisor
   So Q numbers can be generated more easily */
/*
q_t qconstruct(const int integer, const unsigned long fractional, const unsigned nexponent) {
	// assert(integer <= max_q_int && integer => min_q_int); // no effect is sizeof(int) <= sizeof(q_t)
	// assert(fractional <= max_fract_int && fractional => min_frac_int);
	const q_t i = qint(integer);
	unsigned long f = (fractional << BITS) / (10ul * nexponent);
	// assert((f & HIGH_BITS) == 0);
	return i | f;
} */

q_t qfloor(const q_t q) {
	return q & ~MASK;
}

q_t qceil(q_t q) {
	const q_t adj = qisinteger(q) ? QINT(0) : QINT(1);
	q = qadd(q, adj);
	return ((u_t)q) & (MASK << BITS);
}

q_t qtrunc(q_t q) {
	const q_t adj = qisnegative(q) && qlow(q) ? QINT(1) : QINT(0);
	q = qadd(q, adj);
	return ((u_t)q) & (MASK << BITS);
}

q_t qround(q_t q) {
	const int negative = qisnegative(q);
	q = qabs(q);
	const q_t adj = (qlow(q) & HIGH) ? QINT(1) : QINT(0);
	q = qadd(q, adj);
	q = ((u_t)q) & (MASK << BITS);
	return negative ? qnegate(q) : q;
}

int qpack(const q_t *q, char *buffer, const size_t length) {
	assert(buffer);
	if (length < sizeof(*q))
		return -1;
	q_t qn = *q;
	uint8_t *b = (uint8_t*)buffer;
	for (size_t i = 0; i < sizeof(qn); i++) {
		b[i] = qn;
		qn = (u_t)qn >> CHAR_BIT;
	}
	return sizeof(qn);
}

int qunpack(q_t *q, const char *buffer, const size_t length) {
	assert(q);
	assert(buffer);
	if (length < sizeof(*q))
		return -1;
	uint8_t *b = (uint8_t*)buffer;
	u_t nq = 0;
	for (size_t i = 0; i < sizeof(*q); i++) {
		nq <<= CHAR_BIT;
		nq |= b[sizeof(*q)-i-1];
	}
	*q = nq;
	return sizeof(*q);
}

static inline ld_t multiply(const q_t a, const q_t b) {
	const ld_t dd = ((ld_t)a * (ld_t)b) + (lu_t)HIGH;
	/* NB. portable version of "dd >> BITS", for double width signed values */
	return dd < 0 ? (-1ull << (2 * BITS)) | ((lu_t)dd >> BITS) : ((lu_t)dd) >> BITS;
}

q_t qmul(const q_t a, const q_t b) {
	return qsat(multiply(a, b));
}

q_t qfma(const q_t a, const q_t b, const q_t c) {
	return qsat(multiply(a, b) + (ld_t)c);
}

q_t qdiv(const q_t a, const q_t b) {
	assert(b);
	const ld_t dd = ((ld_t)a) << BITS;
	ld_t bd2 = divn(b, 1);
	if (!((dd >= 0 && b > 0) || (dd < 0 && b < 0)))
		bd2 = -bd2;
	return (dd + bd2) / b;
}

q_t qrem(const q_t a, const q_t b) {
	return qsub(a, qmul(qtrunc(qdiv(a, b)), b));
}

/*q_t qmod(q_t a, q_t b) {
	a = qabs(a);
	b = qabs(b);
	const q_t nb = qnegate(b);
	while (qeqless(a, nb)) a = qadd(a, b);
	while (qeqmore(a,  b)) a = qadd(a, nb);
	return a;
}*/

/********* Basic Library Routines ********************************************/
/********* Numeric Input/Output **********************************************/

static char itoch(const unsigned ch) {
	assert(ch < 36);
	if (ch <= 9)
		return ch + '0';
	return ch + 'A';
}

static inline void swap(char *a, char *b) {
	assert(a);
	assert(b);
	const int c = *a;
	*a = *b;
	*b = c;
}

static void reverse(char *s, const size_t length) {
	assert(s);
	for (size_t i = 0; i < length/2; i++)
		swap(&s[i], &s[length - i - 1]);
}

static int uprint(u_t p, char *s, const size_t length, const long base) {
	assert(s);
	assert(base >= 2 && base <= 36);
	if (length < 2)
		return -1;
	size_t i = 0;
	do {
		unsigned ch = p % base;
		p /= base;
		s[i++] = itoch(ch);
	} while (p && i < length);
	if (p && i >= length)
		return -1;
	reverse(s, i);
	return i;
}

/* <https://codereview.stackexchange.com/questions/109212> */
int qsprintb(q_t p, char *s, size_t length, const unsigned long base) {
	assert(s);
	const int negative = qisnegative(p);
	if (negative)
		p = qnegate(p);
	const d_t hi = qhigh(p);
	char frac[BITS+2] = { '.' };
	memset(s, 0, length);
	assert(base >= 2 && base <= 36);
	u_t lo = qlow(p);
	size_t i = 1;
	for (i = 1; lo; i++) {
		if (qconf.dp >= 0 && (int)i >= qconf.dp)
			break;
		lo *= base;
		frac[i] = itoch(lo >> BITS);
		lo &= MASK;
	}
	if (negative)
		s[0] = '-';
	const int hisz = uprint(hi, s + negative, length - 1, base);
	if (hisz < 0 || (hisz + i + negative + 1) > length)
		return -1;
	memcpy(s + hisz + negative, frac, i);
	return i + hisz;
}


int qsprint(const q_t p, char *s, const size_t length) {
	return qsprintb(p, s, length, qconf.base); 
}

static inline int extract(unsigned char c, const int radix) {
	c = tolower(c);
	if (c >= '0' && c <= '9')
		c -= '0';
	else if (c >= 'a' && c <= 'z')
		c -= ('a' - 10);
	else
		return -1;
	if (c < radix)
		return c;
	return -1;
}

static inline q_t qmk(long integer, unsigned long fractional) {
	const int negative = integer < 0;
	integer = negative ? -integer : integer;
	const q_t r = qcons((d_t)integer, fractional);
	return negative ? qnegate(r) : r;
}

int qnconvb(q_t *q, const char *s, size_t length, const long base) {
	assert(q);
	assert(s);
	*q = QINT(0);
	if (length < 1)
		return -1;
	const long idp = qconf.dp;
	long hi = 0, lo = 0, places = 1, negative = 0, overflow = 0;
	size_t sidx = 0;
	assert(base >= 2 && base <= 36);

	if (s[sidx] == '-') {
		if (length < 2)
			return -1;
		negative = 1;
		sidx++;
	}

	for (; sidx < length && s[sidx]; sidx++) {
		const long e = extract(s[sidx], base);
		if (e < 0)
			break;
		hi = (hi * base) + e;
		if (hi > MULTIPLIER) /* continue on with conversion */
			overflow = 1;
	}
	if (sidx >= length || !s[sidx])
		goto done;
	if (s[sidx] != '.')
		return -2;
	sidx++;
	
	for (int dp = 0; sidx < length && s[sidx]; sidx++, dp++) {
		const int ch = extract(s[sidx], base);
		if (ch < 0)
			return -3;
		if (dp <= idp) { /* continue on with conversion */
			lo = (lo * base) + ch;
			if (places > (DMAX/base))
				return -4;
			places *= base;
		}
	}
	if (!places)
		return -5;
	lo = ((long)((unsigned long)lo << BITS) / places);
done:
	{
		const q_t nq = qmk(hi, lo);
		*q = negative ? qnegate(nq) : nq;
	}
	if (overflow)
		return -6;
	return 0;
}

int qnconv(q_t *q, const char *s, size_t length) {
	return qnconvb(q, s, length, qconf.base);
}

int qconv(q_t *q, const char * const s) {
	assert(s);
	return qnconv(q, s, strlen(s));
}

int qconvb(q_t *q, const char * const s, const long base) {
	assert(s);
	return qnconvb(q, s, strlen(s), base);
}


/********* Numeric Input/Output **********************************************/
/********* CORDIC Routines ***************************************************/

typedef enum {
	CORDIC_MODE_VECTOR_E/* = 'VECT'*/,
	CORDIC_MODE_ROTATE_E/* = 'ROT'*/,
} cordic_mode_e;

typedef enum {
	CORDIC_COORD_HYPERBOLIC_E = -1,
	CORDIC_COORD_LINEAR_E     =  0,
	CORDIC_COORD_CIRCULAR_E   =  1,
} cordic_coordinates_e;

static const d_t cordic_circular_scaling   = 0x9B74; /* 1/scaling-factor */
static const d_t cordic_hyperbolic_scaling = 0x13520; /* 1/scaling-factor */

/* Universal CORDIC <https://en.wikibooks.org/wiki/Digital_Circuits/CORDIC>
 *
 *	x(i+1) = x(i) - u.d(i).y(i).pow(2, -i)
 * 	y(i+1) = y(i) +   d(i).x(i).pow(2, -i)
 * 	z(i+1) = z(i) -   d(i).a(i)
 *
 *  d(i) =  sgn(z(i))      (rotation)
 *  d(i) = -sgn(x(i).y(i)) (vectoring)
 *
 *             hyperbolic      linear          circular
 *  u =                -1           0                 1
 *  a = atanh(pow(2, -i))  pow(2, -i)  atan(pow(2, -i))
 *
 *  linear shift sequence:      i = 0, 1, 2, 3, ...
 *  circular shift sequence:    i = 1, 2, 3, 4, ...
 *  hyperbolic shift sequence:  i = 1, 2, 3, 4, 4, 5, ... */
static int cordic(const cordic_coordinates_e coord, const cordic_mode_e mode, int iterations, d_t *x0, d_t *y0, d_t *z0) {
	assert(x0);
	assert(y0);
	assert(z0);
	if (mode != CORDIC_MODE_VECTOR_E && mode != CORDIC_MODE_ROTATE_E)
		return -1;

	BUILD_BUG_ON(sizeof(d_t) != sizeof(uint32_t));
	BUILD_BUG_ON(sizeof(u_t) != sizeof(uint32_t));

	static const u_t arctans[] = { /* atan(2^0), atan(2^-1), atan(2^-2), ... */
		0xC90FuL, 0x76B1uL, 0x3EB6uL, 0x1FD5uL,
		0x0FFAuL, 0x07FFuL, 0x03FFuL, 0x01FFuL,
		0x00FFuL, 0x007FuL, 0x003FuL, 0x001FuL,
		0x000FuL, 0x0007uL, 0x0003uL, 0x0001uL,
		0x0000uL, // 0x0000uL,
	};
	static const size_t arctans_length = sizeof arctans / sizeof arctans[0];

	static const u_t arctanhs[] = { /* atanh(2^-1), atanh(2^-2), ... */
		0x8c9fuL, 0x4162uL, 0x202buL, 0x1005uL,
		0x0800uL, 0x0400uL, 0x0200uL, 0x0100uL,
		0x0080uL, 0x0040uL, 0x0020uL, 0x0010uL,
		0x0008uL, 0x0004uL, 0x0002uL, 0x0001uL,
		0x0000uL, // 0x0000uL,
	};
	static const size_t arctanhs_length = sizeof arctanhs / sizeof arctanhs[0];

	static const u_t halfs[] = { /* 2^0, 2^-1, 2^-2, ..*/
		0x10000uL,
		0x8000uL, 0x4000uL, 0x2000uL, 0x1000uL,
		0x0800uL, 0x0400uL, 0x0200uL, 0x0100uL,
		0x0080uL, 0x0040uL, 0x0020uL, 0x0010uL,
		0x0008uL, 0x0004uL, 0x0002uL, 0x0001uL,
		//0x0000uL, // 0x0000uL,
	};
	static const size_t halfs_length = sizeof halfs / sizeof halfs[0];

	const u_t *lookup = NULL;
	size_t i = 0, j = 0, k = 0, length = 0;
	const size_t *shiftx = NULL, *shifty = NULL;
	int hyperbolic = 0;

	switch (coord) { /**@todo check this, it's probably buggy for linear/hyperbolic, it's also ugly*/
	case CORDIC_COORD_CIRCULAR_E:
		lookup = arctans;
		length = arctans_length;
		i = 0;
		shifty = &i;
		shiftx = &i;
		break;
	case CORDIC_COORD_HYPERBOLIC_E:
		lookup = arctanhs;
		length = arctanhs_length;
		hyperbolic = 1;
		i = 1;
		shifty = &i;
		shiftx = &i;
		break;
	case CORDIC_COORD_LINEAR_E:
		lookup = halfs;
		length = halfs_length;
		shifty = &j;
		shiftx = NULL;
		i = 1;
		break;
	default: /* not implemented */
		return -2;
	}

	iterations = iterations > (int)length ? (int)length : iterations;
	iterations = iterations < 0           ? (int)length : iterations;

	d_t x = *x0, y = *y0, z = *z0;

	/* rotation mode: z determines direction,
	 * vector mode:   y determines direction */
	for (; j < (unsigned)iterations; i++, j++) {
		again:
		{
			const d_t  m = mode == CORDIC_MODE_ROTATE_E ? z : -y /*-(qmul(y,x))*/;
			const d_t  d =   -!!(m /*<=*/ < 0);
			const d_t xs = ((((shiftx ? divn(y, *shiftx) : 0)) ^ d) - d);
			const d_t ys =             (divn(x, *shifty)       ^ d) - d;
			const d_t xn = x - (hyperbolic ? -xs : xs);
			const d_t yn = y + ys;
			const d_t zn = z - ((lookup[j] ^ d) - d);
			x = xn; /* cosine, in circular, rotation mode */
			y = yn; /*   sine, in circular, rotation mode   */
			z = zn;
		}
		if (hyperbolic) {
			if (k++ >= 3) {
				k = 0;
				goto again;
			}
		}
	}
	*x0 = x;
	*y0 = y;
	*z0 = z;

	return iterations;
}

/* See: - <https://dspguru.com/dsp/faqs/cordic/>
 *      - <https://en.wikipedia.org/wiki/CORDIC> */
static int qcordic(q_t theta, const int iterations, q_t *sine, q_t *cosine) {
	assert(sine);
	assert(cosine);

	static const q_t   pi =   QPI,    npi =  -QPI;
	static const q_t  hpi =   QPI/2, hnpi = -(QPI/2);
	static const q_t  qpi =   QPI/4, qnpi = -(QPI/4);
	static const q_t  dpi =   QPI*2, dnpi = -(QPI*2);

	/* convert to range -pi   to pi */
	while (qless(theta, npi)) theta = qadd(theta,  dpi);
	while (qmore(theta,  pi)) theta = qadd(theta, dnpi);

	int negate = 0, shift = 0;

	/* convert to range -pi/2 to pi/2 */
	if (qless(theta, hnpi)) {
		theta = qadd(theta,  pi);
		negate = 1;
	} else if (qmore(theta, hpi)) {
		theta = qadd(theta, npi);
		negate = 1;
	}

	/* convert to range -pi/4 to pi/4 */
	if (qless(theta, qnpi)) {
		theta = qadd(theta,  hpi);
		shift = -1;
	} else if (qmore(theta, qpi)) {
		theta = qadd(theta, hnpi);
		shift =  1;
	}

	d_t x = cordic_circular_scaling, y = 0, z = theta /* no theta scaling needed */;

	/* CORDIC in Q2.16 format */
	if (cordic(CORDIC_COORD_CIRCULAR_E, CORDIC_MODE_ROTATE_E, iterations, &x, &y, &z) < 0)
		return -1;

	/* undo shifting and quadrant changes */
	if (shift > 0) {
		const d_t yt = y;
		y =  x;
		x = -yt;
	} else if (shift < 0) {
		const d_t yt = y;
		y = -x;
		x =  yt;
	}

	if (negate) {
		x = -x;
		y = -y;
	}
	/* set output; no scaling needed */
	*cosine = x;
	  *sine = y;
	return 0;
}

q_t qatan(const q_t t) {
	q_t x = qint(1), y = t, z = QINT(0);
	cordic(CORDIC_COORD_CIRCULAR_E, CORDIC_MODE_VECTOR_E, -1, &x, &y, &z);
	return z;
}

void qsincos(q_t theta, q_t *sine, q_t *cosine) {
	assert(sine);
	assert(cosine);
	const int r = qcordic(theta, -1, sine, cosine);
	assert(r >= 0);
}

q_t qsin(const q_t theta) {
	q_t sine = QINT(0), cosine = QINT(0);
	qsincos(theta, &sine, &cosine);
	return sine;
}

q_t qcos(const q_t theta) {
	q_t sine = QINT(0), cosine = QINT(0);
	qsincos(theta, &sine, &cosine);
	return cosine;
}

q_t qtan(const q_t theta) {
	q_t sine = QINT(0), cosine = QINT(0);
	qsincos(theta, &sine, &cosine);
	return qdiv(sine, cosine); /* can use qcordic_div, with range limits it imposes */
}

q_t qcot(const q_t theta) {
	q_t sine = QINT(0), cosine = QINT(0);
	qsincos(theta, &sine, &cosine);
	return qdiv(cosine, sine); /* can use qcordic_div, with range limits it imposes */
}

q_t qcordic_mul(const q_t a, const q_t b) { /* works for small values; result < 4 */
	q_t x = a, y = QINT(0), z = b;
	const int r = cordic(CORDIC_COORD_LINEAR_E, CORDIC_MODE_ROTATE_E, -1, &x, &y, &z);
	assert(r >= 0);
	return y;
}

q_t qcordic_div(const q_t a, const q_t b) {
	q_t x = b, y = a, z = QINT(0);
	const int r = cordic(CORDIC_COORD_LINEAR_E, CORDIC_MODE_VECTOR_E, -1, &x, &y, &z);
	assert(r >= 0);
	return z;
}

void qsincosh(const q_t a, q_t *sinh, q_t *cosh) {
	assert(sinh);
	assert(cosh);
	q_t x = cordic_hyperbolic_scaling, y = QINT(0), z = a; // (e^2x - 1) / (e^2x + 1)
	const int r = cordic(CORDIC_COORD_HYPERBOLIC_E, CORDIC_MODE_ROTATE_E, -1, &x, &y, &z);
	assert(r >= 0);
	*sinh = y;
	*cosh = x;
}

q_t qtanh(const q_t a) {
	q_t sinh = QINT(0), cosh = QINT(0);
	qsincosh(a, &sinh, &cosh);
	return qdiv(sinh, cosh);
}

q_t qcosh(const q_t a) {
	q_t sinh = QINT(0), cosh = QINT(0);
	qsincosh(a, &sinh, &cosh);
	return cosh;
}

q_t qsinh(const q_t a) {
	q_t sinh = QINT(0), cosh = QINT(0);
	qsincosh(a, &sinh, &cosh);
	return sinh;
}

q_t qcordic_exp(const q_t e) {
	q_t s = QINT(0), h = QINT(0);
	qsincosh(e, &s, &h);
	return qadd(s, h);
}

q_t qcordic_ln(const q_t d) {
	q_t x = qadd(d, QINT(1)), y = qsub(d, QINT(1)), z = QINT(0);
	static const q_t two = QINT(2);
	const int r = cordic(CORDIC_COORD_HYPERBOLIC_E, CORDIC_MODE_VECTOR_E, -1, &x, &y, &z);
	assert(r >= 0);
	return qmul(z, two);
}

q_t qcordic_sqrt(const q_t n) {  /* testing only; works for 0 < x < 2 */
	const q_t quarter = 1uLL << (BITS - 2); /* 0.25 */
	q_t x = qadd(n, quarter),
	    y = qsub(n, quarter),
	    z = 0;
	const int r = cordic(CORDIC_COORD_HYPERBOLIC_E, CORDIC_MODE_VECTOR_E, -1, &x, &y, &z);
	assert(r >= 0);
	return qmul(x, cordic_hyperbolic_scaling);
}

q_t qhypot(const q_t a, const q_t b) {
	q_t x = qabs(a), y = qabs(b), z = QINT(0); /* abs() should not be needed? */
	const int r = cordic(CORDIC_COORD_CIRCULAR_E, CORDIC_MODE_VECTOR_E, -1, &x, &y, &z);
	assert(r >= 0);
	return qmul((x), cordic_circular_scaling);
}

void qpol2rec(const q_t magnitude, const q_t theta, q_t *i, q_t *j) {
	assert(i);
	assert(j);
	q_t sin = QINT(0), cos = QINT(0);
	qsincos(theta, &sin, &cos);
	*i = qmul(sin, magnitude);
	*j = qmul(cos, magnitude);
}

void qrec2pol(const q_t i, const q_t j, q_t *magnitude, q_t *theta) {
	assert(magnitude);
	assert(theta);
	const int is = qisnegative(i), js = qisnegative(j);
	q_t x = qabs(i), y = qabs(j), z = QINT(0);
	const int r = cordic(CORDIC_COORD_CIRCULAR_E, CORDIC_MODE_VECTOR_E, -1, &x, &y, &z);
	assert(r >= 0);
	*magnitude = qmul((x), cordic_circular_scaling);
	if (is && js)
		z = qadd(z, QPI);
	else if (js)
		z = qadd(z, QPI/2l);
	else if (is)
		z = qadd(z, (3l*QPI)/2l);
	*theta = z;
}

q_t qcordic_hyperbolic_gain(const int n) {
	q_t x = QINT(1), y = QINT(0), z = QINT(0);
	const int r = cordic(CORDIC_COORD_HYPERBOLIC_E, CORDIC_MODE_ROTATE_E, n, &x, &y, &z);
	assert(r >= 0);
	return x;
}

q_t qcordic_circular_gain(const int n) {
	q_t x = QINT(1), y = QINT(0), z = QINT(0);
	const int r = cordic(CORDIC_COORD_CIRCULAR_E, CORDIC_MODE_ROTATE_E, n, &x, &y, &z);
	assert(r >= 0);
	return x;
}

/********* CORDIC Routines ***************************************************/
/********* Power / Logarithms ************************************************/

static int isodd(const unsigned n) {
	return n & 1;
}

d_t dpower(d_t b, unsigned e) { /* https://stackoverflow.com/questions/101439 */
    d_t result = 1;
    for (;;) {
        if (isodd(e))
            result *= b;
        e >>= 1;
        if (!e)
            break;
        b *= b;
    }
    return result;
}

d_t dlog(d_t x, const unsigned base) { /* rounds up, look at remainder to round down */
	d_t b = 0;
	assert(x && base > 1);
	while ((x /= (d_t)base)) /* can use >> for base that are powers of two */
		b++;
	return b;
}

q_t qlog(const q_t x) {
	assert(qmore(x, 0));
	static const q_t lmax = QMK(9, 0x8000, 16); /* 9.5, lower limit needs checking */
	if (qmore(x, lmax)) /**@todo refactor so this is not recursive */
		return qadd(qinfo.ln2, qlog(divn(x, 1)));
	return qcordic_ln(x);
}

q_t qexp(const q_t e) { /* exp(e) = exp(e/2)*exp(e/2) */
	if (qless(e, QINT(1)))
		return qcordic_exp(e);
	const q_t ed2 = qexp(divn(e, 1));
	return qmul(ed2, ed2);
}

/* // Nearly works!
 * q_t qpow(q_t n, const q_t exp) { 
	// return qexp(multiply(qlog(n), exp));
	if (qequal(QINT(0), n)) {
		if (qeqmore(exp, QINT(0)))
			return QINT(0);
		// else abort()
	}	
	const bool isn = qisnegative(n);
	n = qabs(n);
	if (qisnegative(exp))
		n = qdiv(QINT(1), qpow(n, qabs(exp)));
	else
		n = qexp(multiply(qlog(n), exp));
	return isn ? qnegate(n) : n;
}
*/
q_t qsqrt(const q_t x) { /* Newton Rhaphson method */
	assert(qeqmore(x, 0));
	const q_t difference = qmore(x, QINT(100)) ? 0x0100 : 0x0010;
	if (qequal(QINT(0), x))
		return QINT(0);
	q_t guess = qmore(x, qinfo.sqrt2) ? divn(x, 1) : QINT(1);
	while (qmore(qabs(qsub(qmul(guess, guess), x)), difference))
		guess = divn(qadd(qdiv(x, guess), guess), 1);
	return qabs(guess); /* correct for overflow int very large numbers */
}

/* See: https://dsp.stackexchange.com/questions/25770/looking-for-an-arcsin-algorithm
 * https://stackoverflow.com/questions/7378187/approximating-inverse-trigonometric-functions
 * https://ch.mathworks.com/examples/fixed-point-designer/mw/fixedpoint_product-fixpt_atan2_demo-calculate-fixed-point-arctangent
 */

q_t qasin(const q_t t) { /** @bug not working! */
	return qmul(QINT(2), qatan(qdiv(t, qadd(QINT(1), qsqrt(qsub(QINT(1), qmul(t, t)))))));
}

q_t qacos(const q_t t) { /** @bug not working! */
	return qmul(QINT(2), qatan(qdiv(qsqrt(qsub(QINT(1), qmul(t, t))) , qadd(QINT(1), t))));
}

/********* Power / Logarithms ************************************************/
/********* Conversion Utilities **********************************************/

q_t qdeg2rad(const q_t deg) {
	return qdiv(qmul(QPI, deg), QINT(180));
}

q_t qrad2deg(const q_t rad) {
	return qdiv(qmul(QINT(180), rad), QPI);
}

// long/int/q32.32/q8.8 conversion routines go here, if implemented

/********* Conversion Utilities **********************************************/

/********* Filters ***********************************************************/

void qfilter_init(qfilter_t *f, const q_t time, const q_t rc, const q_t seed) {
	assert(f);
	f->time = time;
	f->rc = rc;
	f->filtered = seed; // alpha * seed for LPF
	f->raw = seed;
}

q_t qfilter_low_pass(qfilter_t *f, const q_t time, const q_t data) {
	assert(f);
	/* If the calling rate is constant (for example the function is
	 * guaranteed to be always called at a rate of 5 milliseconds) we
	 * can avoid the costly alpha calculation! */
	const q_t dt = (u_t)time - (u_t)f->time;
	const q_t alpha = qdiv(dt, qadd(f->rc, dt));
	f->filtered = qfma(alpha, qsub(data, f->filtered), f->filtered);
	f->time = time;
	f->raw  = data;
	return f->filtered;
}

q_t qfilter_high_pass(qfilter_t *f, const q_t time, const q_t data) {
	assert(f);
	const q_t dt = (u_t)time - (u_t)f->time;
	const q_t alpha = qdiv(f->rc, qadd(f->rc, dt));
	f->filtered = qmul(alpha, qadd(f->filtered, qsub(data, f->raw)));
	f->time = time;
	f->raw  = data;
	return f->filtered;
}

q_t qfilter_value(const qfilter_t *f) {
	assert(f);
	return f->filtered;
}

/********* Filters ***********************************************************/

/********* PID Controller ****************************************************/

/* Must be called at a constant rate; perhaps a PID which takes call time
 * into account could be made, but that would complicate things. Differentiator
 * term needs filtering also. */
q_t qpid_update(qpid_t *pid, const q_t error, const q_t position) {
	assert(pid);
	const q_t p  = qmul(pid->p_gain, error);
	pid->i_state = qadd(pid->i_state, error);
	pid->i_state = qmax(pid->i_state, pid->i_min);
	pid->i_state = qmin(pid->i_state, pid->i_max);
	const q_t i  = qmul(pid->i_state, pid->i_gain);
	const q_t d  = qmul(pid->d_gain, qsub(position, pid->d_state));
	pid->d_state = position;
	return qsub(qadd(p, i), d);
}

/********* PID Controller ****************************************************/

