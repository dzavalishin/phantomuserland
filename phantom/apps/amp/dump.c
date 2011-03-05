/* this file is a part of amp software, (C) tomislav uzelac 1996,1997
*/
 
/* dump.c  binary/hex dump from buffer
 * 
 * Created by: tomislav uzelac  May 1996
 * Last modified by: tomislav May 31 1997
 */
#include <unistd.h>
#include <string.h>

#include "audio.h"
#include "getbits.h"

#define DUMP
#include "dump.h"

/* no hex dump, sorry
 */
void dump(int *length)   /* in fact int length[4] */
{
int i,j;
int _data,space=0;
	Print(" *********** binary dump\n");
	_data=data;
	for (i=0;i<4;i++) {
		for (j=0;j<space;j++) Print(" ");
		for (j=0;j<length[i];j++) {
			Print("%1d",(buffer[_data/8] >> (7-(_data&7)) )&1 );
			space++;
			_data++;
			_data&=8*BUFFER_SIZE-1;
			if (!(_data & 7)) {
				Print(" ");
				space++;
				if (space>70) {
					Print("\n");
					space=0;
				}
			}
		}
		Print("~\n");
	}
}

void show_header(struct AUDIO_HEADER *header)
{
int bitrate=t_bitrate[header->ID][3-header->layer][header->bitrate_index];
int fs=t_sampling_frequency[header->ID][header->sampling_frequency];
int mpg,layer;
char stm[8];
	if (A_QUIET) return;
	layer=4-header->layer;
	if (header->ID==1) mpg=1;
	else mpg=2;
	if (header->mode==3) StringCopy(stm,"mono");
	else StringCopy(stm,"stereo");

	Print("\n\
Properties:    %s %dHz\n\
Coding Method: MPEG%1d.0 layer%1d\n\
Bitrate:       %dkbit/s\n"\
		,stm,fs,mpg,layer,bitrate);
}
