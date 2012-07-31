#ifndef _HERMITE_H_

extern void *RAInitResamplerHermite(int inrate, int outrate, int nchans);
extern void RAFreeResamplerHermite(void *inst);

extern int RAGetMaxOutputHermite(int insamps, void *inst);
extern int RAGetMinInputHermite(int outsamps, void *inst);
extern int RAGetDelayHermite(void *inst);
extern int RAResampleMonoHermite(void *inbuf, int insamps, short *outbuf, void *inst);
extern int RAResampleStereoHermite(void *inbuf, int insamps, short *outbuf, void *inst);

#endif

