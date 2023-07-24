# Q: Fixed Point Number Library

| Project   | Q: Fixed Point Number Library     |
| --------- | --------------------------------- |
| Author    | Richard James Howe                |
| Copyright | 2018 Richard James Howe           |
| License   | MIT                               |
| Email     | howe.r.j.89@gmail.com             |
| Website   | <https://github.com/howerj/q>     |

This is a small fixed point number library designed for embedded use. It
implements all of your favorite transcendental functions, plus only the best
basic operators, selected for your calculating pleasure. The format is a signed
[Q16.16][], which is good enough for [Doom][] and good enough for you.

The default [makefile][] target builds the library and a test program, which will
require a [C][] compiler (and Make). The library is small enough that is can be
easily modified to suite your purpose. The dependencies are minimal; they are few
string handling functions, and '[tolower][]' for numeric input. This should allow
the code to ported to the platform of your choice. The 'run' make target builds
the test program (called 'q') and runs it on some input. The '-h' option will
spit out a more detailed help.

I would compile the library with the '-fwrapv' option enabled, you might
be some kind of Maverick who doesn't play by no rules however.

The trigonometric functions, and some others, are implemented internally with
[CORDIC][].

Of note, all operators are bounded by minimum and maximum values, which are not
shown in the following table, by default all arithmetic is saturating. The
effective input range of a number might lower than what is possible given a
mathematical functions definition - either because of the limited range of the
[Q16.16][] type, or because the implementation of a function introduces too
much error along some part of its' input range. Caveat Emptor (although you're
not exactly paying for this library now, are you? Caveat Auditor perhaps).

( This table needs completing, specifically the input ranges... )

| C Function    | Operator    | Input Range | Asserts  | Notes                                           |
| ------------- | ----------- | ----------- | -------- | ----------------------------------------------- |
| qadd(a, b)    | a +   b     |             |          | Addition                                        |
| qsub(a, b)    | a \-  b     |             |          | Subtraction                                     |
| qdiv(a, b)    | a /   b     | b != 0      | Yes      | Division                                        |
| qmul(a, b)    | a \*  b     |             |          | Multiplication                                  |
| qrem(a, b)    | a rem b     | b != 0      | Yes      | Remainder: remainder after division             |
| qmod(a, b)    | a mod b     | b != 0      | Yes      | Modulo                                          |
| qsin(theta)   | sin(theta)  |             |          | Sine                                            |
| qcos(theta)   | cos(theta)  |             |          | Cosine                                          |
| qtan(theta)   | tan(theta)  |             |          | Tangent                                         |
| qcot(theta)   | cot(theta)  |             |          | Cotangent                                       |
| qasin(x)      | asin(x)     | abs(x) <= 1 | Yes      | Arcsine                                         |
| qacos(x)      | acos(x)     | abs(x) <= 1 | Yes      | Arccosine                                       |
| qhypot(a, b)  | hypot(a, b) |             |          | Hypotenuse; sqrt(a\*a + b\*b)                   |
| qatan(t)      | atan(t)     |             |          | Arctangent                                      |
| qsinh(a)      | sinh(a)     |             |          | Hyperbolic Sine                                 |
| qcosh(a)      | cosh(a)     |             |          | Hyperbolic Cosine                               |
| qtanh(a)      | tanh(a)     |             |          | Hyperbolic Tangent                              |
| qexp(e)       | exp(e)      | e < ln(MAX) | No       | Exponential function, High error for 'e' > ~7.  |
| qlog(n)       | log(n)      | n >  0      | Yes      | Natural Logarithm                               |
| qsqrt(x)      | sqrt(x)     | n >= 0      | Yes      | Square Root                                     |
| qround(q)     | round(q)    |             |          | Round to nearest value                          |
| qceil(q)      | ceil(q)     |             |          | Round up value                                  |
| qtrunc(q)     | trunc(q)    |             |          | Truncate value                                  |
| qfloor(q)     | floor(q)    |             |          | Round down value                                |
| qnegate(a)    | -a          |             |          | Negate a number                                 |
| qabs(a)       | abs(a)      |             |          | Absolute value of a number                      |
| qfma(a, b, c) | (a\*b)+c    |             |          | Fused Multiply Add, uses Q32.32 internally      |
| qequal(a, b)  | a == b      |             |          |                                                 |
| qsignum(a)    | signum(a)   |             |          | Signum function                                 |
| qsign(a)      | sgn(a)      |             |          | Sign function                                   |
|               |             |             |          |                                                 |

For the round/ceil/trunc/floor functions the following table from the
[cplusplus.com][] helps:

| value | round | floor | ceil | trunc |
| ----- | ----- | ----- | ---- | ----- |
|   2.3 |   2.0 |   2.0 |  3.0 |   2.0 |
|   3.8 |   4.0 |   3.0 |  4.0 |   3.0 |
|   5.5 |   6.0 |   5.0 |  6.0 |   5.0 |
|  -2.3 |  -2.0 |  -3.0 | -2.0 |  -2.0 |
|  -3.8 |  -4.0 |  -4.0 | -3.0 |  -3.0 |
|  -5.5 |  -6.0 |  -6.0 | -5.0 |  -5.0 |

Have fun with the adding, and subtracting, and stuff, I hope it goes well. It
would be cool to make an [APL][] interpreter built around this library. Testing
would become much easier as you could use programming language constructs to
create new tests over larger ranges of numbers.

[APL]: https://en.wikipedia.org/wiki/APL_(programming_language)
[Doom]: https://en.wikipedia.org/wiki/Doom_(1993_video_game)
[tolower]: http://www.cplusplus.com/reference/cctype/tolower/
[makefile]: https://en.wikipedia.org/wiki/Make_(software)
[C]: https://en.wikipedia.org/wiki/C_%28programming_language%29
[cplusplus.com]: http://www.cplusplus.com/reference/cmath/round/
[Q16.16]: https://en.wikipedia.org/wiki/Fixed-point_arithmetic
[CORDIC]: https://en.wikipedia.org/wiki/CORDIC
[VHDL]: https://en.wikipedia.org/wiki/VHDL
[FPGA]: https://en.wikipedia.org/wiki/Field-programmable_gate_array

<style type="text/css">body{margin:40px auto;max-width:850px;line-height:1.6;font-size:16px;color:#444;padding:0 10px}h1,h2,h3{line-height:1.2}</style>
