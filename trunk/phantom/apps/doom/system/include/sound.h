
#ifndef _SOUND_H_
#define _SOUND_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define SOUND_VERSION 0x0100
#define PCM_ALL       0

#define PCM_OUT       0x08000000
#define PCM_RING      0x10000000
#define PCM_STATIC    0x20000000
#define PCM_FLOAT     0x40000000
#define PCM_FILTER    0x80000000

#define PCM_2_16_48   1
#define PCM_1_16_48   2
#define PCM_2_16_44   3
#define PCM_1_16_44   4
#define PCM_2_16_32   5
#define PCM_1_16_32   6
#define PCM_2_16_24   7
#define PCM_1_16_24   8
#define PCM_2_16_22   9
#define PCM_1_16_22  10
#define PCM_2_16_16  11
#define PCM_1_16_16  12
#define PCM_2_16_12  13
#define PCM_1_16_12  14
#define PCM_2_16_11  15
#define PCM_1_16_11  16
#define PCM_2_16_8   17
#define PCM_1_16_8   18
#define PCM_2_8_48   19
#define PCM_1_8_48   20
#define PCM_2_8_44   21
#define PCM_1_8_44   22
#define PCM_2_8_32   23
#define PCM_1_8_32   24
#define PCM_2_8_24   25
#define PCM_1_8_24   26
#define PCM_2_8_22   27
#define PCM_1_8_22   28
#define PCM_2_8_16   29
#define PCM_1_8_16   30
#define PCM_2_8_12   31
#define PCM_1_8_12   32
#define PCM_2_8_11   33
#define PCM_1_8_11   34
#define PCM_2_8_8    35
#define PCM_1_8_8    36

#define SRV_GETVERSION      0
#define SND_CREATE_BUFF     1
#define SND_DESTROY_BUFF    2
#define SND_SETFORMAT       3
#define SND_GETFORMAT       4
#define SND_RESET           5
#define SND_SETPOS          6
#define SND_GETPOS          7
#define SND_SETBUFF         8
#define SND_OUT             9
#define SND_PLAY           10
#define SND_STOP           11
#define SND_SETVOLUME      12
#define SND_GETVOLUME      13
#define SND_SETPAN         14
#define SND_GETPAN         15
#define SND_GETBUFFSIZE    16

#define PLAY_SYNC     0x80000000

typedef unsigned int SNDBUF;

int _stdcall  InitSound(int *version);

int _stdcall  CreateBuffer(unsigned int format,int size,SNDBUF *buf);
int _stdcall  DestroyBuffer(SNDBUF hBuff);

int _stdcall  SetFormat(SNDBUF hBuff, unsigned int format);
int _stdcall  GetFormat(SNDBUF hBuff, unsigned int *format);

int _stdcall  ResetBuffer(SNDBUF hBuff, unsigned int flags);
int _stdcall  SetBufferPos(SNDBUF hBuff, int offset);
int _stdcall  GetBufferPos(SNDBUF hBuff, int *offset);
int _stdcall  GetBufferSize(SNDBUF hBuff, int *size);

int _stdcall  SetBuffer(SNDBUF hBuff,void* buff,
                        int offs, int size);
int _stdcall  WaveOut(SNDBUF hBuff,void *buff, int size);
int _stdcall  PlayBuffer(SNDBUF hBuff,unsigned int flags);
int _stdcall  StopBuffer(SNDBUF hBuff);

int _stdcall  SetVolume(SNDBUF hBuff, int left, int right);
int _stdcall  GetVolume(SNDBUF hBuff, int *left, int *right);
int _stdcall  SetPan(SNDBUF hBuff, int pan);
int _stdcall  GetPan(SNDBUF hBuff, int *pan);

int _stdcall  GetMasterVol(int* vol);
int _stdcall  SetMasterVol(int vol);


typedef struct
{
    unsigned int   riff_id;
    unsigned int   riff_size;
    unsigned int   riff_format;

    unsigned int   fmt_id;
    unsigned int   fmt_size;

    unsigned short int wFormatTag;
    unsigned short int nChannels;
    unsigned int   nSamplesPerSec;
    unsigned int   nAvgBytesPerSec;
    unsigned short int nBlockAlign;
    unsigned short int wBitsPerSample;
    unsigned int   data_id;
    unsigned int   data_size;
} WAVEHEADER;


unsigned int _stdcall test_wav(WAVEHEADER *hdr);

#ifdef __cplusplus
extern "C"
}
#endif

#endif //_SOUND_H_
