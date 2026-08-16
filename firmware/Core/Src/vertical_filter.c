/*
 * vertical_filter.c
 *
 *  Created on: Jun 10, 2026
 *      Author: deanstamatakos
 */

#include "vertical_filter.h"

/*
 * ============================================================
 * vertical_filter.c
 * ------------------------------------------------------------
 * Very small vertical estimator.
 *
 * Predict step:
 *   uses acceleration to update altitude and velocity
 *
 * Update step:
 *   uses barometer altitude to correct the estimate
 *
 * This is a good first flight estimator.
 * ============================================================
 */

void VerticalFilter_Init(VerticalFilter_t *f,
                         float init_alt_m,
                         float init_vel_mps,
                         float accel_var,
                         float baro_var)
{
    f->h = init_alt_m;
    f->v = init_vel_mps;

    /* Start with moderate uncertainty */
    f->P00 = 10.0f;
    f->P01 = 0.0f;
    f->P10 = 0.0f;
    f->P11 = 10.0f;

    f->accel_var = accel_var;
    f->baro_var = baro_var;
}

void VerticalFilter_Predict(VerticalFilter_t *f,
                            float accel_mps2,
                            float dt_s)
{
    float dt2 = dt_s * dt_s;

    /*
     * State model:
     *   h = h + v*dt + 0.5*a*dt^2
     *   v = v + a*dt
     */
    f->h = f->h + f->v * dt_s + 0.5f * accel_mps2 * dt2;
    f->v = f->v + accel_mps2 * dt_s;

    /*
     * Covariance update:
     * F = [1 dt; 0 1]
     * G = [0.5 dt^2; dt]
     */
    float P00 = f->P00;
    float P01 = f->P01;
    float P10 = f->P10;
    float P11 = f->P11;

    float FP00 = P00 + dt_s * P10;
    float FP01 = P01 + dt_s * P11;
    float FP10 = P10;
    float FP11 = P11;

    float P00n = FP00 + dt_s * FP01;
    float P01n = FP01;
    float P10n = FP10 + dt_s * FP11;
    float P11n = FP11;

    float g0 = 0.5f * dt2;
    float g1 = dt_s;
    float q = f->accel_var;

    P00n += g0 * g0 * q;
    P01n += g0 * g1 * q;
    P10n += g1 * g0 * q;
    P11n += g1 * g1 * q;

    f->P00 = P00n;
    f->P01 = P01n;
    f->P10 = P10n;
    f->P11 = P11n;
}

void VerticalFilter_UpdateBaro(VerticalFilter_t *f,
                               float baro_alt_m)
{
    /*
     * Measurement model:
     *   z = h + noise
     */
    float y = baro_alt_m - f->h;
    float S = f->P00 + f->baro_var;

    if (S < 1e-6f)
        return;

    float K0 = f->P00 / S;
    float K1 = f->P10 / S;

    /* State correction */
    f->h = f->h + K0 * y;
    f->v = f->v + K1 * y;

    /* Covariance correction */
    float P00 = f->P00;
    float P01 = f->P01;
    float P10 = f->P10;
    float P11 = f->P11;

    f->P00 = (1.0f - K0) * P00;
    f->P01 = (1.0f - K0) * P01;
    f->P10 = P10 - K1 * P00;
    f->P11 = P11 - K1 * P01;
}

float VerticalFilter_GetAltitude(const VerticalFilter_t *f)
{
    return f->h;
}

float VerticalFilter_GetVelocity(const VerticalFilter_t *f)
{
    return f->v;
}
