#ifndef VERTICAL_FILTER_H
#define VERTICAL_FILTER_H

typedef struct
{
    /* Estimated state */
    float h;
    float v;

    /* Covariance matrix P */
    float P00;
    float P01;
    float P10;
    float P11;

    /* Noise tuning */
    float accel_var;
    float baro_var;
} VerticalFilter_t;

void VerticalFilter_Init(VerticalFilter_t *f,
                         float init_alt_m,
                         float init_vel_mps,
                         float accel_var,
                         float baro_var);

void VerticalFilter_Predict(VerticalFilter_t *f,
                            float accel_mps2,
                            float dt_s);

void VerticalFilter_UpdateBaro(VerticalFilter_t *f,
                               float baro_alt_m);

float VerticalFilter_GetAltitude(const VerticalFilter_t *f);
float VerticalFilter_GetVelocity(const VerticalFilter_t *f);

#endif
