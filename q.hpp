/**@file q.hpp
 * @brief Q-Number (Q16.16, signed) library, C++ Wrapper
 * @copyright Richard James Howe (2018)
 * @license MIT
 * @email howe.r.j.89@gmail.com
 * @site <https://github.com/howerj/q> */

#ifndef QCPP_H
#define QCPP_H

#include "q.h"

/**@todo Either expand this to include all functionality, or delete this wrapper...*/
class Q {
	private:
		q_t v;
	public:
		Q(): v(0) { }
		Q(const int x) { v = qint(x); }
		void q(q_t x)  { v = x; }

		operator int() const { return qtoi(v); }

		Q & operator= (const Q &o)  { v = o.v;     return *this; }
		Q & operator= (const int o) { v = qint(o); return *this; }

		Q operator++() { Q r; v = qadd(v, qint(1)); r.v = v; return r; }
		Q operator--() { Q r; v = qsub(v, qint(1)); r.v = v; return r; }

		Q & operator+= (const Q &o) { v = qadd(v, o.v); return *this; }
		Q & operator-= (const Q &o) { v = qsub(v, o.v); return *this; }
		Q & operator*= (const Q &o) { v = qmul(v, o.v); return *this; }
		Q & operator/= (const Q &o) { v = qdiv(v, o.v); return *this; }
		Q & operator&= (const Q &o) { v = qand(v, o.v); return *this; }
		Q & operator|= (const Q &o) { v = qor(v, o.v);  return *this; }
		Q & operator^= (const Q &o) { v = qxor(v, o.v); return *this; }

		const Q operator+ (const Q &o) const { Q r; r.v = qadd(this->v, o.v); return r; }
		const Q operator- (const Q &o) const { Q r; r.v = qsub(this->v, o.v); return r; }
		const Q operator* (const Q &o) const { Q r; r.v = qmul(this->v, o.v); return r; }
		const Q operator/ (const Q &o) const { Q r; r.v = qdiv(this->v, o.v); return r; }
		const Q operator& (const Q &o) const { Q r; r.v = qand(this->v, o.v); return r; }
		const Q operator| (const Q &o) const { Q r; r.v = qor(this->v, o.v);  return r; }
		const Q operator^ (const Q &o) const { Q r; r.v = qxor(this->v, o.v); return r; }
		const Q operator~ () const { Q r; r.v = qinvert(this->v); return r; }
		const Q operator<< (const Q &o) const { Q r; r.v = qals(this->v, o.v); return r; }
		const Q operator>> (const Q &o) const { Q r; r.v = qars(this->v, o.v); return r; }

		const int operator== (const Q &o) const { return   qequal(this->v, o.v); }
		const int operator!= (const Q &o) const { return qunequal(this->v, o.v); }
		const int operator>  (const Q &o) const { return    qmore(this->v, o.v); }
		const int operator>= (const Q &o) const { return  qeqmore(this->v, o.v); }
		const int operator<  (const Q &o) const { return    qless(this->v, o.v); }
		const int operator<= (const Q &o) const { return  qeqless(this->v, o.v); }

		Q abs()  { Q r; r.q(qabs(v)); return r; }
		Q sqrt() { Q r; r.q(qsqrt(v)); return r; }
		Q sin()  { Q r; r.q(qsin(v));  return r; }
		Q cos()  { Q r; r.q(qcos(v));  return r; }
		Q tan()  { Q r; r.q(qtan(v));  return r; }
		Q cot()  { Q r; r.q(qcot(v));  return r; }
		//Q atan() { Q r; r.q(qatan(v)); return r; }
		//Q acos() { Q r; r.q(qacos(v)); return r; }
		//Q asin() { Q r; r.q(qasin(v)); return r; }
		Q cosh() { Q r; r.q(qcosh(v)); return r; }
		Q sinh() { Q r; r.q(qsinh(v)); return r; }
		Q tanh() { Q r; r.q(qtanh(v)); return r; }
		Q log()  { Q r; r.q(qlog(v));  return r; }
		Q exp()  { Q r; r.q(qexp(v));  return r; }
		Q round() { Q r; r.q(qround(v)); return r; }
		Q ceil()  { Q r; r.q(qceil(v));  return r; }
		Q floor() { Q r; r.q(qfloor(v)); return r; }
		Q trunc() { Q r; r.q(qtrunc(v)); return r; }
		Q negate() { Q r; r.q(qnegate(v)); return r; }
		Q degreesToRadians() { Q r; r.q(qdeg2rad(v)); return r; }
		Q radiansToDegrees() { Q r; r.q(qrad2deg(v)); return r; }

		bool negative() { return qisnegative(this->v); }
		bool positive() { return qispositive(this->v); }
		bool odd()      { return qisodd(this->v); }
		bool even()     { return qiseven(this->v); }
		bool integer()  { return qisinteger(this->v); }

		static Q max(const Q &a, const Q &b) { Q r; r.q(qmax(a.v, b.v)); return r; }
		Q max(const Q &a) { Q r; r.q(qmax(a.v, this->v)); return r; }
		static Q min(const Q &a, const Q &b) { Q r; r.q(qmin(a.v, b.v)); return r; }
		Q min(const Q &a) { Q r; r.q(qmin(a.v, this->v)); return r; }

};

#endif
