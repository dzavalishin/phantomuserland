/*
 * Copyright (c) 1998-2001 Yoshihide SONODA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef _WAVE_FMT_H_
#define _WAVE_FMT_H_

#include <sys/types.h>


#define H_RIFF (*(int *)"RIFF")
#define H_WAVE (*(int *)"WAVE")
#define H_DATA (*(int *)"data")
#define H_FMT  (*(int *)"fmt ")

/* 構造体定義 (ref. MS-Windows mmsystem.h) */
typedef struct tWAVEFORMAT
{
    u_int16_t wFormatTag;
    u_int16_t nChannels;
    u_int32_t nSamplesPerSec;
    u_int32_t nAvgBytesPerSec;
    u_int16_t nBlockAlign;
    u_int16_t wBitsPerSample;
} WAVEFORMAT, *PWAVEFORMAT;

typedef struct tWAVEFORMATEX
{
    u_int16_t wFormatTag;
    u_int16_t nChannels;
    u_int32_t nSamplesPerSec;
    u_int32_t nAvgBytesPerSec;
    u_int16_t nBlockAlign;
    u_int16_t wBitsPerSample;
    u_int16_t cbSize;
} WAVEFORMATEX, *PWAVEFORMATEX;

#endif /* _WAVE_FMT_H_ */
