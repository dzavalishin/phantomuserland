/* getblk.c, DCT block decoding                                             */

/* Copyright (C) 1996, MPEG Software Simulation Group. All Rights Reserved. */

/*
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any license fee or
 * royalty on an "as is" basis.  The MPEG Software Simulation Group disclaims
 * any and all warranties, whether express, implied, or statuary, including any
 * implied warranties or merchantability or of fitness for a particular
 * purpose.  In no event shall the copyright-holder be liable for any
 * incidental, punitive, or consequential damages of any kind whatsoever
 * arising from the use of these programs.
 *
 * This disclaimer of warranty extends to the user of these programs and user's
 * customers, employees, agents, transferees, successors, and assigns.
 *
 * The MPEG Software Simulation Group does not represent or warrant that the
 * programs furnished hereunder are free of infringement of any third-party
 * patents.
 *
 * Commercial implementations of MPEG-1 and MPEG-2 video, including shareware,
 * are subject to royalty fees to patent holders.  Many of these patents are
 * general enough such that they are unavoidable regardless of implementation
 * design.
 *
 */

#include <stdio.h>
//#include <lib/sys.h>

#include "fcntl.h"
#include "config.h"
#include "global.h"


/* defined in getvlc.h */
typedef struct {
  char run, level, len;
} DCTtab;

extern DCTtab DCTtabfirst[],DCTtabnext[],DCTtab0[],DCTtab1[];
extern DCTtab DCTtab2[],DCTtab3[],DCTtab4[],DCTtab5[],DCTtab6[];
extern DCTtab DCTtab0a[],DCTtab1a[];


/* decode one intra coded MPEG-1 block */

void Decode_MPEG1_Intra_Block(comp,dc_dct_pred)
int comp;
int dc_dct_pred[];
{
  int val, i, j, sign;
  unsigned int code;
  DCTtab *tab;
  short *bp;

  bp = ld->block[comp];

  /* ISO/IEC 11172-2 section 2.4.3.7: Block layer. */
  /* decode DC coefficients */
  if (comp<4)
    bp[0] = (dc_dct_pred[0]+=Get_Luma_DC_dct_diff()) << 3;
  else if (comp==4)
    bp[0] = (dc_dct_pred[1]+=Get_Chroma_DC_dct_diff()) << 3;
  else
    bp[0] = (dc_dct_pred[2]+=Get_Chroma_DC_dct_diff()) << 3;

  if (Fault_Flag) return;

  /* D-pictures do not contain AC coefficients */
  if(picture_coding_type == D_TYPE)
    return;

  /* decode AC coefficients */
  for (i=1; ; i++)
  {
    code = Show_Bits(16);
    if (code>=16384)
      tab = &DCTtabnext[(code>>12)-4];
    else if (code>=1024)
      tab = &DCTtab0[(code>>8)-4];
    else if (code>=512)
      tab = &DCTtab1[(code>>6)-8];
    else if (code>=256)
      tab = &DCTtab2[(code>>4)-16];
    else if (code>=128)
      tab = &DCTtab3[(code>>3)-16];
    else if (code>=64)
      tab = &DCTtab4[(code>>2)-16];
    else if (code>=32)
      tab = &DCTtab5[(code>>1)-16];
    else if (code>=16)
      tab = &DCTtab6[code-16];
    else
    {
      if (!Quiet_Flag)
        printf("invalid Huffman code in Decode_MPEG1_Intra_Block()\n");
      Fault_Flag = 1;
      return;
    }

    Flush_Buffer(tab->len);

    if (tab->run==64) /* end_of_block */
      return;

    if (tab->run==65) /* escape */
    {
      i+= Get_Bits(6);

      val = Get_Bits(8);
      if (val==0)
        val = Get_Bits(8);
      else if (val==128)
        val = Get_Bits(8) - 256;
      else if (val>128)
        val -= 256;

      if((sign = (val<0)))
        val = -val;
    }
    else
    {
      i+= tab->run;
      val = tab->level;
      sign = Get_Bits(1);
    }

    if (i>=64)
    {
      if (!Quiet_Flag)
        printf("DCT coeff index (i) out of bounds (intra) %i\n",i);
      Fault_Flag = 1;
      return;
    }

    j = scan[ZIG_ZAG][i];
    val = (val*ld->quantizer_scale*ld->intra_quantizer_matrix[j]) >> 3;

    /* mismatch control ('oddification') */
    if (val!=0) /* should always be true, but it's not guaranteed */
      val = (val-1) | 1; /* equivalent to: if ((val&1)==0) val = val - 1; */

    /* saturation */
    if (!sign)
      bp[j] = (val>2047) ?  2047 :  val; /* positive */
    else
      bp[j] = (val>2048) ? -2048 : -val; /* negative */
  }
}


/* decode one non-intra coded MPEG-1 block */

void Decode_MPEG1_Non_Intra_Block(comp)
int comp;
{
  int val, i, j, sign;
  unsigned int code;
  DCTtab *tab;
  short *bp;

  bp = ld->block[comp];

  /* decode AC coefficients */
  for (i=0; ; i++)
  {
    code = Show_Bits(16);
    if (code>=16384)
    {
      if (i==0)
        tab = &DCTtabfirst[(code>>12)-4];
      else
        tab = &DCTtabnext[(code>>12)-4];
    }
    else if (code>=1024)
      tab = &DCTtab0[(code>>8)-4];
    else if (code>=512)
      tab = &DCTtab1[(code>>6)-8];
    else if (code>=256)
      tab = &DCTtab2[(code>>4)-16];
    else if (code>=128)
      tab = &DCTtab3[(code>>3)-16];
    else if (code>=64)
      tab = &DCTtab4[(code>>2)-16];
    else if (code>=32)
      tab = &DCTtab5[(code>>1)-16];
    else if (code>=16)
      tab = &DCTtab6[code-16];
    else
    {
      if (!Quiet_Flag)
        printf("invalid Huffman code in Decode_MPEG1_Non_Intra_Block()\n");
      Fault_Flag = 1;
      return;
    }

    Flush_Buffer(tab->len);

    if (tab->run==64) /* end_of_block */
      return;

    if (tab->run==65) /* escape */
    {
      i+= Get_Bits(6);

      val = Get_Bits(8);
      if (val==0)
        val = Get_Bits(8);
      else if (val==128)
        val = Get_Bits(8) - 256;
      else if (val>128)
        val -= 256;

      if((sign = (val<0)))
        val = -val;
    }
    else
    {
      i+= tab->run;
      val = tab->level;
      sign = Get_Bits(1);
    }

    if (i>=64)
    {
      if (!Quiet_Flag)
        printf("DCT coeff index (i) out of bounds (inter) %i\n",i);
      Fault_Flag = 1;
      return;
    }

    j = scan[ZIG_ZAG][i];
    val = (((val<<1)+1)*ld->quantizer_scale*ld->non_intra_quantizer_matrix[j]) >> 4;

    /* mismatch control ('oddification') */
    if (val!=0) /* should always be true, but it's not guaranteed */
      val = (val-1) | 1; /* equivalent to: if ((val&1)==0) val = val - 1; */

    /* saturation */
    if (!sign)
      bp[j] = (val>2047) ?  2047 :  val; /* positive */
    else
      bp[j] = (val>2048) ? -2048 : -val; /* negative */
  }
}


/* decode one intra coded MPEG-2 block */

void Decode_MPEG2_Intra_Block(comp,dc_dct_pred)
int comp;
int dc_dct_pred[];
{
  int val, i, j, sign, nc, cc, run;
  unsigned int code;
  DCTtab *tab;
  short *bp;
  int *qmat;
  struct layer_data *ld1;

  /* with data partitioning, data always goes to base layer */
  ld1 = (ld->scalable_mode==SC_DP) ? &base : ld;
  bp = ld1->block[comp];

  if (base.scalable_mode==SC_DP)
  {
    if (base.priority_breakpoint<64)
      ld = &enhan;
    else
      ld = &base;
  }

  cc = (comp<4) ? 0 : (comp&1)+1;

  qmat = (comp<4 || chroma_format==CHROMA420)
         ? ld1->intra_quantizer_matrix
         : ld1->chroma_intra_quantizer_matrix;

  /* ISO/IEC 13818-2 section 7.2.1: decode DC coefficients */
  if (cc==0)
    val = (dc_dct_pred[0]+= Get_Luma_DC_dct_diff());
  else if (cc==1)
    val = (dc_dct_pred[1]+= Get_Chroma_DC_dct_diff());
  else
    val = (dc_dct_pred[2]+= Get_Chroma_DC_dct_diff());

  if (Fault_Flag) return;

  bp[0] = val << (3-intra_dc_precision);

  nc=0;

#ifdef TRACE
  if (Trace_Flag)
    printf("DCT(%d)i:",comp);
#endif /* TRACE */

  /* decode AC coefficients */
  for (i=1; ; i++)
  {
    code = Show_Bits(16);
    if (code>=16384 && !intra_vlc_format)
      tab = &DCTtabnext[(code>>12)-4];
    else if (code>=1024)
    {
      if (intra_vlc_format)
        tab = &DCTtab0a[(code>>8)-4];
      else
        tab = &DCTtab0[(code>>8)-4];
    }
    else if (code>=512)
    {
      if (intra_vlc_format)
        tab = &DCTtab1a[(code>>6)-8];
      else
        tab = &DCTtab1[(code>>6)-8];
    }
    else if (code>=256)
      tab = &DCTtab2[(code>>4)-16];
    else if (code>=128)
      tab = &DCTtab3[(code>>3)-16];
    else if (code>=64)
      tab = &DCTtab4[(code>>2)-16];
    else if (code>=32)
      tab = &DCTtab5[(code>>1)-16];
    else if (code>=16)
      tab = &DCTtab6[code-16];
    else
    {
      if (!Quiet_Flag)
        printf("invalid Huffman code in Decode_MPEG2_Intra_Block()\n");
      Fault_Flag = 1;
      return;
    }

    Flush_Buffer(tab->len);

#ifdef TRACE
    if (Trace_Flag)
    {
      printf(" (");
      Print_Bits(code,16,tab->len);
    }
#endif /* TRACE */

    if (tab->run==64) /* end_of_block */
    {
#ifdef TRACE
      if (Trace_Flag)
        printf("): EOB\n");
#endif /* TRACE */
      return;
    }

    if (tab->run==65) /* escape */
    {
#ifdef TRACE
      if (Trace_Flag)
      {
        putchar(' ');
        Print_Bits(Show_Bits(6),6,6);
      }
#endif /* TRACE */

      i+= run = Get_Bits(6);

#ifdef TRACE
      if (Trace_Flag)
      {
        putchar(' ');
        Print_Bits(Show_Bits(12),12,12);
      }
#endif /* TRACE */

      val = Get_Bits(12);
      if ((val&2047)==0)
      {
        if (!Quiet_Flag)
          printf("invalid escape in Decode_MPEG2_Intra_Block()\n");
        Fault_Flag = 1;
        return;
      }
      if((sign = (val>=2048)))
        val = 4096 - val;
    }
    else
    {
      i+= run = tab->run;
      val = tab->level;
      sign = Get_Bits(1);

#ifdef TRACE
      if (Trace_Flag)
        printf("%d",sign);
#endif /* TRACE */
    }

    if (i>=64)
    {
      if (!Quiet_Flag)
        printf("DCT coeff index (i) out of bounds (intra2) %i\n",i);
      Fault_Flag = 1;
      return;
    }

#ifdef TRACE
    if (Trace_Flag)
      printf("): %d/%d",run,sign ? -val : val);
#endif /* TRACE */

    j = scan[ld1->alternate_scan][i];
    val = (val * ld1->quantizer_scale * qmat[j]) >> 4;
    bp[j] = sign ? -val : val;
    nc++;

    if (base.scalable_mode==SC_DP && nc==base.priority_breakpoint-63)
      ld = &enhan;
  }
}


/* decode one non-intra coded MPEG-2 block */

void Decode_MPEG2_Non_Intra_Block(comp)
int comp;
{
  int val, i, j, sign, nc, run;
  unsigned int code;
  DCTtab *tab;
  short *bp;
  int *qmat;
  struct layer_data *ld1;

  /* with data partitioning, data always goes to base layer */
  ld1 = (ld->scalable_mode==SC_DP) ? &base : ld;
  bp = ld1->block[comp];

  if (base.scalable_mode==SC_DP)
  {
    if (base.priority_breakpoint<64)
      ld = &enhan;
    else
      ld = &base;
  }

  qmat = (comp<4 || chroma_format==CHROMA420)
         ? ld1->non_intra_quantizer_matrix
         : ld1->chroma_non_intra_quantizer_matrix;

  nc = 0;

#ifdef TRACE
  if (Trace_Flag)
    printf("DCT(%d)n:",comp);
#endif /* TRACE */

  /* decode AC coefficients */
  for (i=0; ; i++)
  {
    code = Show_Bits(16);
    if (code>=16384)
    {
      if (i==0)
        tab = &DCTtabfirst[(code>>12)-4];
      else
        tab = &DCTtabnext[(code>>12)-4];
    }
    else if (code>=1024)
      tab = &DCTtab0[(code>>8)-4];
    else if (code>=512)
      tab = &DCTtab1[(code>>6)-8];
    else if (code>=256)
      tab = &DCTtab2[(code>>4)-16];
    else if (code>=128)
      tab = &DCTtab3[(code>>3)-16];
    else if (code>=64)
      tab = &DCTtab4[(code>>2)-16];
    else if (code>=32)
      tab = &DCTtab5[(code>>1)-16];
    else if (code>=16)
      tab = &DCTtab6[code-16];
    else
    {
      if (!Quiet_Flag)
        printf("invalid Huffman code in Decode_MPEG2_Non_Intra_Block()\n");
      Fault_Flag = 1;
      return;
    }

    Flush_Buffer(tab->len);

#ifdef TRACE
    if (Trace_Flag)
    {
      printf(" (");
      Print_Bits(code,16,tab->len);
    }
#endif /* TRACE */

    if (tab->run==64) /* end_of_block */
    {
#ifdef TRACE
      if (Trace_Flag)
        printf("): EOB\n");
#endif /* TRACE */
      return;
    }

    if (tab->run==65) /* escape */
    {
#ifdef TRACE
      if (Trace_Flag)
      {
        putchar(' ');
        Print_Bits(Show_Bits(6),6,6);
      }
#endif /* TRACE */

      i+= run = Get_Bits(6);

#ifdef TRACE
      if (Trace_Flag)
      {
        putchar(' ');
        Print_Bits(Show_Bits(12),12,12);
      }
#endif /* TRACE */

      val = Get_Bits(12);
      if ((val&2047)==0)
      {
        if (!Quiet_Flag)
          printf("invalid escape in Decode_MPEG2_Intra_Block()\n");
        Fault_Flag = 1;
        return;
      }
      if((sign = (val>=2048)))
        val = 4096 - val;
    }
    else
    {
      i+= run = tab->run;
      val = tab->level;
      sign = Get_Bits(1);

#ifdef TRACE
      if (Trace_Flag)
        printf("%d",sign);
#endif /* TRACE */
    }

    if (i>=64)
    {
      if (!Quiet_Flag)
        printf("DCT coeff index (i) out of bounds (inter2) %i\n",i);
      Fault_Flag = 1;
      return;
    }

#ifdef TRACE
    if (Trace_Flag)
      printf("): %d/%d",run,sign?-val:val);
#endif /* TRACE */

    j = scan[ld1->alternate_scan][i];
    val = (((val<<1)+1) * ld1->quantizer_scale * qmat[j]) >> 5;
    bp[j] = sign ? -val : val;
    nc++;

    if (base.scalable_mode==SC_DP && nc==base.priority_breakpoint-63)
      ld = &enhan;
  }
}
