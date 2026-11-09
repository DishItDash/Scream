#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>
#include <assert.h>
#include <dlfcn.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#define OK(expr) do { OSStatus e=(expr); if(e){fprintf(stderr,"%s: %d at %d\n",#expr,(int)e,__LINE__);exit(1);} }while(0)
static int silent=0, callback_count=0;
static OSStatus input(void *ctx, AudioUnitRenderActionFlags *flags,const AudioTimeStamp *ts,UInt32 bus,UInt32 n,AudioBufferList *b) {
    callback_count++;
    (void)ctx;(void)flags;(void)bus;
    for(unsigned ch=0;ch<b->mNumberBuffers;ch++) {
        float *x=b->mBuffers[ch].mData;
        for(unsigned i=0;i<n;i++) x[i]=silent?0:.2f*sinf((ts->mSampleTime+i)*.03f);
    }
    return noErr;
}
static float get(AudioUnit au,unsigned id){Float32 v;OK(AudioUnitGetParameter(au,id,kAudioUnitScope_Global,0,&v));return v;}
static void set(AudioUnit au,unsigned id,float v){OK(AudioUnitSetParameter(au,id,kAudioUnitScope_Global,0,v,0));}
struct AudioRead { AudioUnit au; unsigned id; float value; };
static void *read_audio_parameter(void *ptr) {struct AudioRead *p=ptr;p->value=get(p->au,p->id);return NULL;}
static float audio_get(AudioUnit au,unsigned id) {struct AudioRead r={au,id,0};pthread_t t;assert(!pthread_create(&t,NULL,read_audio_parameter,&r));pthread_join(t,NULL);return r.value;}
int main(int argc,char **argv) {
    assert(argc==2); void *lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
    if(!lib){fprintf(stderr,"%s\n",dlerror());return 1;}
    AudioComponentFactoryFunction factory=(AudioComponentFactoryFunction)dlsym(lib,"GetAUv2PluginFactory");assert(factory);
    AudioComponentDescription desc={kAudioUnitType_Effect,'Scsm','Dshd',0,0};
    AudioComponent comp=AudioComponentRegister(&desc,CFSTR("DishItDash: Scream Custom"),0x10100,factory);assert(comp);
    const double rates[]={44100,48000,88200,96000};
    for(unsigned r=0;r<4;r++) {
        AudioUnit au;OK(AudioComponentInstanceNew(comp,&au));
        AudioStreamBasicDescription fmt={rates[r],kAudioFormatLinearPCM,kAudioFormatFlagsNativeFloatPacked|kAudioFormatFlagIsNonInterleaved,4,1,4,2,32,0};
        OK(AudioUnitSetProperty(au,kAudioUnitProperty_StreamFormat,kAudioUnitScope_Input,0,&fmt,sizeof(fmt)));
        OK(AudioUnitSetProperty(au,kAudioUnitProperty_StreamFormat,kAudioUnitScope_Output,0,&fmt,sizeof(fmt)));
        UInt32 maxframes=1024;OK(AudioUnitSetProperty(au,kAudioUnitProperty_MaximumFramesPerSlice,kAudioUnitScope_Global,0,&maxframes,sizeof(maxframes)));
        AURenderCallbackStruct cb={input,&callback_count};OK(AudioUnitSetProperty(au,kAudioUnitProperty_SetRenderCallback,kAudioUnitScope_Input,0,&cb,sizeof(cb)));
        OK(AudioUnitInitialize(au));
        assert(get(au,14)==0);assert(get(au,15)==.5f);
        Float64 lag=0;UInt32 size=sizeof(lag);OK(AudioUnitGetProperty(au,kAudioUnitProperty_Latency,kAudioUnitScope_Global,0,&lag,&size));
        assert(lag>.08&&lag<.1);
        UInt32 ids[32];size=sizeof(ids);OK(AudioUnitGetProperty(au,kAudioUnitProperty_ParameterList,kAudioUnitScope_Global,0,ids,&size));assert(size==16*sizeof(UInt32));
        set(au,14,.73f);set(au,15,.25f);
        CFPropertyListRef state=NULL;size=sizeof(state);OK(AudioUnitGetProperty(au,kAudioUnitProperty_ClassInfo,kAudioUnitScope_Global,0,&state,&size));assert(state);
        set(au,14,0);set(au,15,1);
        OK(AudioUnitSetProperty(au,kAudioUnitProperty_ClassInfo,kAudioUnitScope_Global,0,&state,sizeof(state)));
        assert(fabsf(get(au,14)-.73f)<1e-6f);assert(get(au,15)==.25f);
        /* Legacy v1.0.12 fixture: 8-byte header, 14 doubles, 16 reserved bytes.
           Reuse a valid state so LFO offsets remain authentic. */
        CFMutableDictionaryRef legacy=CFDictionaryCreateMutableCopy(NULL,0,(CFDictionaryRef)state);
        CFDataRef dataState=CFDictionaryGetValue(legacy,CFSTR("data"));assert(dataState);
        CFMutableDataRef legacyData=CFDataCreateMutableCopy(NULL,0,dataState);
        UInt8 *bytes=CFDataGetMutableBytePtr(legacyData);assert(CFDataGetLength(legacyData)>136);
        bytes[0]=0;bytes[1]=12;bytes[2]=0;bytes[3]=1;
        memset(bytes+8+14*sizeof(double),0,16);
        CFDictionarySetValue(legacy,CFSTR("data"),legacyData);
        CFPropertyListRef legacyState=legacy;
        OK(AudioUnitSetProperty(au,kAudioUnitProperty_ClassInfo,kAudioUnitScope_Global,0,&legacyState,sizeof(legacyState)));
        assert(get(au,14)==0);assert(get(au,15)==.5f);
        CFRelease(legacyData);CFRelease(legacy);CFRelease(state);
        set(au,14,.73f);
        struct {UInt32 count;AudioBuffer b[2];} buffers={2,{{1,0,NULL},{1,0,NULL}}};
        float data[2][1024];double time=0;silent=0;double energy=0;callback_count=0;
        for(unsigned j=0;j<100;j++) {
            unsigned n=(j%4==0)?1:(j%4==1)?127:(j%4==2)?512:1024;
            for(int ch=0;ch<2;ch++){buffers.b[ch].mData=data[ch];buffers.b[ch].mDataByteSize=n*4;}
            AudioTimeStamp ts={0};ts.mFlags=kAudioTimeStampSampleTimeValid;ts.mSampleTime=time;
            AudioUnitRenderActionFlags flags=0;OK(AudioUnitRender(au,&flags,&ts,0,n,(AudioBufferList*)&buffers));
            for(unsigned ch=0;ch<2;ch++)for(unsigned i=0;i<n;i++){assert(isfinite(data[ch][i]));energy+=data[ch][i]*data[ch][i];}
            time+=n;
        }
        assert(callback_count>=100 && energy>0);
        assert(fabsf(audio_get(au,14)-.73f)<1e-6f);assert(audio_get(au,15)==.5f);
        OK(AudioUnitReset(au,kAudioUnitScope_Global,0));silent=1;
        for(unsigned j=0;j<16;j++) {
            AudioTimeStamp ts={0};ts.mFlags=kAudioTimeStampSampleTimeValid;ts.mSampleTime=time;
            for(int ch=0;ch<2;ch++){buffers.b[ch].mData=data[ch];buffers.b[ch].mDataByteSize=4096;}
            AudioUnitRenderActionFlags flags=0;OK(AudioUnitRender(au,&flags,&ts,0,1024,(AudioBufferList*)&buffers));
            for(unsigned ch=0;ch<2;ch++)for(unsigned i=0;i<1024;i++)assert(data[ch][i]==0);
            time+=1024;
        }
        OK(AudioUnitUninitialize(au));OK(AudioComponentInstanceDispose(au));
        printf("PASS AU %.0f Hz: %.3f ms latency, parameters, state, varied blocks, reset\n",rates[r],lag*1000);
    }
}
