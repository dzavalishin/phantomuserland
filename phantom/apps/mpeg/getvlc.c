/* getvlc.c, variable length decoding                                       */

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

#include "config.h"
#include "global.h"
#include "getvlc.h"

/* private prototypes */
/* generic picture macroblock type processing functions */
static int Get_I_macroblock_type _ANSI_ARGS_((void));
static int Get_P_macroblock_type _ANSI_ARGS_((void));
static int Get_B_macroblock_type _ANSI_ARGS_((void));
static int Get_D_macroblock_type _ANSI_ARGS_((void));

/* spatial picture macroblock type processing functions */
static int Get_I_Spatial_macroblock_type _ANSI_ARGS_((void));
static int Get_P_Spatial_macroblock_type _ANSI_ARGS_((void));
static int Get_B_Spatial_macroblock_type _ANSI_ARGS_((void));
static int Get_SNR_macroblock_type _ANSI_ARGS_((void));

int Get_macroblock_type()
{
  int macroblock_type = 0;

  if (ld->scalable_mode==SC_SNR)
    macroblock_type = Get_SNR_macroblock_type();
  else
  {
    switch (picture_coding_type)
    {
    case I_TYPE:
      macroblock_type = ld->pict_scal ? Get_I_Spatial_macroblock_type() : Get_I_macroblock_type();
      break;
    case P_TYPE:
      macroblock_type = ld->pict_scal ? Get_P_Spatial_macroblock_type() : Get_P_macroblock_type();
      break;
    case B_TYPE:
      macroblock_type = ld->pict_scal ? Get_B_Spatial_macroblock_type() : Get_B_macroblock_type();
      break;
    case D_TYPE:
      macroblock_type = Get_D_macroblock_type();
      break;
    default:
      printf("Get_macroblock_type(): unrecognized picture coding type\n");
      break;
    }
  }

  return macroblock_type;
}

static int Get_I_macroblock_type()
{
#ifdef TRACE
  if (Trace_Flag)
    printf("macroblock_type(I) ");
#endif /* TRACE */

  if (Get_Bits1())
  {
#ifdef TRACE
    if (Trace_Flag)
      printf("(1): Intra (1)\n");
#endif /* TRACE */
    return 1;
  }

  if (!Get_Bits1())
  {
    if (!Quiet_Flag)
      printf("Invalid macroblock_type code\n");
    Fault_Flag = 1;
  }

#ifdef TRACE
  if (Trace_Flag)
    printf("(01): Intra, Quant (17)\n");
#endif /* TRACE */

  return 17;
}

/*
static char *MBdescr[]={
  "",                  "Intra",        "No MC, Coded",         "",
  "Bwd, Not Coded",    "",             "Bwd, Coded",           "",
  "Fwd, Not Coded",    "",             "Fwd, Coded",           "",
  "Interp, Not Coded", "",             "Interp, Coded",        "",
  "",                  "Intra, Quant", "No MC, Coded, Quant",  "",
  "",                  "",             "Bwd, Coded, Quant",    "",
  "",                  "",             "Fwd, Coded, Quant",    "",
  "",                  "",             "Interp, Coded, Quant", ""
};
*/
static int Get_P_macroblock_type()
{
  int code;

#ifdef TRACE
  if (Trace_Flag)
    printf("macroblock_type(P) (");
#endif /* TRACE */

  if ((code = Show_Bits(6))>=8)
  {
    code >>= 3;
    Flush_Buffer(PMBtab0[code].len);
#ifdef TRACE
    if (Trace_Flag)
    {
      Print_Bits(code,3,PMBtab0[code].len);
      printf("): %s (%d)\n",MBdescr[(int)PMBtab0[code].val],PMBtab0[code].val);
    }
#endif /* TRACE */
    return PMBtab0[code].val;
  }

  if (code==0)
  {
    if (!Quiet_Flag)
      printf("Invalid macroblock_type code\n");
    Fault_Flag = 1;
    return 0;
  }

  Flush_Buffer(PMBtab1[code].len);

#ifdef TRACE
  if (Trace_Flag)
  {
    Print_Bits(code,6,PMBtab1[code].len);
    printf("): %s (%d)\n",MBdescr[(int)PMBtab1[code].val],PMBtab1[code].val);
  }
#endif /* TRACE */

  return PMBtab1[code].val;
}

static int Get_B_macroblock_type()
{
  int code;

#ifdef TRACE
  if (Trace_Flag)
    printf("macroblock_type(B) (");
#endif /* TRACE */

  if ((code = Show_Bits(6))>=8)
  {
    code >>= 2;
    Flush_Buffer(BMBtab0[code].len);

#ifdef TRACE
    if (Trace_Flag)
    {
      Print_Bits(code,4,BMBtab0[code].len);
      printf("): %s (%d)\n",MBdescr[(int)BMBtab0[code].val],BMBtab0[code].val);
    }
#endif /* TRACE */

    return BMBtab0[code].val;
  }

  if (code==0)
  {
    if (!Quiet_Flag)
      printf("Invalid macroblock_type code\n");
    Fault_Flag = 1;
    return 0;
  }

  Flush_Buffer(BMBtab1[code].len);

#ifdef TRACE
  if (Trace_Flag)
  {
    Print_Bits(code,6,BMBtab1[code].len);
    printf("): %s (%d)\n",MBdescr[(int)BMBtab1[code].val],BMBtab1[code].val);
  }
#endif /* TRACE */

  return BMBtab1[code].val;
}

static int Get_D_macroblock_type()
{
  if (!Get_Bits1())
  {
    if (!Quiet_Flag)
      printf("Invalid macroblock_type code\n");
    Fault_Flag=1;
  }

  return 1;
}

/* macroblock_type for pictures with spatial scalability */
static int Get_I_Spatial_macroblock_type()
{
  int code;

#ifdef TRACE
  if (Trace_Flag)
    printf("macroblock_type(I,spat) (");
#endif /* TRACE */

  code = Show_Bits(4);

  if (code==0)
  {
    if (!Quiet_Flag)
      printf("Invalid macroblock_type code\n");
    Fault_Flag = 1;
    return 0;
  }

#ifdef TRACE
  if (Trace_Flag)
  {
    Print_Bits(code,4,spIMBtab[code].len);
    printf("): %02x\n",spIMBtab[code].val);
  }
#endif /* TRACE */

  Flush_Buffer(spIMBtab[code].len);
  return spIMBtab[code].val;
}

static int Get_P_Spatial_macroblock_type()
{
  int code;

#ifdef TRACE
  if (Trace_Flag)
    printf("macroblock_type(P,spat) (");
#endif /* TRACE */

  code = Show_Bits(7);

  if (code<2)
  {
    if (!Quiet_Flag)
      printf("Invalid macroblock_type code\n");
    Fault_Flag = 1;
    return 0;
  }

  if (code>=16)
  {
    code >>= 3;
    Flush_Buffer(spPMBtab0[code].len);

#ifdef TRACE
    if (Trace_Flag)
    {
      Print_Bits(code,4,spPMBtab0[code].len);
      printf("): %02x\n",spPMBtab0[code].val);
    }
#endif /* TRACE */

    return spPMBtab0[code].val;
  }

  Flush_Buffer(spPMBtab1[code].len);

#ifdef TRACE
  if (Trace_Flag)
  {
    Print_Bits(code,7,spPMBtab1[code].len);
    printf("): %02x\n",spPMBtab1[code].val);
  }
#endif /* TRACE */

  return spPMBtab1[code].val;
}

static int Get_B_Spatial_macroblock_type()
{
  int code;
  VLCtab *p;

#ifdef TRACE
  if (Trace_Flag)
    printf("macroblock_type(B,spat) (");
#endif /* TRACE */

  code = Show_Bits(9);

  if (code>=64)
    p = &spBMBtab0[(code>>5)-2];
  else if (code>=16)
    p = &spBMBtab1[(code>>2)-4];
  else if (code>=8)
    p = &spBMBtab2[code-8];
  else
  {
    if (!Quiet_Flag)
      printf("Invalid macroblock_type code\n");
    Fault_Flag = 1;
    return 0;
  }

  Flush_Buffer(p->len);

#ifdef TRACE
  if (Trace_Flag)
  {
    Print_Bits(code,9,p->len);
    printf("): %02x\n",p->val);
  }
#endif /* TRACE */

  return p->val;
}

static int Get_SNR_macroblock_type()
{
  int code;

#ifdef TRACE			/* *CH* */
  if (Trace_Flag)
    printf("macroblock_type(SNR) (");
#endif //TRACE

  code = Show_Bits(3);

  if (code==0)
  {
    if (!Quiet_Flag)
      printf("Invalid macroblock_type code\n");
    Fault_Flag = 1;
    return 0;
  }

  Flush_Buffer(SNRMBtab[code].len);

#ifdef TRACE			/* *CH* */
  if (Trace_Flag)
  {
    Print_Bits(code,3,SNRMBtab[code].len);
    printf("): %s (%d)\n",MBdescr[(int)SNRMBtab[code].val],SNRMBtab[code].val);
  }
#endif //TRACE


  return SNRMBtab[code].val;
}

int Get_motion_code()
{
  int code;

#ifdef TRACE
  if (Trace_Flag)
    printf("motion_code (");
#endif /* TRACE */

  if (Get_Bits1())
  {
#ifdef TRACE
    if (Trace_Flag)
      printf("0): 0\n");
#endif /* TRACE */
    return 0;
  }

  if ((code = Show_Bits(9))>=64)
  {
    code >>= 6;
    Flush_Buffer(MVtab0[code].len);

#ifdef TRACE
    if (Trace_Flag)
    {
      Print_Bits(code,3,MVtab0[code].len);
      printf("%d): %d\n",
        Show_Bits(1),Show_Bits(1)?-MVtab0[code].val:MVtab0[code].val);
    }
#endif /* TRACE */

    return Get_Bits1()?-MVtab0[code].val:MVtab0[code].val;
  }

  if (code>=24)
  {
    code >>= 3;
    Flush_Buffer(MVtab1[code].len);

#ifdef TRACE
    if (Trace_Flag)
    {
      Print_Bits(code,6,MVtab1[code].len);
      printf("%d): %d\n",
        Show_Bits(1),Show_Bits(1)?-MVtab1[code].val:MVtab1[code].val);
    }
#endif /* TRACE */

    return Get_Bits1()?-MVtab1[code].val:MVtab1[code].val;
  }

  if ((code-=12)<0)
  {
    if (!Quiet_Flag)
/* HACK */
      printf("Invalid motion_vector code (MBA %d, pic %d)\n", global_MBA, global_pic);
    Fault_Flag=1;
    return 0;
  }

  Flush_Buffer(MVtab2[code].len);

#ifdef TRACE
  if (Trace_Flag)
  {
    Print_Bits(code+12,9,MVtab2[code].len);
    printf("%d): %d\n",
      Show_Bits(1),Show_Bits(1)?-MVtab2[code].val:MVtab2[code].val);
  }
#endif /* TRACE */

  return Get_Bits1() ? -MVtab2[code].val : MVtab2[code].val;
}

/* get differential motion vector (for dual prime prediction) */
int Get_dmvector()
{
#ifdef TRACE
  if (Trace_Flag)
    printf("dmvector (");
#endif /* TRACE */

  if (Get_Bits(1))
  {
#ifdef TRACE
    if (Trace_Flag)
      Print(Show_Bits(1) ? "11): -1\n" : "10): 1\n");
#endif /* TRACE */
    return Get_Bits(1) ? -1 : 1;
  }
  else
  {
#ifdef TRACE
    if (Trace_Flag)
      printf("0): 0\n");
#endif /* TRACE */
    return 0;
  }
}

int Get_coded_block_pattern()
{
  int code;

#ifdef TRACE
  if (Trace_Flag)
    printf("coded_block_pattern_420 (");
#endif /* TRACE */

  if ((code = Show_Bits(9))>=128)
  {
    code >>= 4;
    Flush_Buffer(CBPtab0[code].len);

#ifdef TRACE
    if (Trace_Flag)
    {
      Print_Bits(code,5,CBPtab0[code].len);
      printf("): ");
      printf_Bits(CBPtab0[code].val,6,6);
      printf(" (%d)\n",CBPtab0[code].val);
    }
#endif /* TRACE */

    return CBPtab0[code].val;
  }

  if (code>=8)
  {
    code >>= 1;
    Flush_Buffer(CBPtab1[code].len);

#ifdef TRACE
    if (Trace_Flag)
    {
      Print_Bits(code,8,CBPtab1[code].len);
      printf("): ");
      Print_Bits(CBPtab1[code].val,6,6);
      printf(" (%d)\n",CBPtab1[code].val);
    }
#endif /* TRACE */

    return CBPtab1[code].val;
  }

  if (code<1)
  {
    if (!Quiet_Flag)
      printf("Invalid coded_block_pattern code\n");
    Fault_Flag = 1;
    return 0;
  }

  Flush_Buffer(CBPtab2[code].len);

#ifdef TRACE
  if (Trace_Flag)
  {
    Print_Bits(code,9,CBPtab2[code].len);
    printf("): ");
    Print_Bits(CBPtab2[code].val,6,6);
    printf(" (%d)\n",CBPtab2[code].val);
  }
#endif /* TRACE */

  return CBPtab2[code].val;
}

int Get_macroblock_address_increment()
{
  int code, val;

#ifdef TRACE
  if (Trace_Flag)
    printf("macroblock_address_increment (");
#endif /* TRACE */

  val = 0;

  while ((code = Show_Bits(11))<24)
  {
    if (code!=15) /* if not macroblock_stuffing */
    {
      if (code==8) /* if macroblock_escape */
      {
#ifdef TRACE
        if (Trace_Flag)
          printf("00000001000 ");
#endif /* TRACE */

        val+= 33;
      }
      else
      {
        if (!Quiet_Flag)
          printf("Invalid macroblock_address_increment code\n");

        Fault_Flag = 1;
        return 1;
      }
    }
    else /* macroblock suffing */
    {
#ifdef TRACE
      if (Trace_Flag)
        printf("00000001111 ");
#endif /* TRACE */
    }

    Flush_Buffer(11);
  }

  /* macroblock_address_increment == 1 */
  /* ('1' is in the MSB position of the lookahead) */
  if (code>=1024)
  {
    Flush_Buffer(1);
#ifdef TRACE
    if (Trace_Flag)
      printf("1): %d\n",val+1);
#endif /* TRACE */
    return val + 1;
  }

  /* codes 00010 ... 011xx */
  if (code>=128)
  {
    /* remove leading zeros */
    code >>= 6;
    Flush_Buffer(MBAtab1[code].len);

#ifdef TRACE
    if (Trace_Flag)
    {
      Print_Bits(code,5,MBAtab1[code].len);
      printf("): %d\n",val+MBAtab1[code].val);
    }
#endif /* TRACE */

    
    return val + MBAtab1[code].val;
  }
  
  /* codes 00000011000 ... 0000111xxxx */
  code-= 24; /* remove common base */
  Flush_Buffer(MBAtab2[code].len);

#ifdef TRACE
  if (Trace_Flag)
  {
    Print_Bits(code+24,11,MBAtab2[code].len);
    printf("): %d\n",val+MBAtab2[code].val);
  }
#endif /* TRACE */

  return val + MBAtab2[code].val;
}

/* combined MPEG-1 and MPEG-2 stage. parse VLC and 
   perform dct_diff arithmetic.

   MPEG-1:  ISO/IEC 11172-2 section
   MPEG-2:  ISO/IEC 13818-2 section 7.2.1 
   
   Note: the arithmetic here is presented more elegantly than
   the spec, yet the results, dct_diff, are the same.
*/

int Get_Luma_DC_dct_diff()
{
  int code, size, dct_diff;

#ifdef TRACE
/*
  if (Trace_Flag)
    printf("dct_dc_size_luminance: (");
*/
#endif /* TRACE */

  /* decode length */
  code = Show_Bits(5);

  if (code<31)
  {
    size = DClumtab0[code].val;
    Flush_Buffer(DClumtab0[code].len);
#ifdef TRACE
/*
    if (Trace_Flag)
    {
      Print_Bits(code,5,DClumtab0[code].len);
      printf("): %d",size);
    }
*/
#endif /* TRACE */
  }
  else
  {
    code = Show_Bits(9) - 0x1f0;
    size = DClumtab1[code].val;
    Flush_Buffer(DClumtab1[code].len);

#ifdef TRACE
/*
    if (Trace_Flag)
    {
      Print_Bits(code+0x1f0,9,DClumtab1[code].len);
      printf("): %d",size);
    }
*/
#endif /* TRACE */
  }

#ifdef TRACE
/*
  if (Trace_Flag)
    printf(", dct_dc_differential (");
*/
#endif /* TRACE */

  if (size==0)
    dct_diff = 0;
  else
  {
    dct_diff = Get_Bits(size);
#ifdef TRACE
/*
    if (Trace_Flag)
      printf_Bits(dct_diff,size,size);
*/
#endif /* TRACE */
    if ((dct_diff & (1<<(size-1)))==0)
      dct_diff-= (1<<size) - 1;
  }

#ifdef TRACE
/*
  if (Trace_Flag)
    printf("): %d\n",dct_diff);
*/
#endif /* TRACE */

  return dct_diff;
}


int Get_Chroma_DC_dct_diff()
{
  int code, size, dct_diff;

#ifdef TRACE
/*
  if (Trace_Flag)
    printf("dct_dc_size_chrominance: (");
*/
#endif /* TRACE */

  /* decode length */
  code = Show_Bits(5);

  if (code<31)
  {
    size = DCchromtab0[code].val;
    Flush_Buffer(DCchromtab0[code].len);

#ifdef TRACE
/*
    if (Trace_Flag)
    {
      printf_Bits(code,5,DCchromtab0[code].len);
      printf("): %d",size);
    }
*/
#endif /* TRACE */
  }
  else
  {
    code = Show_Bits(10) - 0x3e0;
    size = DCchromtab1[code].val;
    Flush_Buffer(DCchromtab1[code].len);

#ifdef TRACE
/*
    if (Trace_Flag)
    {
      printf_Bits(code+0x3e0,10,DCchromtab1[code].len);
      printf("): %d",size);
    }
*/
#endif /* TRACE */
  }

#ifdef TRACE
/* 
  if (Trace_Flag)
    printf(", dct_dc_differential (");
*/
#endif /* TRACE */

  if (size==0)
    dct_diff = 0;
  else
  {
    dct_diff = Get_Bits(size);
#ifdef TRACE
/*
    if (Trace_Flag)
      printf_Bits(dct_diff,size,size);
*/
#endif /* TRACE */
    if ((dct_diff & (1<<(size-1)))==0)
      dct_diff-= (1<<size) - 1;
  }

#ifdef TRACE
/*
  if (Trace_Flag)
    printf("): %d\n",dct_diff);
*/
#endif /* TRACE */

  return dct_diff;
}
