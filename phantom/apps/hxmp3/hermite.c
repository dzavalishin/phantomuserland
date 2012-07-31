/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hermite.c 2463 2009-02-13 14:52:05Z haraldkipp $
 * 
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
 * 
 * The contents of this file, and the files included with this file,
 * are subject to the current version of the RealNetworks Public
 * Source License (the "RPSL") available at
 * http://www.helixcommunity.org/content/rpsl unless you have licensed
 * the file under the current version of the RealNetworks Community
 * Source License (the "RCSL") available at
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL
 * will apply. You may also obtain the license terms directly from
 * RealNetworks.  You may not use this file except in compliance with
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for
 * the rights, obligations and limitations governing use of the
 * contents of the file.
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
 * 
 * This file is part of the Helix DNA Technology. RealNetworks is the
 * developer of the Original Code and owns the copyrights in the
 * portions it created.
 * 
 * This file, and the files included with this file, is distributed
 * and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS
 * ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET
 * ENJOYMENT OR NON-INFRINGEMENT.
 * 
 * Technology Compatibility Kit Test Suite(s) Location:
 *    http://www.helixcommunity.org/content/tck
 * 
 * Contributor(s):
 * 
 * ***** END LICENSE BLOCK ***** */

/*
 * Sampling rate conversion, by polynomial interpolation.
 * Ken Cooke (kenc@real.com)
 */
#include <stdlib.h>
//#include <memdebug.h>
#include "hermite.h"

#define MAXRATE		((1<<23) - 1) /* sampling rates cannot exceed MAXRATE */
#define MAXSAMPS	((1<<23) - 1) /* max outsamps for GetMinInput() */
#define MAXCHANS	2

typedef long long SymInt64;

static __inline__ int MulShift31(int x, int y)
{
    SymInt64 a = x;
    SymInt64 b = y;
    a *= b;
    a >>= 31;
    return (int)a;
}


/* interpolator state */
typedef struct { 
	int inrate;
	int outrate;
	int nchans;
	int time_i;
	unsigned int time_f;
	unsigned int step_i;
	unsigned int step_f;
	short hist[3*MAXCHANS];	/* input history */
} STATE;

/* Initialize Hermite resampler
 * 
 * Parameters
 * ----------
 * int inrate              sample rate of input (Hz)
 * int outrate             desired sample rate of output (Hz)
 * int nchans              number of channels
 * 
 * return value            instance pointer which will be passed in all future RAXXXHermite() function calls, 0 if error
 *
 * Notes
 * -----
 * - inrate, outrate, nchans must be within valid ranges (see below)
 * - inrate < outrate (i.e. upsampling only!)
 */
void *
RAInitResamplerHermite(int inrate, int outrate, int nchans)
{
	STATE *s;
	unsigned int step_i, step_f, ratio, rem;
	int i;

	/* validate params */
	if ((inrate <= 0) || (inrate > MAXRATE) ||
            (outrate <= 0) || (outrate > MAXRATE))
		return 0;

        /* only allow downsampling on certain platforms
         * until we have a fixed point resampler that can
         * downsample properly. */
#ifndef _SYMBIAN
        /* XXXgfw remove this when the new resampler is available. */
        if( inrate > outrate )
        {
            return 0;
        }
#endif        
        
	if ((nchans < 1) || (nchans > MAXCHANS))
		return 0;

	/* create interpolator state */
	s = (STATE *) malloc(sizeof(STATE));
	if (!s)
		return 0;

	/* Compute 64-bit timestep, as a signed integer and 32-bit fraction */
	step_i = inrate / outrate;	/* integer part */
	rem = inrate;

	step_f = 0 ;
	for (i = 0; i < 4; i++) {
		rem <<= 8;
		ratio = rem / outrate;
		rem -= ratio * outrate;
		step_f = (step_f << 8) | (ratio & 0xff);	/* 8 more fraction bits */
	}

	s->inrate = inrate;
	s->outrate = outrate;
	s->nchans = nchans;
	s->time_i = 0;
	s->time_f = 0;
	s->step_i = step_i;
	s->step_f = step_f;
	for (i = 0; i < (3*MAXCHANS); i++)
		s->hist[i] = 0;

	return (void *)s;
}

/* Free memory associated with Hermite resampler
 * 
 * Parameters
 * ----------
 * void *inst              instance pointer 
 * 
 * return value            none
 *
 * Notes
 * -----
 */
void
RAFreeResamplerHermite(void *inst)
{
	STATE *s = (STATE *)inst;
	free(s);
}

/* Get max possible outsamps given insamps input
 * 
 * Parameters
 * ----------
 * int insamps             number of input samples
 * void *inst              instance pointer 
 * 
 * return value            maximum number of output samples generated by resampling insamps samples, -1 if error
 *
 * Notes
 * -----
 * - some alternate implementations are included as comments
 *     these might be useful, depending on the target platform
 * - insamps must be even for stereo, function will exit with error if not
 */
int RAGetMaxOutputHermite(int insamps, void *inst)
{
	/* do an empty (null) resample of insamps samples */
	int inframes, outframes;
	unsigned int i, f;
	STATE *s = (STATE *)inst;

	if (s->nchans == 2 && insamps & 0x01)
		return -1;

	inframes = (s->nchans == 2 ? insamps >> 1 : insamps);
	for (i = f = outframes = 0; i < (unsigned int)inframes; outframes++) {
		f += s->step_f;
		i += s->step_i + (f < s->step_f);	/* add with carry */
	}

	return (int)(outframes * s->nchans);
	
    /* equivalent method using __int64 
	 * 	
	 * inframes = (s->nchans == 2 ? insamps >> 1 : insamps);
	 * step64 = ((__int64)s->step_i << 32) + (__int64)s->step_f;
	 * outframes = ( ((__int64)inframes << 32) + step64 - 1) / step64;	COMMENT: ceiling
	 * return (int)(outframes * s->nchans);
	 */

    /* equivalent method using double-precision floats
	 * 
	 * double step;
	 * inframes = (s->nchans == 2 ? insamps >> 1 : insamps);
	 * step = s->step_i + (s->step_f / 4294967296.0);
	 * outframes = (int) ceil((double)inframes / step);
	 * return (outframes * s->nchans);
	 */
}

/* Get minimum number of input samples required to generate outsamps output samples
 * 
 * Parameters
 * ----------
 * int outsamps            number of desired output samples
 * void *inst              instance pointer 
 * 
 * return value            minimum number of input samples required to generate outsamps output samples, -1 if error
 *
 * Notes
 * -----
 * - some alternate implementations are included as comments
 *     these might be useful, depending on the target platform
 * - outsamps must be even for stereo, function will exit with error if not
 */
int
RAGetMinInputHermite(int outsamps, void *inst)
{
	unsigned int outframes;
	STATE *s = (STATE *)inst;
	unsigned int inframes, f, i;

	/* to ensure no overflow in multiply */
	if (outsamps > MAXSAMPS)
		return -1;
	if (s->nchans == 2 && outsamps & 0x01)
		return -1;

	outframes = (unsigned int)(s->nchans == 2 ? outsamps >> 1 : outsamps);
	inframes = 0;

	/* fractional part */
	f = s->step_f;
	for (i = 0; i < 4; i++) {
		inframes += outframes * (f & 0xff);		/* add 24x8 partial product */
		inframes = (inframes + 0xff) >> 8;		/* shift, rounding up */
		f >>= 8;
	}
	/* integer part */
	inframes += outframes * s->step_i;

	return (int)(inframes * s->nchans);

    /* equivalent method using __int64 
	 * 
	 * step64 = ((__int64)s->step_i << 32) + (__int64)s->step_f;
	 * inframes = ( (__int64)outframes * step64);
	 * inframes += (__int64)(0x00000000ffffffff);		COMMENT: (add 1.0 - 2^-32 to 32.32 number)
	 * return (int)((inframes >> 32) * s->nchans);
	 */

    /* equivalent method using double-precision floats
	 * 
	 * double step;
	 * outframes = (s->nchans == 2 ? outsamps >> 1 : outsamps);
	 * step = s->step_i + (s->step_f / 4294967296.0);
	 * inframes = (int) ceil((double)outframes * step);
	 * return (inframes * s->nchans);
	 */

    /* equivalent method using an empty (null) resample 
	 * 
	 * outframes = (s->nchans == 2 ? outsamps >> 1 : outsamps);
	 * for (i = f = currOut = 0; currOut < outframes; currOut++) {
	 * 	f += s->step_f;
	 * 	i += s->step_i + (f < s->step_f);	COMMENT: add with carry 
	 * }
	 * if (f)		COMMENT: ceiling (if any fractional part, round up) 
	 * 	i++;
	 * return (int)(i * s->nchans);
	 */
}

/* Get number of frames of delay in the Hermite resampler
 * 
 * Parameters
 * ----------
 * void *inst              instance pointer 
 * 
 * return value            frames of delay
 *
 * Notes
 * -----
 * - always two frames of delay (2 samples per channel)
 */
int
RAGetDelayHermite(void *inst)
{
	return 2;
}

/* Cubic Hermite interpolation - one channel
 * 
 * Parameters
 * ----------
 * void *inbuf             pointer to buffer of input data (16-bit PCM)
 * int insamps             number of samples in inbuf
 * cvtFunctionType cvt     conversion function pointer, ignored
 * short *outbuf           output buffer, must be large enough to hold RAGetMaxOutputHermite(insamps) samples
 * void *inst              instance pointer 
 * 
 * return value            number of output samples generated and placed in outbuf, -1 if error
 *
 * Notes
 * -----
 * - no restrictions on number of insamps
 * - inbuf MUST contain 16-bit PCM data, the cvt function is ignored
 */
int
RAResampleMonoHermite(void *inbuf, int insamps, short *outbuf, void *inst)
{
	STATE *s = (STATE *)inst;
	unsigned int f, step_i, step_f;
	int outsamps, i, acc0;
	int x0, x1, x2, x3, frac;
	short *inptr;

	/* restore state */
	i = s->time_i;
	f = s->time_f;
	step_i = s->step_i;
	step_f = s->step_f;
	outsamps = 0;
	inptr = (short *)inbuf;

	if (s->nchans != 1)
		return -1;

	/* mono */
	while (i < insamps) {
		
		if (i < 3) {
			x3 = (i < 3 ? s->hist[i+0] : inptr[i-3]) << 12;
			x2 = (i < 2 ? s->hist[i+1] : inptr[i-2]) << 12;
			x1 = (i < 1 ? s->hist[i+2] : inptr[i-1]) << 12;
		} else {
			x3 = inptr[i-3] << 12;
			x2 = inptr[i-2] << 12;
			x1 = inptr[i-1] << 12;
		}
		x0 = inptr[i] << 12;
		frac = f >> 1;

		/* 4-tap Hermite, using Farrow structure */
		acc0 = (3 * (x2 - x1) + x0 - x3) >> 1;
		acc0 = MulShift31(acc0, frac);
		acc0 += 2 * x1 + x3 - ((5 * x2 + x0) >> 1);
		acc0 = MulShift31(acc0, frac);
		acc0 += (x1 - x3) >> 1;
		acc0 = MulShift31(acc0, frac);
		acc0 += x2;

		f += step_f;
		i += step_i + (f < step_f);	/* add with carry */

		acc0 = (acc0 + (1<<11)) >> 12;
		if (acc0 > +32767) acc0 = +32767;
		if (acc0 < -32768) acc0 = -32768;
		outbuf[outsamps++] = (short)acc0;
	}

	/* save delay samples for next time (hist[0] = oldest, hist[2] = newest) */
	s->hist[0] = (insamps < 3 ? s->hist[insamps+0] : inptr[insamps-3]);
	s->hist[1] = (insamps < 2 ? s->hist[insamps+1] : inptr[insamps-2]);
	s->hist[2] = (insamps < 1 ? s->hist[insamps+2] : inptr[insamps-1]);

	/* save state */
	s->time_f = f;
	s->time_i = i - insamps;

	return outsamps;
}

/* Cubic Hermite interpolation - two channels
 * 
 * Parameters
 * ----------
 * void *inbuf             pointer to buffer of input data (16-bit PCM, interleaved LRLRLR...)
 * int insamps             number of samples in inbuf
 * cvtFunctionType cvt     conversion function pointer, ignored
 * short *outbuf           output buffer, must be large enough to hold RAGetMaxOutputHermite(insamps) samples
 * void *inst              instance pointer 
 * 
 * return value            number of output samples generated and placed in outbuf, -1 if error
 *
 * Notes
 * -----
 * - no restrictions on number of insamps
 * - inbuf MUST contain 16-bit PCM data, the cvt function is ignored
 * - insamps must be even, function will exit with error if not
 */
int
RAResampleStereoHermite(void *inbuf, int insamps, short *outbuf, void *inst)
{
	STATE *s = (STATE *)inst;
	unsigned int f, step_i, step_f;
	int outsamps, i, acc0, acc1, j;
	int x0, x1, x2, x3, frac;
	short *inptr;

	/* restore state */
	i = s->time_i;
	f = s->time_f;
	step_i = s->step_i;
	step_f = s->step_f;
	outsamps = 0;
	inptr = (short *)inbuf;

	/* fail if odd number of input samples */
	if (s->nchans != 2 || insamps & 0x01)
		return -1;

	/* stereo - assume insamps is even */
	insamps /= 2;				/* number of stereo frames - consume samples two at a time */
	while (i < insamps) {
		frac = f >> 1;
		j = 2*i;

		/* left */
		if (i < 3) {
			x3 = (i < 3 ? s->hist[j+0] : inptr[j-6]) << 12;
			x2 = (i < 2 ? s->hist[j+2] : inptr[j-4]) << 12;
			x1 = (i < 1 ? s->hist[j+4] : inptr[j-2]) << 12;
		} else {
			x3 = inptr[j-6] << 12;
			x2 = inptr[j-4] << 12;
			x1 = inptr[j-2] << 12;
		}
		x0 = inptr[j] << 12;

		/* 4-tap Hermite, using Farrow structure */
		acc0 = (3 * (x2 - x1) + x0 - x3) >> 1;
		acc0 = MulShift31(acc0, frac);
		acc0 += 2 * x1 + x3 - ((5 * x2 + x0) >> 1);
		acc0 = MulShift31(acc0, frac);
		acc0 += (x1 - x3) >> 1;
		acc0 = MulShift31(acc0, frac);
		acc0 += x2;

		/* right */
		if (i < 3) {
			x3 = (i < 3 ? s->hist[j+1] : inptr[j-5]) << 12;
			x2 = (i < 2 ? s->hist[j+3] : inptr[j-3]) << 12;
			x1 = (i < 1 ? s->hist[j+5] : inptr[j-1]) << 12;
		} else {
			x3 = inptr[j-5] << 12;
			x2 = inptr[j-3] << 12;
			x1 = inptr[j-1] << 12;
		}
		x0 = inptr[j+1] << 12;

		/* 4-tap Hermite, using Farrow structure */
		acc1 = (3 * (x2 - x1) + x0 - x3) >> 1;
		acc1 = MulShift31(acc1, frac);
		acc1 += 2 * x1 + x3 - ((5 * x2 + x0) >> 1);
		acc1 = MulShift31(acc1, frac);
		acc1 += (x1 - x3) >> 1;
		acc1 = MulShift31(acc1, frac);
		acc1 += x2;

		f += step_f;
		i += step_i + (f < step_f);	/* add with carry */

		acc0 = (acc0 + (1<<11)) >> 12;
		if (acc0 > +32767) acc0 = +32767;
		if (acc0 < -32768) acc0 = -32768;
		outbuf[outsamps++] = (short)acc0;

		acc1 = (acc1 + (1<<11)) >> 12;
		if (acc1 > +32767) acc1 = +32767;
		if (acc1 < -32768) acc1 = -32768;
		outbuf[outsamps++] = (short)acc1;
	}

	/* save delay samples for next time (hist[0] = oldest left, hist[1] = oldest right, ...) */
	s->hist[0] = (insamps < 3 ? s->hist[2*(insamps+0) + 0] : inptr[2*(insamps-3) + 0]);
	s->hist[2] = (insamps < 2 ? s->hist[2*(insamps+1) + 0] : inptr[2*(insamps-2) + 0]);
	s->hist[4] = (insamps < 1 ? s->hist[2*(insamps+2) + 0] : inptr[2*(insamps-1) + 0]);

	s->hist[1] = (insamps < 3 ? s->hist[2*(insamps+0) + 1] : inptr[2*(insamps-3) + 1]);
	s->hist[3] = (insamps < 2 ? s->hist[2*(insamps+1) + 1] : inptr[2*(insamps-2) + 1]);
	s->hist[5] = (insamps < 1 ? s->hist[2*(insamps+2) + 1] : inptr[2*(insamps-1) + 1]);

	/* save state */
	s->time_f = f;
	s->time_i = i - insamps;

	return outsamps;
}
