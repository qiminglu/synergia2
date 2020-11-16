#ifndef FF_ALGORITHM_H
#define FF_ALGORITHM_H

#include <cmath>
#include <limits>
#include <complex>
#include <stdexcept>

//#include "basic_toolkit/PhysicsConstants.h"
#include "synergia/foundation/physical_constants.h"
#include "synergia/foundation/math_constants.h"

#include <Kokkos_Core.hpp>

#define LIBFF_USE_GSV 1

inline bool close_to_zero(double v)
{ return fabs(v) < 1e-13; }

namespace FF_algorithm
{
    // default yoshida order and steps
    constexpr int default_steps = 6;
    constexpr int default_order = 4;


    template<typename T >
    KOKKOS_INLINE_FUNCTION
    T invsqrt(T const& x) 
    { return 1.0 / sqrt(x); }


    KOKKOS_INLINE_FUNCTION
    constexpr double quiet_nan()
    {
#ifdef Kokkos_ENABLE_CUDA
        return nan("");
#else
        return std::numeric_limits<double>::quiet_NaN();
#endif
    }

    // exact solution for drift spaces
    template <typename T>
    KOKKOS_INLINE_FUNCTION
    void drift_unit
      (T & x, T const& xp, T & y, T const& yp, T & cdt, T const& dpop,
       double length, double reference_momentum, double m, double reference_cdt)
    {
        T uni(1.0);
        T sig((0.0<length) - (length<0.0));

        T vl(length);
        T vm(m);
        T vrm(reference_momentum);
        T vrc(reference_cdt);

        T dp = dpop + uni;
        T inv_npz = uni / sqrt(dp * dp - xp * xp - yp * yp);
        T lxpr = xp * vl * inv_npz;
        T lypr = yp * vl * inv_npz;
        T D2 = lxpr * lxpr + vl * vl + lypr * lypr;
        T p = dp * vrm;
        T E2 = p * p + vm * vm;
        //T beta2 = p*p / E2;
        T ibeta2 = E2 / (p * p);
        x = x + lxpr;
        y = y + lypr;
        //cdt += sqrt(D2 / beta2) - reference_cdt;
        cdt = cdt + sig * sqrt(D2 * ibeta2) - vrc;
    }

    template <typename T>
    KOKKOS_INLINE_FUNCTION
    void slot_unit(
            T & x, T & xp, T & y, T & yp, T & cdt, T & dpop, 
            double ct, double st, double pref, double m)
    {
        const T uni(1.0);

        const T vct(ct);
        const T vst(st);
        const T vpref(pref);
        const T vm(m);

        T r0 = x;
        T r1 = y;
        T r2 = 0.0;

        T zp = sqrt((dpop+uni)*(dpop+uni) - xp * xp - yp * yp);

        T p = (dpop+uni) * vpref;
        T e = sqrt(p*p + vm*vm);

        T b0 = xp * vpref / e;
        T b1 = yp * vpref / e;
        T b2 = zp * vpref / e;

        T bp = vst * b0 + vct * b2;

        T tau = -x * vst / bp;

        r0 = x + tau * b0;
        r1 = y + tau * b1;
        r2 =     tau * b2;

        x = r0 * vct - r2 * vst;
        y = r1;

        cdt = cdt + tau;

        xp = xp * vct - zp * vst;
        yp = yp;
    }

    template <typename T>
    KOKKOS_INLINE_FUNCTION
    void edge_unit
      (T const & y, T & yp, T k)
    {
        yp = yp - k * y;
    }

    template <typename T>
    KOKKOS_INLINE_FUNCTION
    void edge_unit
      (T const & y, T & xp, T & yp, double kx, double ky, char)
    {
        const T vkx(kx);
        const T vky(ky);

        yp = yp - vkx * y;
        xp = xp + vky * y;
    }

    template <typename T>
    KOKKOS_INLINE_FUNCTION
    void edge_unit
      (T const & y, T & xp, T & yp, T const & dpop, T k)
    {
        const T uni(1.0);

        T zp = sqrt((dpop+uni)*(dpop+uni) - xp * xp - yp * yp);

        T xxp = xp;
        T yyp = yp;

        yp = yp - (xxp / zp) * k * y;
        xp = xp + (yyp / zp) * k * y;
    }

    // exact solution for dipole without high order combined functions
    template <typename T>
    KOKKOS_INLINE_FUNCTION
    void dipole_unit
      (T & x, T & xp, T & y, T & yp, T & cdt, T const & dpop, double l, double k0)
    {
        T xp1 = xp - k0 * l / (dpop + 1.0);
        T yp1 = yp;

        T poeb = (dpop + 1.0) / k0;   // p/eB

        x += poeb * ( sqrt(1 - xp1*xp1 - yp1*yp1) - sqrt(1 - xp*xp - yp*yp) );
        y += poeb * yp * ( atan( xp/sqrt(1-xp*xp-yp*yp) ) - atan( xp1/sqrt(1-xp1*xp1-yp1*yp1) ) );

        xp = xp1;
    }

    // exact solution for dipoles, comes from CHEF
    template <typename T>
    KOKKOS_INLINE_FUNCTION
    void bend_complete
      (T & x, T & xp, T & y, T & yp, T & cdt, T const& dpop,
       double dphi, double strength, double p_ref, double m, double cdt_ref,
       Kokkos::complex<double> phase, Kokkos::complex<double> term)
    {
        typedef Kokkos::complex<T> CT;

        const T uni(1.0);

        T p0 = p_ref;
        T p  = p_ref * (dpop + 1.0);
        T E0 = sqrt(p0 * p0 + m * m);
        T E  = sqrt(p * p + m * m);

        T igamma = m / E0;
        T ibeta  = uni / sqrt(uni - igamma * igamma);

        T csq = pconstants::c * pconstants::c * 1e-9;
        T psq = (dpop + 1.0) * (dpop + 1.0);

        T Ef = uni / sqrt(psq + igamma * igamma * ibeta * ibeta);

        T beta1 = Ef * xp;
        T beta2 = Ef * yp;
        T beta3 = Ef * sqrt( (dpop + 1.0) * (dpop + 1.0) - xp * xp - yp * yp );

        CT ui  = CT(0.0, x);
        CT vui = CT(pconstants::c * beta3, pconstants::c * beta1);

        T iomega = E / (csq * strength);

        CT bi = CT(0.0, 1.0) * vui * iomega - ui;
        CT bf = bi * phase + term;

        T rho = pconstants::c * sqrt( beta1 * beta1 + beta3 * beta3 ) * iomega;

        T dthmphi = asin(bi.real() / rho) - asin(bf.real() / rho);

        CT expf = Kokkos::exp( CT(0.0, dthmphi) );
        CT vuf  = vui * expf;
        CT uf   = (ui + bi) * expf - bf;

        T dtheta = dthmphi + dphi;
        T ncdt = - pconstants::c * dtheta * iomega;

        x    = uf.imag();
        y   += beta2 * ncdt;
        cdt += ncdt - cdt_ref;
        xp   = vuf.imag() / (Ef * pconstants::c);
    }

    template <typename T>
    KOKKOS_INLINE_FUNCTION
    void bend_unit(T& x, T& xp, T& y, T& yp, T& cdt, T const& dpop, 
            double theta, double strength, double p_ref, 
            double m, double cdt_ref, 
            Kokkos::complex<double> phase, 
            Kokkos::complex<double> term)
    {
        typedef Kokkos::complex<T> CT;

        const T uni(1.0);
        const T tc(pconstants::c);

        const T vtheta(theta);
        const T vstrength(strength);
        const T vp_ref(p_ref);
        const T vm(m);
        const T vcdt_ref(cdt_ref);

        T p0 = vp_ref;
        T p  = vp_ref * (dpop + uni);
        T E0 = sqrt(p0 * p0 + vm * vm);
        T E  = sqrt(p * p + vm * vm);

        T igamma = vm / E0;
        T ibeta  = uni / sqrt(uni - igamma * igamma);

        T csq = T(pconstants::c * pconstants::c * 1e-9);
        T psq = (dpop + uni) * (dpop + uni);

        T Ef = uni / sqrt(psq + igamma * igamma * ibeta * ibeta);

        T beta1 = Ef * xp;
        T beta2 = Ef * yp;
        T beta3 = Ef * sqrt( (dpop + uni) * (dpop + uni) - xp * xp - yp * yp );

        //CT ui  = CT(0.0, x);
        T ui_r = 0.0;
        T ui_i = x;

        //CT vui = CT(tc * beta3, tc * beta1);
        T vui_r = tc * beta3;
        T vui_i = tc * beta1;

        T iomega = E / (csq * vstrength);

        //CT bi = CT(0.0, 1.0) * vui * iomega - ui;
        T bi_r = -vui_i * iomega - ui_r;
        T bi_i =  vui_r * iomega - ui_i;

        //CT bf = bi * phase + term;
        T phase_r(phase.real()); T phase_i(phase.imag());
        T bf_r = bi_r * phase_r - bi_i * phase_i + T(term.real());
        T bf_i = bi_r * phase_i + bi_i * phase_r + T(term.imag());

        T rho = tc * sqrt( beta1 * beta1 + beta3 * beta3 ) * iomega;

        //T dthmphi = asin(bi.real() / rho) - asin(bf.real() / rho);
        T dthmphi = asin(bi_r / rho) - asin(bf_r / rho);

        //CT expf = Kokkos::exp( CT(0.0, dthmphi) );
        T expf_r = cos(dthmphi);
        T expf_i = sin(dthmphi);

        //CT vuf  = vui * expf;
        T vuf_i = vui_r * expf_i + vui_i * expf_r;

        //CT uf   = (ui + bi) * expf - bf;
        T uf_i = (ui_r+bi_r)*expf_i + (ui_i+bi_i)*expf_r - bf_i;

        T dtheta = dthmphi + vtheta;
        T ncdt = -tc * dtheta * iomega;

        //x = uf.imag();
        x = uf_i;

        //xp = vuf.imag() / (Ef * tc);
        xp = vuf_i / (Ef * tc);

        y   = y + beta2 * ncdt;
        cdt = cdt + ncdt - vcdt_ref;
    }

    KOKKOS_INLINE_FUNCTION
    Kokkos::complex<double> bend_unit_phase(double theta)
    { return Kokkos::exp( Kokkos::complex<double>(0.0, theta) ); }

    KOKKOS_INLINE_FUNCTION
    Kokkos::complex<double> bend_edge_phase(double theta)
    { return Kokkos::exp( Kokkos::complex<double>(0.0, -theta) ); }

    KOKKOS_INLINE_FUNCTION
    Kokkos::complex<double> bend_unit_term(double r0, double theta)
    { return Kokkos::complex<double>(0.0, r0) * Kokkos::complex<double>(1.0 - cos(theta), -sin(theta)); }

    template <typename T>
    KOKKOS_INLINE_FUNCTION
    void bend_edge(T& x, T& xp, T& y, T& yp, T& cdt, T const& dpop, 
            T e, Kokkos::complex<double> phase, 
            T strength, T p_ref, T m)
    {
        typedef Kokkos::complex<T> CT;

        const T uni(1.0);
        const T tc(pconstants::c);

        T p0 = p_ref;
        T p  = p_ref * (dpop + uni);
        T E0 = sqrt(p0 * p0 + m * m);
        T E  = sqrt(p * p + m * m);

        T igamma = m / E0;
        T ibeta  = uni / sqrt(uni - igamma * igamma);

        T csq = T(pconstants::c * pconstants::c * 1e-9);
        T psq = (dpop + uni) * (dpop + uni);

        T Ef = uni / sqrt(psq + igamma * igamma * ibeta * ibeta);

        T beta1 = Ef * xp;
        T beta2 = Ef * yp;
        T beta3 = Ef * sqrt( (dpop + uni) * (dpop + uni) - xp * xp - yp * yp );

        //CT ui  = CT(0.0, x);
        T ui_r = 0.0;
        T ui_i = x;

        //CT vui = CT(pconstants::c * beta3, pconstants::c * beta1);
        T vui_r = tc * beta3;
        T vui_i = tc * beta1;

        T iomega = E / (csq * strength);

        //CT bi = CT(0.0, 1.0) * vui * iomega - ui;
        T bi_r = -vui_i * iomega - ui_r;
        T bi_i =  vui_r * iomega - ui_i;

        //CT bf = bi * phase;
        T phase_r(phase.real()); T phase_i(phase.imag());
        T bf_r = bi_r * phase_r - bi_i * phase_i;
        T bf_i = bi_r * phase_i + bi_i * phase_r;

        T rho = tc * sqrt( beta1 * beta1 + beta3 * beta3 ) * iomega;

        //T dthmphi = asin(bi.real() / rho) - asin(bf.real() / rho);
        T dthmphi = asin(bi_r / rho) - asin(bf_r / rho);

        //CT expf = Kokkos::exp( CT(0.0, dthmphi) );
        T expf_r = cos(dthmphi);
        T expf_i = sin(dthmphi);

        //CT vuf  = vui * expf;
        T vuf_i = vui_r * expf_i + vui_i * expf_r;

        //CT uf   = (ui + bi) * expf - bf;
        T uf_i = (ui_r+bi_r)*expf_i + (ui_i+bi_i)*expf_r - bf_i;

        T dtheta = dthmphi + e;
        T ncdt = - tc * dtheta * iomega;

        //x = uf.imag();
        x = uf_i;

        //xp = vuf.imag() / (Ef * pconstants::c);
        xp = vuf_i / (Ef * tc);

        y = y + beta2 * ncdt;
        cdt = cdt + ncdt;
    }

// thin_cf_quadrupole_unit is not used.  It follows expressions in
// CHEF's InducedKickPropagators which are now contained in
// thin_cf_quadrupole_b{x|y} and retained here for historical reference
    template <typename T>
    KOKKOS_INLINE_FUNCTION
    void thin_cf_quadrupole_unit
      (T const& x, T& xp, T const& y, T& yp, double r0, double const * kL)
    {
        T vk0(kL[0]);
        T vk1(kL[1]);

        T alf = x / r0;

#define MODEL 10

#if MODEL==10
        T Bx = r0 * ( y / (r0 + x) );
        T By = r0 * log(1.0 + alf);
        xp = xp - vk0 * By * (1.0 + alf);
        yp = yp + vk0 * Bx * (1.0 + alf);
#elif MODEL==11
        T Bx = y * (1.0 - alf + alf * alf);
        T By = x * (1.0 - 0.5 * alf + alf * alf / 3.0);
        xp = xp - vk0 * By * (1.0 + alf);
        yp = yp + vk0 * Bx * (1.0 + alf);
#elif MODEL==12
        T Bx = y * (1.0 - alf);
        T By = x * (1.0 - 0.5 * alf);
        xp = xp - vk0 * By * (1.0 + alf);
        yp = yp + vk0 * Bx * (1.0 + alf);
#elif MODEL==13
        T Bx = y;
        T By = x;
        xp = xp - vk0 * By * (1.0 + alf);
        yp = yp + vk0 * Bx * (1.0 + alf);
#elif MODEL==14
        T Bx = y;
        T By = x;
        xp = xp - vk0 * By;
        yp = yp + vk0 * Bx;
#endif
    }

    // the expressions in thin_cf_quadrupole_b{x|y} come from
    // Zolkin, Sector magnets or transverse electromagnetic fields in cylindrical coordinates,
    // Phys.Rev.Accel.Beams 20 (2017) no.4, 043501 Table XIII
    template <typename T>
    KOKKOS_INLINE_FUNCTION
    T thin_cf_quadrupole_bx (T const& x, T const& y, T const& r0)
    { return r0 * y / (r0 + x); }

    template <typename T>
    KOKKOS_INLINE_FUNCTION
    T thin_cf_quadrupole_by (T const& x, T const& y, T const& r0, T const& alf)
    { return r0 * log(T(1.0) + alf); }

   // the expressions in thin_cf_sectupole_b{x|y} come from
    // Zolkin, Sector magnets or transverse electromagnetic fields in cylindrical coordinates,
    // Phys.Rev.Accel.Beams 20 (2017) no.4, 043501 Table XIII
    template <typename T>
    KOKKOS_INLINE_FUNCTION
    T thin_cf_sextupole_bx (T const& x, T const& y, T const& r0, T const& alf)
    { return x * (T(2.0) + alf) * y / (T(1.0) + alf); }

    template <typename T>
    KOKKOS_INLINE_FUNCTION
    T thin_cf_sextupole_by (T const& x, T const& y, T const& r0, T const& alf)
    { return T(0.5) * (x*x + T(2.0)*x*r0) - y*y - r0*r0*log(T(1.0) + alf); }

    // combined function sbends kick up to quadrupole
    template <typename T>
    KOKKOS_INLINE_FUNCTION
    void thin_cf_kick_1
      (T const& x, T& xp, T const& y, T& yp, double r0, double const * kL)
    {
        const T uni(1.0);

        T vk1n(kL[0]);
        T vk1s(kL[1]);

        T vr0(r0);
        T alf = x / vr0;

        xp = xp - vk1n * (uni + alf) * thin_cf_quadrupole_by(x, y, vr0, alf);
        yp = yp + vk1n * (uni + alf) * thin_cf_quadrupole_bx(x, y, vr0);
    }

    // combined function sbends kick up to sextupole
    template <typename T>
    KOKKOS_INLINE_FUNCTION
    void thin_cf_kick_2
      (T const& x, T& xp, T const& y, T& yp, double r0, double const * kL)
    {
        const T uni(1.0);

        T vk1n(kL[0]);
        T vk2n(kL[2]);

        T vr0(r0);
        T alf = x / vr0;

        xp = xp - vk1n * (uni + alf) * thin_cf_quadrupole_by(x, y, vr0, alf)
                - vk2n * (uni + alf) * thin_cf_sextupole_by(x, y, vr0, alf) * T(0.5);

        yp = yp + vk1n * (uni + alf) * thin_cf_quadrupole_bx(x, y, vr0)
                + vk2n * (uni + alf) * thin_cf_sextupole_bx(x, y, vr0, alf) * T(0.5);
    }


    // quadrupole with simple n kicks algorithm. non-Yoshida
    template <typename T>
    KOKKOS_INLINE_FUNCTION
    void quadrupole_chef
      (T & x, T & xp, T & y, T & yp, T & cdt, T const& dpop,
       double pref, double m, double refcdt, double len, double * str, int kicks)
    {
        double substep_ref_cdt = refcdt / kicks;
        double frontLength     = 6.0*(len/4.0)/15.0;
        double sepLength       = ( len - 2.0*frontLength ) / 3.0;
        double kl[2]           = {str[0] * len / 4.0, str[1] * len / 4.0};

        if( kicks == 1 )
        {
            kl[0] = str[0] * len;
            kl[1] = str[1] * len;

            drift_unit( x, xp, y, yp, cdt, dpop, 0.5 * len, pref, m, substep_ref_cdt );
            thin_quadrupole_unit(x, xp, y, yp, kl);  // str*len
            drift_unit( x, xp, y, yp, cdt, dpop, 0.5 * len, pref, m, substep_ref_cdt );
        }
        else if( (kicks % 4) == 0 )
        {
            int    u     = kicks/4;
            double xu    = u;
            frontLength /= xu;
            sepLength   /= xu;
            kl[0]       /= xu;
            kl[1]       /= xu;

            for( int i=0; i<u; ++i)
            {
                drift_unit( x, xp, y, yp, cdt, dpop, frontLength, pref, m, substep_ref_cdt );
                thin_quadrupole_unit(x, xp, y, yp, kl);  // quaterStrength

                for( int i=0; i<3; ++i)
                {
                    drift_unit( x, xp, y, yp, cdt, dpop, sepLength, pref, m, substep_ref_cdt );
                    thin_quadrupole_unit(x, xp, y, yp, kl);  // quaterStrength
                }

                drift_unit( x, xp, y, yp, cdt, dpop, frontLength, pref, m, substep_ref_cdt );
            }
        }
        else
        {
            kl[0] = str[0] * len / kicks;
            kl[1] = str[1] * len / kicks;

            drift_unit( x, xp, y, yp, cdt, dpop, len / (2.0*kicks), pref, m, substep_ref_cdt );
            thin_quadrupole_unit(x, xp, y, yp, kl);  // str*len/kicks

            for( int i=0; i<kicks-1; ++i )
            {
                drift_unit( x, xp, y, yp, cdt, dpop, len / kicks, pref, m, substep_ref_cdt );
                thin_quadrupole_unit(x, xp, y, yp, kl);  // str*len/kicks
            }

            drift_unit( x, xp, y, yp, cdt, dpop, len / (2.0*kicks), pref, m, substep_ref_cdt );
        }
    }

    // thin kick dipole
    template <typename T>
    KOKKOS_INLINE_FUNCTION
    void thin_dipole_unit
      (T const& x, T& xp, T const& y, T& yp, double const * kL)
    {
        xp = xp - T(kL[0]);
        yp = yp + T(kL[1]);
    }

    template <typename T>
    KOKKOS_INLINE_FUNCTION
    void thin_quadrupole_unit
      (T const& x, T& xp, T const& y, T& yp, double const * kL)
    {
        T vk0(kL[0]);
        T vk1(kL[1]);

        xp = xp - vk0 * x + vk1 * y;
        yp = yp + vk0 * y + vk1 * x;
    }

    template <typename T>
    KOKKOS_INLINE_FUNCTION
    void thin_sextupole_unit
      (T const& x, T& xp, T const& y, T& yp, double const * kL)
    {
        T vk0(kL[0]);
        T vk1(kL[1]);

        xp = xp - T(0.5) * vk0 * (x * x - y * y) + vk1 * x * y;
        yp = yp + vk0 * x * y + T(0.5) * vk1 * (x * x - y * y);
    }

    template <typename T>
    KOKKOS_INLINE_FUNCTION
    void thin_octupole_unit
      (T const& x, T& xp, T const& y, T& yp, double const * kL)
    {
        T vk0(kL[0]);
        T vk1(kL[1]);

        T n1(0.5);
        T n2(1.0/3.0);

        xp = xp - n1 * vk0 * (x * x * x * n2 - x * y * y)
                + n1 * vk1 * (x * x * y - y * y * y * n2);
        yp = yp - n1 * vk0 * (y * y * y * n2 - x * x * y)
                + n1 * vk1 * (x * x * x * n2 - x * y * y);
    }

    template <typename T>
    KOKKOS_INLINE_FUNCTION
    void thin_rbend_unit
      (T const& x, T& xp, T const& y, T& yp, double const * kL)
    {
        thin_dipole_unit(x, xp, y, yp, kL);
        thin_quadrupole_unit(x, xp, y, yp, kL + 2);
        thin_sextupole_unit(x, xp, y, yp, 0.0, kL + 4);
    }

    template <typename T>
    KOKKOS_INLINE_FUNCTION
    void rbend_thin_cf_kick
      (T const& x, T& xp, T const& y, T& yp, double r0, double const * kL)
    {
        thin_quadrupole_unit(x, xp, y, yp, kL + 0);
        thin_sextupole_unit(x, xp, y, yp, 0.0, kL + 2);
    }

    template <typename T>
    KOKKOS_INLINE_FUNCTION
    void thin_kicker_unit
      (T & p, double kL)
    {
        p = p + T(kL);
    }

    template <typename T>
    KOKKOS_INLINE_FUNCTION
    void thin_kicker_unit
      (T& xp, T& yp, double const * kL)
    {
        xp = xp + T(kL[0]);
        yp = yp + T(kL[1]);
    }

    template <typename T>
    KOKKOS_INLINE_FUNCTION
    void constfoc_unit
      (T & x, T & xp, double cs, double sn, double beta, double ibeta)
    {
        T vcs(cs), vsn(sn), vbeta(beta), vibeta(ibeta);

        T x1  = x * vcs + xp * vbeta * vsn;
        T xp1 = - x * vsn * vibeta + xp * vcs;

        x = x1;
        xp = xp1;
    }

    KOKKOS_INLINE_FUNCTION
    double thin_rfcavity_pnew
      (double pref, double m, double volt, double phi_s)
    {
        double E0 = sqrt(pref * pref + m * m);
        double E1 = E0 + volt * sin(phi_s);
        return sqrt(E1 * E1 - m * m);
    }

    template <typename T>
    KOKKOS_INLINE_FUNCTION
    void thin_rfcavity_unit
      (T & px, T & py, T const & cdt, T & dpop,
       double w_rf, double volt, double phi_s, double m, 
       double old_ref_p, double new_ref_p, double const * mhp, int nh)
    {
        // 0 strength RF cavity does nothing (fast)
        if (volt == 0.0) {
            return;
        }
        double const anh_phase             = 0.0;
        double       mhp_harmonic_multiple = 1.0;
        double       mhp_relative_strength = 1.0;
        double       mhp_phase_shift       = 0.0;

        double phase_slip_argument = ( cdt * w_rf / pconstants::c ) + anh_phase;

        double strength_factor = 0.0;

        for (int i=0; i<nh; ++i)
        {
            mhp_harmonic_multiple = mhp[i*3  ];
            mhp_relative_strength = mhp[i*3+1];
            mhp_phase_shift       = mhp[i*3+2];

            strength_factor += mhp_relative_strength *
                sin( mhp_harmonic_multiple * (phi_s + phase_slip_argument) + mhp_phase_shift );
        }

        double p = old_ref_p * (dpop + 1.0);
        double E = sqrt(p * p + m * m);

        E += volt * strength_factor;

        px *= old_ref_p / new_ref_p;
        py *= old_ref_p / new_ref_p;

        dpop = sqrt((E-m)*(E+m)) / new_ref_p - 1.0;
    }


    KOKKOS_INLINE_FUNCTION
    double factorial(int n)
    {
        if (n == 0) return 1.0;

        double r = 1;
        for(int i = 1; i <= n; ++i) r *= i;
        return r;
    }

    // general thin magnets of n-th order
    // n = 1, dipole; n = 2, quadrupole; n = 3, sextupole, etc.
    template <typename T>
    KOKKOS_INLINE_FUNCTION
    void thin_magnet_unit
      (T const& x, T& xp, T const& y, T& yp, double const * kL, int n)
    {
        for(int k = 0; k < n; k += 4)
        {
            xp += -kL[0] * (n-k) * std::pow(x, n-k-1) * std::pow(y, k)
                         / (factorial(n-k) * factorial(k));
        }

        for(int k = 2; k < n; k += 4)
        {
            xp += +kL[0] * (n-k) * std::pow(x, n-k-1) * std::pow(y, k)
                         / (factorial(n-k) * factorial(k));
        }

        for(int k = 1; k < n; k += 4)
        {
            xp += +kL[1] * (n-k) * std::pow(x, n-k-1) * std::pow(y, k)
                         / (factorial(n-k) * factorial(k));
        }

        for(int k = 3; k < n; k += 4)
        {
            xp += -kL[1] * (n-k) * std::pow(x, n-k-1) * std::pow(y, k)
                         / (factorial(n-k) * factorial(k));
        }

        for(int k = 4; k <= n; k += 4)
        {
            yp += -kL[0] * k * std::pow(x, n-k) * std::pow(y, k-1)
                         / (factorial(n-k) * factorial(k));
        }

        for(int k = 2; k <= n; k += 4)
        {
            yp += +kL[0] * k * std::pow(x, n-k) * std::pow(y, k-1)
                         / (factorial(n-k) * factorial(k));
        }

        for(int k = 1; k <= n; k += 4)
        {
            yp += +kL[1] * k * std::pow(x, n-k) * std::pow(y, k-1)
                         / (factorial(n-k) * factorial(k));
        }

        for(int k = 3; k <= n; k += 4)
        {
            yp += -kL[1] * k * std::pow(x, n-k) * std::pow(y, k-1)
                         / (factorial(n-k) * factorial(k));
        }
    }

    KOKKOS_INLINE_FUNCTION
    void nllens_unit
        (double const& x, double & xp, double const& y, double & yp, double const&, double const* k)
    {
        const double icnll = k[0];
        const double kick = k[1];

        double xbar = x * icnll;
        double ybar = y * icnll;

        if (ybar == 0 && fabs(xbar) >= 1.0)
        {
            xp = quiet_nan();
            yp = quiet_nan();
            return;
        }

        Kokkos::complex<double> c_i(0.0, 1.0);
        Kokkos::complex<double> c_1(1.0, 0.0);

        Kokkos::complex<double> zeta(xbar, ybar);
        Kokkos::complex<double> croot = sqrt(c_1 - zeta*zeta);

        // Kokkos::complex doesnt provide the log() method, so we
        // need to calculate the following manually:
        // Kokkos::complex<double> carcsin = -c_i * log(c_i * zeta + croot);

        // convert co to polar form
        Kokkos::complex<double> co = c_i * zeta + croot;
        double a = co.real();
        double b = co.imag();
        double r = sqrt(a*a + b*b);
        double theta = (a>0) ? atan(b/a) : atan(b/a) + mconstants::pi;
        Kokkos::complex<double> logco(log(r), theta);
        Kokkos::complex<double> carcsin = -c_i * logco;

        // keep working on the rest
        Kokkos::complex<double> dF = zeta/(croot*croot) + carcsin/(croot*croot*croot);

        double dpx = kick * dF.real();
        double dpy = -kick * dF.imag();

        xp += dpx;
        yp += dpy;
    }

    template <typename T>
    KOKKOS_INLINE_FUNCTION
    void dipedge_unit
        (T & x, T & xp, T & y, T & yp, double re_2_1, double re_4_3, double const * te)
    {
        // linear terms
        xp = xp + T(re_2_1) * x;
        yp = yp + T(re_4_3) * y;

        // quadratic terms
        x  = x  + T(te[0]) * x*x + T(te[1]) * y*y;
        xp = xp + T(te[2]) * x*x + T(te[4]) * y*y + T(te[3]) * x*xp*T(2) + T(te[5]) * y*yp*T(2);
        y  = y  + T(te[6]) * x*y*T(2);
        yp = yp + T(te[7]) * x*y*T(2) + T(te[8]) * x*yp*T(2) + T(te[9]) * y*xp*T(2);
    }


    KOKKOS_INLINE_FUNCTION
    void solenoid_unit
        (double & x, double & xp, double & y, double & yp, double & cdt, double & dpop,
         double ks, double ksl, double length, double ref_p, double mass, double ref_cdt)
    {
        double p2 = (1.0 + dpop) * (1.0 + dpop);
        double zp = sqrt(p2 - xp*xp - yp*yp);
        double dtheta = ksl / zp;

        double sn = sin(dtheta);
        double cs = cos(dtheta);

        double xpi = xp;
        double ypi = yp;

        xp =    cs*xpi + sn*ypi;
        yp = (-sn)*xpi + cs*ypi;

        cs -= 1.0;

        x += (    cs * (-ypi) + sn * xpi ) / ks;
        y += ( (-sn) * (-ypi) + cs * xpi ) / ks;

        double en = sqrt(p2 * ref_p * ref_p + mass * mass);
        double duration = length / (zp * ref_p / en );

        cdt += duration - ref_cdt;
    }

    KOKKOS_INLINE_FUNCTION
    void solenoid_in_edge_kick
        (double const & x, double & xp, double const & y, double & yp, double kse)
    {
        xp += kse * y;
        yp -= kse * x;
    }

    KOKKOS_INLINE_FUNCTION
    void solenoid_out_edge_kick
        (double const & x, double & xp, double const & y, double & yp, double kse)
    {
        xp -= kse * y;
        yp += kse * x;
    }

    // k[6]: 
    // k[0] = beta_b, k[1] = gamma_b, k[2] = beta_e, 
    // k[3] = ioe,    k[4] = l,       k[5] = radius
    KOKKOS_INLINE_FUNCTION
    void elens_kick_gaussian
        (double const& x, double& xp, double const& y, double& yp, double const& dpop, double const* k)
    {
        const double small_radius = 1.0e-10;
        double r = sqrt( x * x + y * y );

        // no kick at r = 0.0
        if (r == 0.0) return;

        const double beta_b  = k[0];
        const double gamma_b = k[1];
        const double beta_e  = k[2];
        const double ioe     = k[3];
        const double l       = k[4];
        const double radius  = k[5];

        double betagamma_p = (1.0 + dpop) * beta_b * gamma_b;
        double beta_p = betagamma_p / sqrt(1.0 + betagamma_p * betagamma_p);

        double factors = -2.0 * ioe * l * pconstants::rp * 
            (1.0 + beta_e * beta_p) / (beta_e * beta_p * beta_b * gamma_b * pconstants::c);

        double kick = 0.0;

        if (r < small_radius)
        {
            kick = factors * ( r / (2.0 * radius * radius) - 
                    (1.0 / 2.0) * ((r*r*r) / (4*radius*radius*radius*radius)) +
                    (1.0 / 6.0) * ((r*r*r*r*r) / (8*radius*radius*radius*radius*radius*radius)) );
        }
        else
        {
            kick = factors * (1.0 - exp( - r * r / (2.0 * radius * radius))) / r;
        }

        xp += kick * x / r;
        yp += kick * y / r;
    }

    KOKKOS_INLINE_FUNCTION
    void elens_kick_uniform
        (double const& x, double& xp, double const& y, double& yp, double const& dpop, double const* k)
    {
        //const double small_radius = 1.0e-10;
        double r = sqrt( x * x + y * y );

        // no kick at r = 0.0
        if (r == 0.0) return;

        const double beta_b  = k[0];
        const double gamma_b = k[1];
        const double beta_e  = k[2];
        const double ioe     = k[3];
        const double l       = k[4];
        const double radius  = k[5];

        double betagamma_p = (1.0 + dpop) * beta_b * gamma_b;
        double beta_p = betagamma_p / sqrt(1.0 + betagamma_p * betagamma_p);

        double factors = -2.0 * ioe * l * pconstants::rp * 
            (1.0 + beta_e * beta_p) / (beta_e * beta_p * beta_b * gamma_b * pconstants::c);

        double kick = factors * r / (radius * radius);

        xp += kick * x / r;
        yp += kick * y / r;
    }




    // utility
    KOKKOS_INLINE_FUNCTION
    int full_drifts_per_step(int order)
    { return std::pow(3.0, (order-2.0)/2.0) * 2; }

    KOKKOS_INLINE_FUNCTION
    int compact_drifts_per_step(int order)
    { return (full_drifts_per_step(order) - 2) / 2 + 2; }

    // general n-th order yoshida
    template<class T>
    using yoshida_kf_t = void(*)(
            T const&, T&, T const&, T&, T const&, double const*);

    template<class T>
    using bend_yoshida_kf_t = void(*)(
            T const&, T&, T const&, T&, double, double const*);

    template <
        typename T,
        yoshida_kf_t<T> kf,
        int n,
        int components >
    struct yoshida_element
    {
        KOKKOS_INLINE_FUNCTION
        static void integral ( T & x, T & xp,
                               T & y, T & yp,
                               T & cdt, T const & dpop,
                               double pref, double m, double step_ref_cdt,
                               double step_length, double const* step_strength,
                               int steps, double c )
        {
            double substep_ref_cdt = step_ref_cdt / 3.0;

            yoshida_element<T, kf, n-1, components>::integral( 
                    x, xp, y, yp, cdt, dpop, pref, m, substep_ref_cdt,
                    step_length, step_strength, steps, c * x1(n) );

            yoshida_element<T, kf, n-1, components>::integral( 
                    x, xp, y, yp, cdt, dpop, pref, m, substep_ref_cdt,
                    step_length, step_strength, steps, c * x0(n) );

            yoshida_element<T, kf, n-1, components>::integral( 
                    x, xp, y, yp, cdt, dpop, pref, m, substep_ref_cdt,
                    step_length, step_strength, steps, c * x1(n) );
        }

        KOKKOS_INLINE_FUNCTION
        static double x1(int nn)
        { return 1.0 / (2.0 - std::pow(2.0, 1.0/(2*nn+1))); }

        KOKKOS_INLINE_FUNCTION
        static double x0(int nn)
        { return 1.0 - 2.0 * x1(nn); }
    };

#if 1
    template <
        typename T,
        yoshida_kf_t<T> kf,
        int components >
    struct yoshida_element < T, kf, 0, components >
    {
        KOKKOS_INLINE_FUNCTION
        static void integral ( T & x, T & xp,
                               T & y, T & yp,
                               T & cdt, T const & dpop,
                               double pref, double m, double step_ref_cdt,
                               double step_length, double const* step_strength,
                               int steps, double c )
        {
            double substep_ref_cdt = step_ref_cdt / 2.0;
            double kl[components * 2];

            for (int i = 0; i < components * 2; ++i)
                kl[i] = step_strength[i] * c;

            drift_unit( x, xp, y, yp, cdt, dpop, 
                    0.5 * c * step_length, pref, m, substep_ref_cdt );

            kf( x, xp, y, yp, cdt, kl );

            drift_unit( x, xp, y, yp, cdt, dpop, 
                    0.5 * c * step_length, pref, m, substep_ref_cdt );
        }
    };

#else

    template <
        typename T,
        yoshida_kf_t<T> kf,
        int components >
    KOKKOS_INLINE_FUNCTION
    struct yoshida_element < T, kf, 0, components >
    {
        KOKKOS_INLINE_FUNCTION
        static void integral ( T & x, T & xp,
                               T & y, T & yp,
                               T & cdt, T const & dpop,
                               double pref, double m, double step_ref_cdt,
                               double step_length, double * step_strength,
                               int steps, double c )
        {
            double substep_ref_cdt = step_ref_cdt;
            double kl[components * 2];

            for (int i = 0; i < components * 2; ++i)
                kl[i] = step_strength[i] * c * 0.5;

            kf( x, xp, y, yp, kl );

            drift_unit( x, xp, y, yp, cdt, dpop, c * step_length, pref, m, substep_ref_cdt );

            kf( x, xp, y, yp, kl );
        }
    };
#endif


    template <
        typename T,
        yoshida_kf_t<T> kf,
        int order,
        int components >
    KOKKOS_INLINE_FUNCTION
    void yoshida( T & x, T & xp,
                  T & y, T & yp,
                  T & cdt, T const & dpop,
                  double pref, double m, double step_ref_cdt,
                  double step_length, double const* step_strength, 
                  int steps )
    {
        const int n = (order - 2) / 2;

        for(int i = 0; i < steps; ++i)
        {
            yoshida_element<T, kf, n, components>::integral( 
                    x, xp, y, yp, cdt, dpop, pref, m, step_ref_cdt,
                    step_length, step_strength, steps, 1.0 );
        }
    }




    // hardwired 2nd order yoshida
    template <
        typename T,
        yoshida_kf_t<T> kf,
        int components >
    KOKKOS_INLINE_FUNCTION
    void yoshida2(T & x, T & xp,
                  T & y, T & yp,
                  T & cdt, T const& dpop,
                  double reference_momentum,
                  double m, double substep_reference_cdt,
                  double step_length, double const* step_strength,
                  int steps)
    {
        for(int i = 0; i < steps; ++i)
        {
            drift_unit(x, xp, y, yp, cdt, dpop, 
                    0.5 * step_length, reference_momentum, 
                    m, substep_reference_cdt);

            kf( x, xp, y, yp, cdt, step_strength );

            drift_unit(x, xp, y, yp, cdt, dpop, 
                    0.5 * step_length, reference_momentum, 
                    m, substep_reference_cdt);
        }
    }

    // hardwired 4th order yoshida
    template <
        typename T,
        yoshida_kf_t<T> kf,
        int components >
    KOKKOS_INLINE_FUNCTION
    void yoshida4(T & x, T & xp,
                  T & y, T & yp,
                  T & cdt, T const& dpop,
                  double reference_momentum,
                  double m, double step_reference_cdt,
                  double step_length, double const* step_strength,
                  int steps)
    {
        // see yoshida4.py for formulas
        const double c1 = 0.675603595979828817023843904487;
        const double c4 = c1;
        const double c2 = -0.175603595979828817023843904487;
        const double c3 = c2;
        const double d1 = 1.35120719195965763404768780897;
        const double d3 = d1;
        const double d2 = -1.70241438391931526809537561795;

        double k1[components * 2], k2[components * 2], k3[components * 2];

        double substep_reference_cdt = step_reference_cdt * 0.25;

        for (int i=0; i<components; ++i)
        {
            k1[i*2+0] = d1 * step_strength[i*2+0];
            k1[i*2+1] = d1 * step_strength[i*2+1];

            k2[i*2+0] = d2 * step_strength[i*2+0];
            k2[i*2+1] = d2 * step_strength[i*2+1];

            k3[i*2+0] = d3 * step_strength[i*2+0];
            k3[i*2+1] = d3 * step_strength[i*2+1];
        }

        for(int i = 0; i < steps; ++i)
        {
            drift_unit(x, xp, y, yp, cdt, dpop, c1 * step_length, reference_momentum,
                       m, substep_reference_cdt);

            //kf( x, xp, y, yp, d1 * step_strength );
            kf( x, xp, y, yp, cdt, k1 );

            drift_unit(x, xp, y, yp, cdt, dpop, c2 * step_length, reference_momentum,
                       m, substep_reference_cdt);

            //kf( x, xp, y, yp, d2 * step_strength );
            kf( x, xp, y, yp, cdt, k2 );

            drift_unit(x, xp, y, yp, cdt, dpop, c3 * step_length, reference_momentum,
                       m, substep_reference_cdt);

            //kf( x, xp, y, yp, d3 * step_strength );
            kf( x, xp, y, yp, cdt, k3 );

            drift_unit(x, xp, y, yp, cdt, dpop, c4 * step_length, reference_momentum,
                       m, substep_reference_cdt);
        }
    }


    // hardwired 6th order yoshida
    template <
        typename T,
        yoshida_kf_t<T> kf,
        int components >
    KOKKOS_INLINE_FUNCTION
    void yoshida6(T & x, T & xp,
                  T & y, T & yp,
                  T & cdt, T const& dpop,
                  double reference_momentum,
                  double m, double step_reference_cdt,
                  double step_length, double const * step_strength,
                  int steps)
    {
        // see yoshida4.py for formulas
        const double c1 = 0.79361246386112147294625603763;
        const double c2 = -0.206276584816439780698721319354;
        const double c3 = -0.118008867881292655922412133144;
        const double c4 = 0.236949573653050744373598734219;

        const double d1 = 1.58722492772224294589251207526;
        const double d2 = -1.99977809735512250728995471397;
        const double d3 = -1.82324266348482825773733634154;
        const double d4 = 2.29714181079092974648453380998;

        // 10 drifts per step
        double substep_reference_cdt = step_reference_cdt * 0.1;

        double k1[components * 2], k2[components * 2], k3[components * 2], k4[components * 2];

        for (int i=0; i<components; ++i)
        {
            k1[i*2+0] = d1 * step_strength[i*2+0];
            k1[i*2+1] = d1 * step_strength[i*2+1];

            k2[i*2+0] = d2 * step_strength[i*2+0];
            k2[i*2+1] = d2 * step_strength[i*2+1];

            k3[i*2+0] = d3 * step_strength[i*2+0];
            k3[i*2+1] = d3 * step_strength[i*2+1];

            k4[i*2+0] = d4 * step_strength[i*2+0];
            k4[i*2+1] = d4 * step_strength[i*2+1];
        }

        for(int i = 0; i < steps; ++i)
        {
            drift_unit(x, xp, y, yp, cdt, dpop, c1 * step_length, reference_momentum,
                       m, substep_reference_cdt);

            kf( x, xp, y, yp, cdt, k1 );

            drift_unit(x, xp, y, yp, cdt, dpop, c2 * step_length, reference_momentum,
                       m, substep_reference_cdt);

            kf( x, xp, y, yp, cdt, k2 );

            drift_unit(x, xp, y, yp, cdt, dpop, c2 * step_length, reference_momentum,
                       m, substep_reference_cdt);

            kf( x, xp, y, yp, cdt, k1 );

            drift_unit(x, xp, y, yp, cdt, dpop, c3 * step_length, reference_momentum,
                       m, substep_reference_cdt);

            kf( x, xp, y, yp, cdt, k3 );

            drift_unit(x, xp, y, yp, cdt, dpop, c4 * step_length, reference_momentum,
                       m, substep_reference_cdt);

            kf( x, xp, y, yp, cdt, k4 );

            drift_unit(x, xp, y, yp, cdt, dpop, c4 * step_length, reference_momentum,
                       m, substep_reference_cdt);

            kf( x, xp, y, yp, cdt, k3 );

            drift_unit(x, xp, y, yp, cdt, dpop, c3 * step_length, reference_momentum,
                       m, substep_reference_cdt);

            kf( x, xp, y, yp, cdt, k1 );

            drift_unit(x, xp, y, yp, cdt, dpop, c2 * step_length, reference_momentum,
                       m, substep_reference_cdt);

            kf( x, xp, y, yp, cdt, k2 );

            drift_unit(x, xp, y, yp, cdt, dpop, c2 * step_length, reference_momentum,
                       m, substep_reference_cdt);

            kf( x, xp, y, yp, cdt, k1 );

            drift_unit(x, xp, y, yp, cdt, dpop, c1 * step_length, reference_momentum,
                       m, substep_reference_cdt);
        }
    }


    namespace yoshida_constants
    {
        // see yoshida4.py for formulas
        namespace bend_yoshida6
        {
            const double c1 = 0.79361246386112147294625603763;
            const double c2 = -0.206276584816439780698721319354;
            const double c3 = -0.118008867881292655922412133144;
            const double c4 = 0.236949573653050744373598734219;

            const double d1 = 1.58722492772224294589251207526;
            const double d2 = -1.99977809735512250728995471397;
            const double d3 = -1.82324266348482825773733634154;
            const double d4 = 2.29714181079092974648453380998;
        }
    }

    template <
        typename T,
        void(kf)(T const & x, T & xp, T const & y, T & yp, double r0, double const * kL),
        int components >
    KOKKOS_INLINE_FUNCTION
    void bend_yoshida4(T & x, T & xp,
                       T & y, T & yp,
                       T & cdt, T const& dpop,
                       double reference_momentum,
                       double m, double step_reference_cdt,
                       double step_angle, double const* step_strength,
                       double r0, double bend_strength,
                       int steps)
    {
#if 0
        // see yoshida4.py for formulas
        const double c1 = 0.675603595979828817023843904487;
        const double c4 = c1;
        const double c2 = -0.175603595979828817023843904487;
        const double c3 = c2;
        const double d1 = 1.35120719195965763404768780897;
        const double d3 = d1;
        const double d2 = -1.70241438391931526809537561795;

        double k1[components * 2], k2[components * 2], k3[components * 2];

        double substep_reference_cdt = step_reference_cdt * 0.25;

        for (int i=0; i<components; ++i)
        {
            k1[i*2+0] = d1 * step_strength[i*2+0];
            k1[i*2+1] = d1 * step_strength[i*2+1];

            k2[i*2+0] = d2 * step_strength[i*2+0];
            k2[i*2+1] = d2 * step_strength[i*2+1];

            k3[i*2+0] = d3 * step_strength[i*2+0];
            k3[i*2+1] = d3 * step_strength[i*2+1];
        }

        static double ssd = 0.0;
        static double sr0 = 0.0;

        static std::complex<double> step_phase[0] = bend_unit_phase(c1 * ssd);
        static std::complex<double> step_phase[1] = bend_unit_phase(c2 * ssd);
        static std::complex<double> step_phase[2] = bend_unit_phase(c3 * ssd);
        static std::complex<double> step_phase[3] = bend_unit_phase(c4 * ssd);

        static std::complex<double> step_term[0] = bend_unit_term(sr0, c1 * ssd);
        static std::complex<double> step_term[1] = bend_unit_term(sr0, c2 * ssd);
        static std::complex<double> step_term[2] = bend_unit_term(sr0, c3 * ssd);
        static std::complex<double> step_term[3] = bend_unit_term(sr0, c4 * ssd);

        double theta = step_angle;

        if (theta != ssd)
        {
            // updates both phase and term
            step_phase[0] = bend_unit_phase(c1 * theta);
            step_phase[1] = bend_unit_phase(c2 * theta);
            step_phase[2] = bend_unit_phase(c3 * theta);
            step_phase[3] = bend_unit_phase(c4 * theta);

            step_term[0] = bend_unit_term(r0, c1 * theta);
            step_term[1] = bend_unit_term(r0, c2 * theta);
            step_term[2] = bend_unit_term(r0, c3 * theta);
            step_term[3] = bend_unit_term(r0, c4 * theta);

            ssd = theta;
            sr0 = r0;
        }
        else if (r0 != sr0)
        {
            // update term only
            step_term[0] = bend_unit_term(r0, c1 * theta);
            step_term[1] = bend_unit_term(r0, c2 * theta);
            step_term[2] = bend_unit_term(r0, c3 * theta);
            step_term[3] = bend_unit_term(r0, c4 * theta);

            sr0 = r0;
        }

        for(int i = 0; i < steps; ++i)
        {
            bend_unit(x, xp, y, yp, cdt, dpop, - c1 * step_angle, bend_strength, reference_momentum,
                       m, substep_reference_cdt, step_phase[0], step_term[0]);

            kf( x, xp, y, yp, r0, k1 );

            bend_unit(x, xp, y, yp, cdt, dpop, - c2 * step_angle, bend_strength, reference_momentum,
                       m, substep_reference_cdt, step_phase[1], step_term[1]);

            kf( x, xp, y, yp, r0, k2 );

            bend_unit(x, xp, y, yp, cdt, dpop, - c3 * step_angle, bend_strength, reference_momentum,
                       m, substep_reference_cdt, step_phase[2], step_term[2]);

            kf( x, xp, y, yp, r0, k3 );

            bend_unit(x, xp, y, yp, cdt, dpop, - c4 * step_angle, bend_strength, reference_momentum,
                       m, substep_reference_cdt, step_phase[3], step_term[3]);
        }
#endif
    }

    KOKKOS_INLINE_FUNCTION
    Kokkos::complex<double> sbend_unit_phase(double c, double delta)
    { return bend_unit_phase(c * delta); }

    KOKKOS_INLINE_FUNCTION
    Kokkos::complex<double> rbend_unit_phase(double c, double delta)
    { return bend_unit_phase(0.0); }

    KOKKOS_INLINE_FUNCTION
    Kokkos::complex<double> sbend_unit_term (double c, double delta, double r0)
    { return bend_unit_term(r0, c * delta); }

    KOKKOS_INLINE_FUNCTION
    Kokkos::complex<double> rbend_unit_term (double c, double delta, double r0)
    { return c * delta; }

    KOKKOS_INLINE_FUNCTION
    double sbend_dphi(double c, double delta)
    { return - c * delta; }

    KOKKOS_INLINE_FUNCTION
    double rbend_dphi(double c, double delta)
    { return 0.0; }

#if 1
    // hardwired 6th order yoshida
    template <
        typename T,
        void(f_kf)(T const & x, T & xp, T const & y, T & yp, double r0, double const * kL),   // kick
        int components >
    KOKKOS_INLINE_FUNCTION
    void bend_yoshida6(T & x, T & xp,
                       T & y, T & yp,
                       T & cdt, T const& dpop,
                       double reference_momentum,
                       double m, double step_reference_cdt,
                       double const * step_strength,
                       double const * dphi,
                       Kokkos::complex<double> const * step_phase,
                       Kokkos::complex<double> const * step_term,
                       double r0, double bend_strength,
                       int steps)
    {
        // see yoshida4.py for formulas
        namespace by6 = yoshida_constants::bend_yoshida6;

        // 10 drifts per step
        double substep_reference_cdt = step_reference_cdt * 0.1;

        double k1[components * 2], k2[components * 2], k3[components * 2], k4[components * 2];

        for (int i=0; i<components; ++i)
        {
            k1[i*2+0] = by6::d1 * step_strength[i*2+0];
            k1[i*2+1] = by6::d1 * step_strength[i*2+1];

            k2[i*2+0] = by6::d2 * step_strength[i*2+0];
            k2[i*2+1] = by6::d2 * step_strength[i*2+1];

            k3[i*2+0] = by6::d3 * step_strength[i*2+0];
            k3[i*2+1] = by6::d3 * step_strength[i*2+1];

            k4[i*2+0] = by6::d4 * step_strength[i*2+0];
            k4[i*2+1] = by6::d4 * step_strength[i*2+1];
        }

        for(int i = 0; i < steps; ++i)
        {
            //drift_unit(x, xp, y, yp, cdt, dpop, c1 * step_length, reference_momentum,
            //          m, substep_reference_cdt);

            bend_unit<T>(x, xp, y, yp, cdt, dpop, dphi[0] , bend_strength, reference_momentum,
                       m, substep_reference_cdt, step_phase[0], step_term[0]);

            f_kf( x, xp, y, yp, r0, k1 );

            //drift_unit(x, xp, y, yp, cdt, dpop, c2 * step_length, reference_momentum,
            //           m, substep_reference_cdt);

            bend_unit<T>(x, xp, y, yp, cdt, dpop, dphi[1], bend_strength, reference_momentum,
                       m, substep_reference_cdt, step_phase[1], step_term[1]);

            f_kf( x, xp, y, yp, r0, k2 );

            //drift_unit(x, xp, y, yp, cdt, dpop, c2 * step_length, reference_momentum,
            //           m, substep_reference_cdt);

            bend_unit<T>(x, xp, y, yp, cdt, dpop, dphi[1], bend_strength, reference_momentum,
                       m, substep_reference_cdt, step_phase[1], step_term[1]);

            f_kf( x, xp, y, yp, r0, k1 );

            //drift_unit(x, xp, y, yp, cdt, dpop, c3 * step_length, reference_momentum,
            //           m, substep_reference_cdt);

            bend_unit<T>(x, xp, y, yp, cdt, dpop, dphi[2], bend_strength, reference_momentum,
                       m, substep_reference_cdt, step_phase[2], step_term[2]);

            f_kf( x, xp, y, yp, r0, k3 );

            //drift_unit(x, xp, y, yp, cdt, dpop, c4 * step_length, reference_momentum,
            //           m, substep_reference_cdt);

            bend_unit<T>(x, xp, y, yp, cdt, dpop, dphi[3], bend_strength, reference_momentum,
                       m, substep_reference_cdt, step_phase[3], step_term[3]);

            f_kf( x, xp, y, yp, r0, k4 );

            //drift_unit(x, xp, y, yp, cdt, dpop, c4 * step_length, reference_momentum,
            //           m, substep_reference_cdt);

            bend_unit<T>(x, xp, y, yp, cdt, dpop, dphi[3], bend_strength, reference_momentum,
                       m, substep_reference_cdt, step_phase[3], step_term[3]);

            f_kf( x, xp, y, yp, r0, k3 );

            //drift_unit(x, xp, y, yp, cdt, dpop, c3 * step_length, reference_momentum,
            //           m, substep_reference_cdt);

            bend_unit<T>(x, xp, y, yp, cdt, dpop, dphi[2], bend_strength, reference_momentum,
                       m, substep_reference_cdt, step_phase[2], step_term[2]);

            f_kf( x, xp, y, yp, r0, k1 );

            //drift_unit(x, xp, y, yp, cdt, dpop, c2 * step_length, reference_momentum,
            //           m, substep_reference_cdt);

            bend_unit<T>(x, xp, y, yp, cdt, dpop, dphi[1], bend_strength, reference_momentum,
                       m, substep_reference_cdt, step_phase[1], step_term[1]);

            f_kf( x, xp, y, yp, r0, k2 );

            //drift_unit(x, xp, y, yp, cdt, dpop, c2 * step_length, reference_momentum,
            //           m, substep_reference_cdt);

            bend_unit<T>(x, xp, y, yp, cdt, dpop, dphi[1], bend_strength, reference_momentum,
                       m, substep_reference_cdt, step_phase[1], step_term[1]);

            f_kf( x, xp, y, yp, r0, k1 );

            //drift_unit(x, xp, y, yp, cdt, dpop, c1 * step_length, reference_momentum,
            //           m, substep_reference_cdt);

            bend_unit<T>(x, xp, y, yp, cdt, dpop, dphi[0], bend_strength, reference_momentum,
                       m, substep_reference_cdt, step_phase[0], step_term[0]);

        }
    }

#endif

};



#endif // FF_ALGORITHM_H
