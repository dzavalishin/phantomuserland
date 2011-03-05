/* this file is a part of amp software, (C) tomislav uzelac 1996,1997
*/
 
/* formats.c  put functions for different audio formats in here. currently
 *	      only .wav, .au would be fine  
 * Created by: tomislav uzelac  May 1996
 */
#include "audio.h"

#define FORMATS
#include "formats.h"

#define FilePutChar(__c, __f) do { char c = __c; write( __f, &c, 1 ); } while(0)

/* leave room for .wav header
*/
void wav_begin(void)
{
	FileWrite("",1,44,out_fd);
}

/* this is not proper really, but works!
*/
void wav_end(struct AUDIO_HEADER *header)
{
unsigned char ispred[20]={0x52 ,0x49 ,0x46 ,0x46 ,0xfc ,0x59  ,0x4  ,0x0 ,0x57
 ,0x41 ,0x56 ,0x45 ,0x66 ,0x6d ,0x74 ,0x20 ,0x10  ,0x0  ,0x0  ,0x0 };
unsigned char iza[8]={0x64 ,0x61 ,0x74 ,0x61};
int len,fs,i;

	len=tell(out_fd)-44;
	fs=t_sampling_frequency[header->ID][header->sampling_frequency];
	Rewind(out_fd);
	FileWrite(ispred,1,20,out_fd);

	/* 'microsoft' PCM */
	FilePutChar(1,out_fd);
	FilePutChar(0,out_fd);
	
	/* nch */
	FilePutChar(nch,out_fd);
	FilePutChar(0,out_fd);
	
	/* samples_per_second */
	for (i=0;i<32;i+=8) FilePutChar((fs>>i)&0xff,out_fd);
	
	/* average block size */
	fs *= 2*nch;
	for (i=0;i<32;i+=8) FilePutChar((fs>>i)&0xff,out_fd);
	
	/* block align */
        fs = 2*nch;
        for (i=0;i<16;i+=8) FilePutChar((fs>>i)&0xff,out_fd);
	         
	/* bits per sample */
        FilePutChar(16,out_fd);
        FilePutChar(0,out_fd);
        
        /* i jope anomalija! */
        FileWrite(iza,1,4,out_fd);

	/* length */
	for (i=0;i<32;i+=8) FilePutChar((len>>i)&0xff,out_fd);
	
	ThreadExit(0);
}
