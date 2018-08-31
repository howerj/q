/**@file q.hpp
 * @brief Q-Number (Q16.16, signed) library, C++ Wrapper
 * @copyright Richard James Howe (2018)
 * @license MIT
 * @email howe.r.j.89@gmail.com
 * @site <https://github.com/howerj/q> */

#ifndef QCPP_H
#define QCPP_H

#include "q.h" 

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

		const Q operator+ (const Q &o) const { Q r; r.v = qadd(this->v, o.v); return r; }
		const Q operator- (const Q &o) const { Q r; r.v = qsub(this->v, o.v); return r; }
		const Q operator* (const Q &o) const { Q r; r.v = qmul(this->v, o.v); return r; }
		const Q operator/ (const Q &o) const { Q r; r.v = qdiv(this->v, o.v); return r; }

		const int operator== (const Q &o) const { return   qequal(this->v, o.v); }
		const int operator!= (const Q &o) const { return qunequal(this->v, o.v); }
		const int operator>  (const Q &o) const { return    qmore(this->v, o.v); }
		const int operator>= (const Q &o) const { return  qeqmore(this->v, o.v); }
		const int operator<  (const Q &o) const { return    qless(this->v, o.v); }
		const int operator<= (const Q &o) const { return  qeqless(this->v, o.v); }
};

#endif
