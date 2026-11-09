#include "spectral_smoother.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static void identity(double rate, float amount, float cap, float mixval) {
    SpectralSmoother *s = ss_create(rate); assert(s);
    unsigned lag = ss_latency(s); assert(lag > 0);
    unsigned count = lag * 5;
    float *history = calloc(count, sizeof(float));
    double error = 0;
    for (unsigned i=0;i<count;i++) {
        float wet[2] = {sinf(i*.047f)*.4f, sinf(i*.047f)*.2f};
        float dry[2] = {cosf(i*.023f)*.2f, cosf(i*.023f)*.1f};
        float mix[2] = {mixval,mixval}, out[2];
        history[i] = wet[0]*mixval+dry[0]*(1-mixval);
        ss_process(s,wet,dry,mix,amount,cap,0,out);
        float expected = i>=lag ? history[i-lag] : 0;
        error=fmax(error,fabs(out[0]-expected));
        assert(fabsf(out[1]-.5f*out[0])<1e-5f);
    }
    printf("identity %.0f Hz amount %.1f cap %.1f mix %.1f error %.8g latency %u\n",rate,amount,cap,mixval,error,lag);
    assert(error<2e-5);
    ss_reset(s);
    for(unsigned i=0;i<lag*2;i++) {
        float z[2]={0},m[2]={1,1},o[2]; ss_process(s,z,z,m,1,12,0,o);
        assert(o[0]==0 && o[1]==0);
    }
    free(history); ss_destroy(s);
}
static void tone(double rate, double hz) {
    SpectralSmoother *s=ss_create(rate); assert(s);
    unsigned lag=ss_latency(s); double in_power=0,out_power=0;
    for(unsigned i=0;i<(unsigned)rate*2;i++) {
        float x=sinf(6.283185307179586*hz*i/rate)*.5;
        float w[2]={x,x*.25f},d[2]={0},m[2]={1,1},o[2];
        ss_process(s,w,d,m,1,6,0,o);
        assert(isfinite(o[0])); assert(fabsf(o[1]-.25f*o[0])<1e-5);
        if(i>(unsigned)rate) {in_power+=.25*pow(sin(6.283185307179586*hz*(i-lag)/rate),2);out_power+=o[0]*o[0];}
    }
    double db=10*log10(out_power/in_power);
    printf("tone %.0f Hz @ %.0f: %.2f dB\n",hz,rate,db);
    assert(db < -1 && db > -6.05);
    ss_destroy(s);
}
static void impulse_reconstruction(void) {
    SpectralSmoother *s=ss_create(48000);unsigned lag=ss_latency(s);
    double err=0;
    for(unsigned i=0;i<lag*5;i++) {
        float x=i==lag+17?.5f:0, w[2]={x,-x}, d[2]={0},m[2]={1,1},o[2];
        ss_process(s,w,d,m,1,12,0,o);
        float expected=i==2*lag+17?.5f:0;
        err=fmax(err,fabs(o[0]-expected));assert(fabsf(o[0]+o[1])<1e-6f);
    }
    printf("FFT impulse reconstruction error %.8g\n",err);assert(err<2e-6);ss_destroy(s);
}
static void audition_and_automation(void) {
    SpectralSmoother *normal=ss_create(48000),*listen=ss_create(48000);
    unsigned lag=ss_latency(normal);
    for(unsigned i=0;i<48000;i++) {
        float x=.4f*sinf(i*.08f),w[2]={x,x},d[2]={0},m[2]={1,1},a[2],b[2];
        ss_process(normal,w,d,m,1,6,0,a);ss_process(listen,w,d,m,1,6,1,b);
        if(i>lag*2)assert(fabsf(a[0]+b[0]-.4f*sinf((i-lag)*.08f))<2e-6f);
    }
    for(unsigned i=0;i<48000;i++) {
        float x=.4f*sinf(i*.08f),w[2]={x,x},d[2]={x,x},m[2]={i%17/16.0f,i%17/16.0f},o[2];
        ss_process(normal,w,d,m,i%101/100.0f,i%13,i%137<70,o);
        assert(isfinite(o[0])&&fabsf(o[0])<1);assert(o[0]==o[1]);
    }
    ss_destroy(normal);ss_destroy(listen);puts("PASS audition reconstruction and automation");
}
int main(void) {
    impulse_reconstruction();audition_and_automation();
    double rates[]={44100,48000,88200,96000};
    for(unsigned r=0;r<4;r++) {
        identity(rates[r],0,6,.37f);identity(rates[r],1,0,1);identity(rates[r],1,6,0);
        tone(rates[r],60);tone(rates[r],1800);tone(rates[r],12000);
    }
    puts("PASS: smoother");
}
