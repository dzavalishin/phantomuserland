/* this file is a part of amp software, (C) tomislav uzelac 1996,1997
*/
 
/* huffman.c  huffman decoding
 *
 * Created by: tomislav uzelac  Mar,Apr 1996
 * Last modified by: tomislav uzelac Mar  8 97
 */
#include "audio.h"
#include "getbits.h"

#define HUFFMAN
#include "huffman.h"

static inline unsigned int viewbits(int n)
{
unsigned int pos,ret_value;

        pos = data >> 3;
        ret_value = buffer[pos] << 24 |
                    buffer[pos+1] << 16 |
                    buffer[pos+2] << 8 |
                    buffer[pos+3];
        ret_value <<= data & 7;
        ret_value >>= 32 - n;

        return ret_value;
}

static inline void sackbits(int n)
{
        data += n;
        data &= 8*BUFFER_SIZE-1;
}

/* huffman_decode() is supposed to be faster now
 * decodes one codeword and returns no. of bits
 */
static inline int huffman_decode(int tbl,int *x,int *y)
{
unsigned int chunk;
register unsigned int *h_tab;
register unsigned int lag;
register unsigned int half_lag;
int len;

	h_tab=tables[tbl];
	chunk=viewbits(19);

	h_tab += h_cue[tbl][chunk >> (19-NC_O)];

	len=(*h_tab>>8)&0x1f;

	/* check for an immediate hit, so we can decode those short codes very fast
	*/
	if ((*h_tab>>(32-len)) != (chunk>>(19-len))) {
		if (chunk >> (19-NC_O) < N_CUE-1)
		  lag=(h_cue[tbl][(chunk >> (19-NC_O))+1] -
		       h_cue[tbl][chunk >> (19-NC_O)]);
		else {
			/* we strongly depend on h_cue[N_CUE-1] to point to
			 * the last entry in the huffman table, so we should
			 * not get here anyway. if it didn't, we'd have to
			 * have another table with huffman tables lengths, and
			 * it would be a mess. just in case, scream&shout.
			 */ 
			Print(" h_cue clobbered. this is a bug. blip.\n");
			ThreadExit (-1);
		}
		chunk <<= 32-19;
		chunk |= 0x1ff;

		half_lag = lag >> 1;

		h_tab += half_lag;
		lag -= half_lag;

		while (lag > 1) {
		        half_lag = lag >> 1;

		        if (*h_tab < chunk)
		                h_tab += half_lag;
		        else
		                h_tab -= half_lag;

                        lag -= half_lag;
		}

		len=(*h_tab>>8)&0x1f;
		if ((*h_tab>>(32-len)) != (chunk>>(32-len))) {
		        if (*h_tab > chunk)
		                h_tab--;
		        else 
		                h_tab++;
		  
		        len=(*h_tab>>8)&0x1f;
		}
	}
	sackbits(len);
	*x=(*h_tab>>4)&0xf;
	*y=*h_tab&0xf;
	return len;
}

static inline int _qsign(int x,int *q)
{
int ret_value=0,i;
	for (i=3;i>=0;i--) 
		if ((x>>i) & 1) {
			if (getbits(1)) *q++=-1;
				else *q++=1;
			ret_value++;
		}
		else *q++=0;
	return ret_value;
}		

int decode_huffman_data(struct SIDE_INFO *info,int gr,int ch,int ssize)
{
int l,i,cnt,x,y;
int q[4],r[3],linbits[3],tr[4]={0,0,0,0};
int big_value = info->big_values[gr][ch] << 1;

	for (l=0;l<3;l++) {
		tr[l]=info->table_select[gr][ch][l];
		linbits[l]=t_linbits[info->table_select[gr][ch][l]];
	}

	tr[3]=32+info->count1table_select[gr][ch];

	/* we have to be careful here because big_values are not necessarily
	 * aligned with sfb boundaries
	 */
	if (!info->window_switching_flag[gr][ch] && info->block_type[gr][ch]==0) {

	/* this code needed some cleanup
	*/
		r[0]=t_l[info->region0_count[gr][ch]] + 1;
		if (r[0] > big_value)
			r[0]=r[1]=big_value;
		else {
			r[1]=t_l[ info->region0_count[gr][ch] + info->region1_count[gr][ch] + 1 ] + 1;
			if (r[1] > big_value)
				r[1]=big_value;
		}
		r[2]=big_value;

	} else {

		if (info->block_type[gr][ch]==2 && info->mixed_block_flag[gr][ch]==0) 
			r[0]=3*(t_s[2]+1);
		else 
			r[0]=t_l[7]+1;

		if (r[0] > big_value)
			r[0]=big_value;

		r[1]=r[2]=big_value;
	}

	l=0; cnt=0;
	for (i=0;i<3;i++) {
		for (;l<r[i];l+=2) {
		        int j = linbits[i];

			cnt+=huffman_decode(tr[i],&x,&y);

			if (x==15 && j>0) {
			        x+=getbits(j);
			        cnt+=j;
			}
			if (x) {
			        if (getbits(1)) x=-x;
			        cnt++;
			}
			if (y==15 && j>0) {
			        y+=getbits(j);
			        cnt+=j;
			}
			if (y) {
			        if (getbits(1)) y=-y;
			        cnt++;
			}

			is[ch][l]=x;
			is[ch][l+1]=y;
		}
	}
	while ((cnt < info->part2_3_length[gr][ch]-ssize) && (l<576)) {
		cnt+=huffman_decode(tr[3],&x,&y);
		cnt+=_qsign(x,q);
		for (i=0;i<4;i++) is[ch][l+i]=q[i]; /* ziher je ziher, is[578]*/
		l+=4;
	}

	/*  set position to start of the next gr/ch
	 */
 	if (cnt != info->part2_3_length[gr][ch] - ssize ) {
 		data-=cnt-(info->part2_3_length[gr][ch] - ssize);
 		data&= 8*BUFFER_SIZE - 1;
 	}
	if (l<576) non_zero[ch]=l;
	else non_zero[ch]=576;
	/* zero out everything else
	*/
	for (;l<576;l++) is[ch][l]=0;
	return 1;
}
