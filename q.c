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
 * 	- work out bounds for all functions, especially for CORDIC
 * 	functions
 * 	- degrees/radians, (optional) float to Q, Q16.16 to Q32.32, Q16.16 to Q8.8,
 * 	Q16.16 to Q4.4, conversion routines
 * 	- GNUPlot scripts would help in visualizing results, and
 * 	errors, which can be calculated by comparing to comparing to
 * 	the C library.
 * 	- C++ wrapper, tailored to C++.
 * 	- Options, in the test library, for printing out ranges of functions,
 * 	for calculating errors as compared with the built in C functions
 * 	- Modes to generate the CORDIC tables
 * 	- Functions for calculating gain at runtime for the CORDIC functions
 * NOTES:
 * 	- <https://en.wikipedia.org/wiki/Q_%28number_format%29>
 * 	- <https://www.mathworks.com/help/fixedpoint/examples/calculate-fixed-point-sine-and-cosine.html>
 * 	- <http://www.coranac.com/2009/07/sines/>
 * 	- <https://stackoverflow.com/questions/4657468/fast-fixed-point-pow-log-exp-and-sqrt>
 * 	- <https://en.wikipedia.org/wiki/Modulo_operation>
 *
 * For a Q8.8 library it would be quite possible to do an exhaustive
 * proof of correctness over most expected properties. */

#include "q.h"
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>

#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))

#define BITS  (16)
#define MASK  ((1ULL <<  BITS) - 1ULL)
#define HIGH   (1ULL << (BITS  - 1ULL))

#define UMAX  (UINT32_MAX)
#define DMIN  (INT32_MIN)
#define DMAX  (INT32_MAX)
#define LUMAX (UINT64_MAX)
#define LDMIN (INT64_MIN)
#define LDMAX (INT64_MAX)

#ifndef MIN
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#endif

#ifndef MAX
#define MAX(X, Y) ((X) < (Y) ? (Y) : (X))
#endif

#define QMK(HIGH, LOW, SF) ((ld_t)((((lu_t)HIGH) << BITS) | (MASK & ((((lu_t)LOW) << BITS) >> (SF)))))

#define QPI (QMK(0x3, 0x243F, 16))

typedef  int16_t hd_t; /* half Q width,      signed */
typedef uint32_t  u_t; /* same width as Q, unsigned, but not in Q format */
typedef uint64_t lu_t; /* double Q width,  unsigned */

typedef enum {
	NUMBER_INPUT_FAILED_E = -1,
	NUMBER_INPUT_OK_E     =  0,
	NUMBER_INPUT_RANGE_E  =  1,
} number_input_error_e;

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

qconf_t qconf = { /**@warning global configuration options */
	.bound = qbound_saturate,
	.dp    = 5,
	.base  = 0, /**< @todo remove special case of '0' */
};

/********* Basic Library Routines ********************************************/

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
	/*@todo more static assertions relating to min/max numbers, BITS and MASK */
	BUILD_BUG_ON( sizeof(q_t) !=  sizeof(u_t));
	BUILD_BUG_ON( sizeof(u_t) !=  sizeof(d_t));
	BUILD_BUG_ON(sizeof(lu_t) !=  sizeof(ld_t));
	BUILD_BUG_ON(sizeof(d_t)  != (sizeof(hd_t) * 2));
	BUILD_BUG_ON(sizeof(lu_t) != (sizeof(u_t)  * 2));

	if(s > DMAX) return qconf.bound(s);
	if(s < DMIN) return qconf.bound(s);
	return s;
}

inline d_t arshift(d_t v, unsigned p) {
	u_t vn = v;
	if(v >= 0)
		return vn >> p;
	const u_t leading = ((u_t)(-1L)) << ((sizeof(v)*CHAR_BIT) - p);
	return leading | (vn >> p);
}

inline d_t divn(d_t v, unsigned p) {
	return v/(1<<p); /**@todo replace with portable bit shifting version */
	/*const d_t m = -!!(v < 0);
	return (v + (m & (((d_t)1 << p) + (d_t)-1UL))) >> p;*/
}

static inline u_t qhigh(const q_t q) { return ((u_t)q) >> BITS; }
static inline u_t qlow(const q_t q)  { return ((u_t)q) & MASK; }
static inline q_t qcons(const u_t hi, const u_t lo) { return (hi << BITS) | (lo & MASK); }

int qisnegative(q_t a)     { return !!(qhigh(a) & HIGH); }
int qispositive(q_t a)     { return  !(qhigh(a) & HIGH); }
int qisinteger(q_t a)      { return  !qlow(a); }
int qisodd(q_t a)          { return qisinteger(a) &&  (qhigh(a) & 1); }
int qiseven(q_t a)         { return qisinteger(a) && !(qhigh(a) & 1); }
int qless(q_t a, q_t b)    { return a < b; }
int qeqless(q_t a, q_t b)  { return a <= b; }
int qmore(q_t a, q_t b)    { return a > b; }
int qeqmore(q_t a, q_t b)  { return a >= b; }
int qequal(q_t a, q_t b)   { return a == b; }
int qunequal(q_t a, q_t b) { return a != b; }
int qtoi(q_t toi)          { return ((u_t)toi) >> BITS; /**@bug sizeof(int) > sizeof(q_t) */ }
q_t qint(int toq)          { return ((u_t)((d_t)toq)) << BITS; }
q_t qnegate(q_t a)         { return (~(u_t)a) + 1ULL; }
q_t qmin(q_t a, q_t b)     { return qless(a, b) ? a : b; }
q_t qmax(q_t a, q_t b)     { return qmore(a, b) ? a : b; }
q_t qabs(q_t a)            { return qisnegative(a) ? qnegate(a) : a; }
q_t qadd(q_t a, q_t b)     { return qsat((ld_t)a + (ld_t)b); }
q_t qsub(q_t a, q_t b)     { return qsat((ld_t)a - (ld_t)b); }
q_t qcopysign(q_t a, q_t b) { return qisnegative(b) ? qnegate(qabs(a)) : qabs(a); }

q_t qand(q_t a, q_t b)     { return a & b; }
q_t qxor(q_t a, q_t b)     { return a ^ b; }
q_t qor(q_t a, q_t b)      { return a | b; }
q_t qinvert(q_t a)         { return ~a; }
q_t qlrs(q_t a, q_t b)     { return (u_t)a >> (u_t)qtoi(b); }
q_t qlls(q_t a, q_t b)     { return (u_t)a << b; }
q_t qars(q_t a, q_t b)     { return arshift(a, qtoi(b)); } /**@todo handle saturation/edge cases? */
q_t qals(q_t a, q_t b)     { return (u_t)a << b; }         /**@todo handle saturation/edge cases? */

/* <http://www.cplusplus.com/reference/cmath/round/>
	value   round   floor   ceil    trunc
	-----   -----   -----   ----    -----
	 2.3     2.0     2.0     3.0     2.0
	 3.8     4.0     3.0     4.0     3.0
	 5.5     6.0     5.0     6.0     5.0
	-2.3    -2.0    -3.0    -2.0    -2.0
	-3.8    -4.0    -4.0    -3.0    -3.0
	-5.5    -6.0    -6.0    -5.0    -5.0 */
q_t qfloor(q_t q) { 
	return q & ~MASK; 
}

q_t qceil(q_t q) {
	const q_t adj = qisinteger(q) ? qinfo.zero : qinfo.one;
	q = qadd(q, adj);
	return ((u_t)q) & (MASK << BITS);
}

q_t qtrunc(q_t q) {
	const q_t adj = qisnegative(q) && qlow(q) ? qinfo.one : qinfo.zero;
	q = qadd(q, adj);
	return ((u_t)q) & (MASK << BITS);
}

q_t qround(q_t q) {
	const int negative = qisnegative(q);
	q = qabs(q);
	const q_t adj = qlow(q) & HIGH ? qinfo.one : qinfo.zero;
	q = qadd(q, adj);
	q = ((u_t)q) & (MASK << BITS);
	return negative ? qnegate(q) : q;
}

/* Pack/Unpack arrays might also be useful */
int qpack(const q_t *q, char *buffer, size_t length) {
	assert(buffer);
	if(length < sizeof(*q))
		return -1;
	q_t qn = *q;
	uint8_t *b = (uint8_t*)buffer;
	for(size_t i = 0; i < sizeof(qn); i++) {
		b[i] = qn;
		qn = (u_t)qn >> CHAR_BIT;
	}
	return sizeof(qn);
}

int qunpack(q_t *q, const char *buffer, size_t length) {
	assert(q);
	assert(buffer);
	if(length < sizeof(*q))
		return -1;
	uint8_t *b = (uint8_t*)buffer;
	u_t nq = 0;
	for(size_t i = 0; i < sizeof(*q); i++) {
		nq <<= CHAR_BIT;
		nq |= b[sizeof(*q)-i-1];
	}
	*q = nq;
	return sizeof(*q);
}

q_t qmk(long integer, unsigned long fractional) { /**@todo make this the same as the MACRO? */
	const int negative = integer < 0;
	integer = negative ? -integer : integer;
	const q_t r = qcons((d_t)integer, fractional);
	return negative ? qnegate(r) : r;
}

static inline ld_t multiply(q_t a, q_t b) {
	const ld_t dd = ((ld_t)a * (ld_t)b) + (lu_t)HIGH;
	/* NB. portable version of "dd >> BITS", for double width signed values */
	return dd < 0 ? (-1ULL << (2*BITS)) | ((lu_t)dd >> BITS) : ((lu_t)dd) >> BITS;
}

q_t qmul(q_t a, q_t b) {
	return qsat(multiply(a, b));
}

q_t qfma(q_t a, q_t b, q_t c) {
	const ld_t ab = multiply(a, b);
	return qsat(ab + (ld_t)c);
}

q_t qdiv(q_t a, q_t b) { /**< @todo add control of rounding modes */
	ld_t dd = ((ld_t)a) << BITS;
	const d_t bd2 = b / 2;
	if((dd >= 0 && b >= 0) || (dd < 0 && b < 0)) {
		dd += bd2;
	} else {
		dd -= bd2;
	}
	return dd / b;
}

q_t qrem(q_t a, q_t b)
{
	const q_t div = qdiv(a, b);
	return qsub(a, qmul(qtrunc(div), b));
}


/*
q_t qmod(q_t a, q_t b)
{
}

*/

/********* Basic Library Routines ********************************************/
/********* Numeric Input/Output **********************************************/

static char itoch(unsigned ch) {
	assert(ch < 36);
	if(ch <= 9)
		return ch + '0';
	return ch + 'a';
}

static inline void swap(char *a, char *b) {
	assert(a);
	assert(b);
	const int c = *a;
	*a = *b;
	*b = c;
}

static void reverse(char *s, size_t length) {
	assert(s);
	if(length <= 1)
		return;
	for(size_t i = 0; i < length/2; i++)
		swap(&s[i], &s[length - i - 1]);
}

static int uprint(u_t p, char *s, size_t length, int base) {
	assert(s);
	assert(base >= 2 && base <= 36);
	if(length < 2)
		return -1;
	size_t i = 0;
	do {
		unsigned ch = p % base;
		p /= base;
		s[i++] = itoch(ch);
	} while(p && i < length);
	if(p && i >= length)
		return -1;
	reverse(s, i);
	return i;
}

/* <https://codereview.stackexchange.com/questions/109212> */
int qsprint(q_t p, char *s, size_t length) { /**@todo different bases, clean this up */
	assert(s);
	const int negative = qisnegative(p);
	if(negative)
		p = qnegate(p);
	const u_t base = qconf.base ? qconf.base : 10; /* 0 as special case */
	const d_t hi = qhigh(p);
	char frac[BITS+1] = { '.' };
	memset(s, 0, length);
	if(base < 2 || base > 36)
		return -1;
	u_t lo = qlow(p);
	size_t i = 1;
	for(i = 1; lo; i++) {
		if(qconf.dp >= 0 && (int)i >= qconf.dp) /**@todo proper rounding*/
			break;
		lo *= base;
		frac[i] = '0' + (lo >> BITS); /**@bug not valid for all bases */
		lo &= MASK;
	}
	if(negative)
		s[0] = '-';
	const int hisz = uprint(hi, s + negative, length - 1, base);
	if(hisz < 0 || (hisz + i + negative + 1) > length)
		return -1;
	memcpy(s + hisz + negative, frac, i);
	return i + hisz;
}

static inline int extract(unsigned char c, int radix)
{
	c = tolower(c);
	if(c >= '0' && c <= '9')
		c -= '0';
	else if(c >= 'a' && c <= 'z')
		c -= ('a' - 10);
	else
		return -1;
	if(c < radix)
		return c;
	return -1;
}

/**@todo Remove '0x' prefix parsing? This removes the special cases, simplifies
 * the code, removes nasty surprises and means octal handling does not have to
 * be changes (is '0.5' octal or decimal when the base is '0', or is '00.5'
 * octal?) */
long int strntol(const char *str, size_t length, const char **endptr, int *base, int *error, int *is_negative)
{
	assert(str);
	assert(endptr);
	assert(base);
	assert(error);
	assert(is_negative);
	assert((*base >= 2 && *base <= 36) || !*base);
	size_t i = 0;
	bool negate = false, overflow = false, failed = false;
	unsigned long radix = *base, r = 0;

	for(; i < length && str[i]; i++) /* ignore leading white space */
		if(!isspace(str[i]))
			break;
	if(i >= length)
		goto end;

	if(str[i] == '-' || str[i] == '+') { /* Optional '+', and '-' for negative */
		if(str[i] == '-')
			negate = true;
		i++;
	}
	if(i >= length) {
		failed = true;
		goto end;
	}

	radix = *base;
	/* @bug 0 is a valid number, but is not '0x', set endptr and 'error' if this happens */
	if(!*base || *base == 16) { /* prefix 0 = octal, 0x or 0X = hex */
		if(str[i] == '0') {
			if(((i+1) < length) && (str[i + 1] == 'x' || str[i + 1] == 'X')) {
				radix = 16;
				i += 2;
			} else {
				radix = 8;
				i++;
			}
		} else {
			if(!*base)
				radix = 10;
		}
	}
	if(i >= length)
		goto end;

	for(; i < length && str[i]; i++) {
		int e = extract(str[i], radix);
		if(e < 0)
			break;
		unsigned long a = e;
		r = (r*radix) + a;
		if(r > LONG_MAX) /* continue on with conversion */
			overflow = true;
	}
end:
	i = MIN(i, length);
	*endptr = &str[i];
	*base = radix;
	*error = NUMBER_INPUT_OK_E;
	if(overflow) { /* result out of range, set 'error' to indicate this */
		*error = NUMBER_INPUT_RANGE_E;
		r = LONG_MAX;
		if(negate)
			return LONG_MIN;
	}
	if(failed) {
		*error = NUMBER_INPUT_FAILED_E;
		*endptr = str;
	}

	*is_negative = false;
	if(negate) {
		r = -r;
		*is_negative = true;
	}
	return r;
}

/**@bug all allowed formats are not implemented
 * Allowed example numbers; 2.0 -3 +0x5.Af -0377.77 1 0
 * @todo clean this all up, it's a giant mess */
int qnconv(q_t *q, char *s, size_t length) {
	assert(q);
	assert(s);
	int r = 0, base = qconf.base, error = NUMBER_INPUT_OK_E, is_negative = false;
	u_t lo = 0, i = 0, j = 1;
	long hi = 0;
	char frac[BITS+1] = { 0 };
	memset(q, 0, sizeof *q);
	const char *endptr = NULL;
	if(*s != '.') { /**@bug negative -0.X is not handled! */
		/**@todo handle negative and out of range cases */
		hi = strntol(s, length, &endptr, &base, &error, &is_negative);
		if(error < 0 || endptr == s)
			return -1;
		if(!(*endptr))
			goto end;
		if(*endptr != '.')
			return -1;
		if(hi == 0)
			base = qconf.base;
	} else {
		endptr = s;
	}
	if(!base)
		base = 10;

	memcpy(frac, endptr + 1, MIN(sizeof(frac), length - (s - endptr)));
	if(hi > DMAX || hi < DMIN)
		return -1;
	const long radix = base;
	for(i = 0; frac[i]; i++) { /**@todo handle overflow */
		if(i > 5) /*max 5 decimal digits in a 16-bit number: trunc((ln(MAX-VAL+1)/ln(base)) + 1) */
			break;
		const int ch = frac[i] - '0'; /**@bug not valid for all bases */
		if(ch < 0)
			return -1;
		lo = (lo * radix) + ch;
		j *= radix;
	}
	lo = ((lo << BITS) / j);
end:
	*q = qmk(hi, lo);
	return r;
}

int qconv(q_t *q, char *s) {
	assert(s);
	return qnconv(q, s, strlen(s));
}

/********* Numeric Input/Output **********************************************/
/********* CORDIC Routines ***************************************************/

typedef enum {
	CORDIC_MODE_VECTOR_E,
	CORDIC_MODE_ROTATE_E,
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
 */
static int cordic(cordic_coordinates_e coord, cordic_mode_e mode, int iterations, d_t *x0, d_t *y0, d_t *z0) {
	assert(x0);
	assert(y0);
	assert(z0);
	if(mode != CORDIC_MODE_VECTOR_E && mode != CORDIC_MODE_ROTATE_E)
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

	switch(coord) { /**@todo check this, it's probably buggy for linear/hyperbolic*/
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
	for(; j < (unsigned)iterations; i++, j++) {
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
		if(hyperbolic) {
			if(k++ >= 3) {
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
int qcordic(q_t theta, int iterations, q_t *sine, q_t *cosine) {
	assert(sine);
	assert(cosine);

	static const q_t   pi =   QPI,    npi =  -QPI;
	static const q_t  hpi =   QPI/2, hnpi = -(QPI/2);
	static const q_t  qpi =   QPI/4, qnpi = -(QPI/4);
	static const q_t  dpi =   QPI*2, dnpi = -(QPI*2);

	/* convert to range -pi   to pi */
	while(qless(theta, npi)) theta = qadd(theta,  dpi);
	while(qmore(theta,  pi)) theta = qadd(theta, dnpi);

	int negate = 0, shift = 0;

	/* convert to range -pi/2 to pi/2 */
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

	d_t x = cordic_circular_scaling, y = 0, z = theta /* no theta scaling needed */;

	/* CORDIC in Q2.16 format */
	if(cordic(CORDIC_COORD_CIRCULAR_E, CORDIC_MODE_ROTATE_E, iterations, &x, &y, &z) < 0)
		return -1;
	/* correct overflow in cosine */
	//if(x < 0) 
	//	x = -x;


	/* undo shifting and quadrant changes */
	if(shift > 0) {
		const d_t yt = y;
		y =  x;
		x = -yt;
	} else if(shift < 0) {
		const d_t yt = y;
		y = -x;
		x =  yt;
	}

	if(negate) {
		x = -x;
		y = -y;
	}
	/* set output; no scaling needed */
	*cosine = x;
	  *sine = y;
	return 0;
}

q_t qatan(q_t t) {
	q_t x = qint(1), y = t, z = 0;
	cordic(CORDIC_COORD_CIRCULAR_E, CORDIC_MODE_VECTOR_E, -1, &x, &y, &z);
	return z;
}

void qsincos(q_t theta, q_t *sine, q_t *cosine) {
	assert(sine);
	assert(cosine);
	const int r = qcordic(theta, -1, sine, cosine);
	assert(r >= 0);
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

q_t qtan(q_t theta) {
	q_t sine = 0, cosine = 0;
	qsincos(theta, &sine, &cosine);
	return qdiv(sine, cosine); // @todo replace with CORDIC divide?
}

q_t qcot(q_t theta) {
	q_t sine = 0, cosine = 0;
	qsincos(theta, &sine, &cosine);
	return qdiv(cosine, sine); // @todo replace with CORDIC divide?
}

/**@bug only works for small values, and not for all negative inputs, bounds
 * need to be worked out and the sign problem fixed. */
q_t qcordic_mul(q_t a, q_t b) { 
	q_t x = a, y = 0, z = b;
	const int r = cordic(CORDIC_COORD_LINEAR_E, CORDIC_MODE_ROTATE_E, -1, &x, &y, &z);
	assert(r >= 0);
	return y;
}

q_t qcordic_div(q_t a, q_t b) {
	q_t x = b, y = a, z = 0;
	const int r = cordic(CORDIC_COORD_LINEAR_E, CORDIC_MODE_VECTOR_E, -1, &x, &y, &z);
	assert(r >= 0);
	return z;
}

void qsincosh(q_t a, q_t *sinh, q_t *cosh) {
	assert(sinh);
	assert(cosh);
	q_t x = cordic_hyperbolic_scaling, y = 0, z = a; // (e^2x - 1) / (e^2x + 1)
	const int r = cordic(CORDIC_COORD_HYPERBOLIC_E, CORDIC_MODE_ROTATE_E, -1, &x, &y, &z);
	assert(r >= 0);
	*sinh = y;
	*cosh = x;
}

q_t qtanh(q_t a) { /**@todo test -1 once parser is fixed '-0.7615' is result */
	q_t sinh = 0, cosh = 0;
	qsincosh(a, &sinh, &cosh);
	return qdiv(sinh, cosh);
}

q_t qcosh(q_t a) {
	q_t sinh = 0, cosh = 0;
	qsincosh(a, &sinh, &cosh);
	return cosh;
}

q_t qsinh(q_t a) {
	q_t sinh = 0, cosh = 0;
	qsincosh(a, &sinh, &cosh);
	return sinh;
}

/* See: 
 * 	- <https://stackoverflow.com/questions/4657468/fast-fixed-point-pow-log-exp-and-sqrt>
 * 	- <https://www.quinapalus.com/efunc.html> */

q_t qcordic_exp(q_t e) { /**@todo replace with version that works for larger values */
	q_t s = 0, h = 0;
	qsincosh(e, &s, &h);
	return qadd(s, h);
}

q_t qcordic_ln(q_t d) {
	q_t x = qadd(d, qinfo.one), y = qsub(d, qinfo.one), z = 0;
	static const q_t two = QMK(2, 0, 16);
	const int r = cordic(CORDIC_COORD_HYPERBOLIC_E, CORDIC_MODE_VECTOR_E, -1, &x, &y, &z);
	assert(r >= 0);
	return qmul(z, two);
}

/**@todo replace Newton Rhaphson method */
q_t qcordic_sqrt(q_t n) {  /* testing only; works for 0 < x < 2 */
	const q_t quarter = 1uLL << (BITS - 2); /* 0.25 */
	q_t x = qadd(n, quarter), 
	    y = qsub(n, quarter), 
	    z = 0;
	const int r = cordic(CORDIC_COORD_HYPERBOLIC_E, CORDIC_MODE_VECTOR_E, -1, &x, &y, &z);
	assert(r >= 0);
	return qmul(x, cordic_hyperbolic_scaling);
}

q_t qhypot(q_t a, q_t b) {
	q_t x = qabs(a), y = qabs(b), z = 0; /* abs() should not be needed? */
	const int r = cordic(CORDIC_COORD_CIRCULAR_E, CORDIC_MODE_VECTOR_E, -1, &x, &y, &z);
	assert(r >= 0);
	return qmul((x), cordic_circular_scaling);
}

/********* CORDIC Routines ***************************************************/

