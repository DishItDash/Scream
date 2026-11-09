#include "spectral_smoother.h"
#include "libs/pffft.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

struct SpectralSmoother {
    unsigned n, hop, pos, phase;
    PFFFT_Setup *fft;
    float *memory, *window, *work, *time, *freq[2], *level, *prefix, *reduction, *target;
    float *wet[2], *dry[2], *mix[2], *ola[2];
    float attack, release, ramp, enable, audition;
};
static float clamp(float v,float lo,float hi) {return isfinite(v)?fminf(hi,fmaxf(lo,v)):lo;}
SpectralSmoother *ss_create(double rate) {
    if(!isfinite(rate)||rate<8000||rate>384000) return NULL;
    SpectralSmoother *s=calloc(1,sizeof(*s)); if(!s)return NULL;
    s->n=1024; while(s->n < rate*.08 && s->n<32768) s->n*=2;
    s->hop=s->n/4;
    s->fft=pffft_new_setup((int)s->n,PFFFT_REAL);
    /* Every slice starts on an FFT-size boundary, preserving SIMD alignment. */
    s->memory=pffft_aligned_malloc(sizeof(float)*s->n*17);
    if(!s->fft||!s->memory){ss_destroy(s);return NULL;}
    memset(s->memory,0,sizeof(float)*s->n*17);
    float *p=s->memory;
#define TAKE(field) s->field=p; p+=s->n
    TAKE(window);TAKE(work);TAKE(time);TAKE(freq[0]);TAKE(freq[1]);
    TAKE(level);TAKE(prefix);TAKE(reduction);TAKE(target);
    TAKE(wet[0]);TAKE(wet[1]);TAKE(dry[0]);TAKE(dry[1]);
    TAKE(mix[0]);TAKE(mix[1]);TAKE(ola[0]);TAKE(ola[1]);
#undef TAKE
    for(unsigned i=0;i<s->n;i++) s->window[i]=.5f-.5f*cos(6.283185307179586*i/s->n);
    s->attack=exp(-(double)s->hop/(rate*.010));
    s->release=exp(-(double)s->hop/(rate*.120));
    s->ramp=1.0f/(float)(rate*.010);
    return s;
}
void ss_destroy(SpectralSmoother *s) {
    if(!s)return;
    if(s->fft)pffft_destroy_setup(s->fft);
    if(s->memory)pffft_aligned_free(s->memory);
    free(s);
}
void ss_reset(SpectralSmoother *s) {
    if(!s)return;
    /* Keep FFT plan, window, and scratch; clear all signal-dependent storage. */
    memset(s->reduction,0,sizeof(float)*s->n);
    memset(s->wet[0],0,sizeof(float)*s->n*8);
    s->pos=s->phase=0;s->enable=s->audition=0;
}
uint32_t ss_latency(const SpectralSmoother *s){return s?s->n:0;}
static void frame(SpectralSmoother *s,float amount,float cap) {
    unsigned n=s->n, bins=n/2;
    for(int ch=0;ch<2;ch++) {
        for(unsigned j=0;j<n;j++) s->time[j]=s->wet[ch][(s->pos+j)%n]*s->window[j];
        pffft_transform_ordered(s->fft,s->time,s->freq[ch],s->work,PFFFT_FORWARD);
    }
    s->prefix[0]=0;
    for(unsigned k=0;k<=bins;k++) {
        float peak=0;
        for(int ch=0;ch<2;ch++) {
            float *f=s->freq[ch];
            float mag=k==0?fabsf(f[0]):k==bins?fabsf(f[1]):hypotf(f[2*k],f[2*k+1]);
            peak=fmaxf(peak,mag*4/n);
        }
        s->level[k]=20*log10f(fmaxf(peak,1e-6f));
        s->prefix[k+1]=s->prefix[k]+s->level[k];
    }
    for(unsigned k=0;k<=bins;k++) {
        unsigned radius=(unsigned)fmaxf(4,k*.18f);
        unsigned lo=k>radius?k-radius:0, hi=k+radius<bins?k+radius:bins;
        float envelope=(s->prefix[hi+1]-s->prefix[lo])/(hi-lo+1);
        float excess=fmaxf(0,s->level[k]-envelope-6);
        s->target[k]=s->level[k]>-90?fminf(cap,amount*excess):0;
    }
    for(unsigned k=0;k<=bins;k++) {
        /* Smooth neighboring gains, then smooth in time in dB. */
        float target=(s->target[k? k-1:k]+2*s->target[k]+s->target[k<bins?k+1:k])*.25f;
        float coeff=target>s->reduction[k]?s->attack:s->release;
        s->reduction[k]=fminf(cap,coeff*s->reduction[k]+(1-coeff)*target);
        float gain=powf(10,-s->reduction[k]*.05f);
        for(int ch=0;ch<2;ch++) {
            float *f=s->freq[ch];
            if(k==0)f[0]*=gain;
            else if(k==bins)f[1]*=gain;
            else {f[2*k]*=gain;f[2*k+1]*=gain;}
        }
    }
    for(int ch=0;ch<2;ch++) {
        pffft_transform_ordered(s->fft,s->freq[ch],s->time,s->work,PFFFT_BACKWARD);
        /* Hann squared sums to 1.5 with four equally spaced overlaps. */
        for(unsigned j=0;j<n;j++)s->ola[ch][(s->pos+j)%n]+=s->time[j]*s->window[j]/(1.5f*n);
    }
}
static float approach(float x,float target,float step){return x+clamp(target-x,-step,step);}
void ss_process(SpectralSmoother *s,const float wet[2],const float dry[2],const float mix[2],
                float amount,float max_db,int listen,float output[2]) {
    if(!s){for(int ch=0;ch<2;ch++)output[ch]=wet[ch]*mix[ch]+dry[ch]*(1-mix[ch]);return;}
    amount=clamp(amount,0,1);max_db=clamp(max_db,0,12);
    s->enable=approach(s->enable,amount>0&&max_db>0?1:0,s->ramp);
    s->audition=approach(s->audition,listen?1:0,s->ramp);
    unsigned pos=s->pos;
    for(int ch=0;ch<2;ch++) {
        float oldwet=s->wet[ch][pos], olddry=s->dry[ch][pos], oldmix=s->mix[ch][pos];
        float processed=oldwet+s->enable*(s->ola[ch][pos]-oldwet);
        float normal=oldmix*processed+(1-oldmix)*olddry;
        output[ch]=normal+s->audition*((oldwet-processed)-normal);
        s->ola[ch][pos]=0;
        s->wet[ch][pos]=isfinite(wet[ch])?wet[ch]:0;
        s->dry[ch][pos]=isfinite(dry[ch])?dry[ch]:0;
        s->mix[ch][pos]=clamp(mix[ch],0,1);
    }
    s->pos=(pos+1)%s->n;
    if(++s->phase==s->hop){s->phase=0;frame(s,amount,max_db);}
}
