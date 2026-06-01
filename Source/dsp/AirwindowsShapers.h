/*
    AirwindowsShapers.h — saturation transfer functions ported from Airwindows.

    Saturation algorithms derived from Airwindows (https://github.com/airwindows/airwindows)
    Copyright (c) 2018 Chris Johnson — MIT License.

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

    Each function is a pure, stateless transfer curve operating on `double`. Clamps
    are mandatory where noted — the polynomial series diverge outside their valid
    range. These are designed to be evaluated inside an oversampled region.
*/
#pragma once

#include <cmath>
#include <algorithm>

namespace aw
{
constexpr double kPi     = 3.14159265358979323846;
constexpr double kHalfPi = 1.57079632679489661923;

// The truncated PurestSaturation series peaks at this value (input == clamp 2.0326),
// NOT 1.0 — so normalise by it to make the ceiling a true |y| <= 1.0 soft clip.
constexpr double kPurestPeak = 1.2211401514585818;

// Density3 — continuum saturator. density: 0..5, 1.0 == neutral (bypass).
// <1 thins (antiderivative-style series); >1 thickens (sin() Taylor series).
inline double density3 (double x, double density)
{
    double altered = x;

    if (density > 1.0)
    {
        altered = std::fmax (std::fmin (x * density * kHalfPi, kHalfPi), -kHalfPi);
        const double X = altered * altered;
        double t = altered * X;
        altered -= t / 6.0;        t *= X;
        altered += t / 120.0;      t *= X;
        altered -= t / 5040.0;     t *= X;
        altered += t / 362880.0;   t *= X;
        altered -= t / 39916800.0;
    }
    else if (density < 1.0)
    {
        altered = std::fmax (std::fmin (x, 1.0), -1.0);
        const double polarity = altered;
        double X = x * altered;
        double t = X;
        altered  = t / 2.0;        t *= X;
        altered -= t / 24.0;       t *= X;
        altered += t / 720.0;      t *= X;
        altered -= t / 40320.0;    t *= X;
        altered += t / 3628800.0;
        altered *= (polarity < 0.0 ? -1.0 : 1.0);
    }

    if (density > 2.0)
        return altered;

    const double mix = std::fabs (density - 1.0);
    return x * (1.0 - mix) + altered * mix;
}

// Mojo — gentle level-dependent sine fold ("weight"). x carries its own drive.
inline double mojo (double x)
{
    const double m = std::pow (std::fabs (x), 0.25);
    if (m > 0.0)
        return (std::sin (x * m * kPi * 0.5) / m) * 0.987654321;
    return x;
}

// Spiral2 "presence" — spiral clip using the previous sample's level as the
// scaling denominator. Delaying the normalisation lifts transient/HF content.
inline double spiralPresence (double x, double prev)
{
    const double a = std::fabs (prev);
    return std::sin (x * a) / (a == 0.0 ? 1.0 : a);
}

// PurestSaturation — mantissa-clean sine soft-clip, used as the output guardrail.
// The clamp is mandatory; the series only converges inside the first sine lobe.
// Output is normalised by the series peak so this stage is a true |y| <= 1.0 ceiling.
inline double purestSat (double x, double inGain, double outGain)
{
    x *= inGain;
    x = std::fmax (std::fmin (x, 2.032610446872596), -2.032610446872596);
    const double X = x * x;
    double t = x * X;
    x -= t * 0.125;              t *= X;
    x += t * 0.0078125;          t *= X;
    x -= t * 0.000244140625;     t *= X;
    x += t * 3.814697265625e-6;  t *= X;
    x -= t * 2.98023223876953125e-8;
    x /= kPurestPeak;
    return x * (outGain < 1.0 ? outGain : 1.0);
}
} // namespace aw
