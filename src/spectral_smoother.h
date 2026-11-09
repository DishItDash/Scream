#pragma once
#include <stdint.h>
typedef struct SpectralSmoother SpectralSmoother;
/* Allocation and FFT planning occur only in create/destroy, never process/reset. */
SpectralSmoother *ss_create(double sample_rate);
void ss_destroy(SpectralSmoother *s);
void ss_reset(SpectralSmoother *s);
uint32_t ss_latency(const SpectralSmoother *s);
void ss_process(SpectralSmoother *s, const float wet[2], const float dry[2],
                const float mix[2], float amount, float max_db, int listen, float output[2]);
