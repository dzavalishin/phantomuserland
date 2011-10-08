#if 0
// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//	System interface for sound.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: i_unix.c,v 1.5 1997/02/03 22:45:10 b1 Exp $";

#include "sounds.h"

char MsgText[256];
void WriteDebug(char *);

#define NUM_SOUND_FX 128
#define SB_SIZE      20480

void CreateSoundBuffer(int Channel, int length, unsigned char *data);
void I_PlaySoundEffect(int sfxid, int Channel, int volume, int pan);
void DS_Error( HRESULT hresult, char *msg );

LPDIRECTSOUND8       lpDS;
LPDIRECTSOUNDBUFFER  lpMix[2];
extern int gametic;
int swap_stereo;

static int mixbuff=0; 

// Needed for calling the actual sound output.
#define SAMPLECOUNT   512
#define NUM_CHANNELS  16
// It is 2 for 16bit, and 2 for two channels.
#define BUFMUL        4
#define MIXBUFFERSIZE (SAMPLECOUNT*BUFMUL)

#define SAMPLERATE	  11025	// Hz
#define SAMPLESIZE	  2   	// 16bit

extern HWND win;

void I_CreateSound(void)
{
  HRESULT        hret;
  int            buff;
  DSBUFFERDESC   dsbd;
  WAVEFORMATEX   wfx;

  hret = DirectSoundCreate8(NULL, &lpDS, NULL);
  if (hret != DS_OK)
  {
     //printf("failed DirectSoundCreate");
     return;
  }

  hret = lpDS->lpVtbl->SetCooperativeLevel(lpDS, win, DSSCL_PRIORITY);
  if (hret != DS_OK)
       //printf("failled DirectSound.SetCooperativeLevel");

  memset( &wfx,0, sizeof(WAVEFORMATEX) ); 
  wfx.wFormatTag      = WAVE_FORMAT_PCM; 
  wfx.nChannels       = 2; 
  wfx.nSamplesPerSec  = 11025;
  wfx.wBitsPerSample  = 16; 
  wfx.nBlockAlign     = 4;
  wfx.nAvgBytesPerSec = 44100;

  memset(&dsbd,0,sizeof(DSBUFFERDESC));
  dsbd.dwSize        = sizeof(DSBUFFERDESC);
  dsbd.dwFlags       = 0;
  dsbd.dwBufferBytes = MIXBUFFERSIZE;
  dsbd.lpwfxFormat   = &wfx;

  hret=lpDS->lpVtbl->CreateSoundBuffer(lpDS,&dsbd, &lpMix[0], NULL);
  hret=lpDS->lpVtbl->CreateSoundBuffer(lpDS,&dsbd, &lpMix[1], NULL);


  return;
};

void ShutdownDirectSound(void)
{
  int buff;
  DWORD BufferStatus;

  if (lpMix[0] !=NULL) 
     lpMix[0]->lpVtbl->Release(lpMix[0]);
  if(lpDS != NULL)
     lpDS->lpVtbl->Release(lpDS);
}

void I_SubmitSound(signed short *mixbuffer)
{
  DWORD hret;
  void* pPtr1=NULL,*pPtr2=NULL;
  DWORD dwSize1=0,dwSize2=0;

  hret = lpMix[mixbuff]->lpVtbl->Stop(lpMix[mixbuff]);
  hret = lpMix[mixbuff]->lpVtbl->SetCurrentPosition(lpMix[mixbuff],0);
 
  hret=lpMix[mixbuff]->lpVtbl->Lock(lpMix[mixbuff],0,MIXBUFFERSIZE,&pPtr1,
                           &dwSize1,&pPtr2,&dwSize2,0);
  if (hret!=DS_OK)
  {	//printf("Error locking on play start");
 	return ;
  }
  memcpy(pPtr1, (void*)mixbuffer, MIXBUFFERSIZE);
  hret-lpMix[mixbuff]->lpVtbl->Unlock(lpMix[mixbuff],pPtr1, dwSize1, pPtr2, dwSize2);
  hret = lpMix[mixbuff]->lpVtbl->Play(lpMix[mixbuff],0,0,0);
  mixbuff= (mixbuff+1)&1;

};

#endif
