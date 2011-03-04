/* getpic.c, picture decoding                                               */

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

/* private prototypes*/
static void picture_data _ANSI_ARGS_((int framenum));
static void macroblock_modes _ANSI_ARGS_((int *pmacroblock_type, int *pstwtype,
  int *pstwclass, int *pmotion_type, int *pmotion_vector_count, int *pmv_format, int *pdmv,
  int *pmvscale, int *pdct_type));
static void Clear_Block _ANSI_ARGS_((int comp));
static void Sum_Block _ANSI_ARGS_((int comp));
static void Saturate _ANSI_ARGS_((short *bp));
static void Add_Block _ANSI_ARGS_((int comp, int bx, int by,
  int dct_type, int addflag));
static void Update_Picture_Buffers _ANSI_ARGS_((void));
static void frame_reorder _ANSI_ARGS_((int bitstream_framenum, 
  int sequence_framenum));
static void Decode_SNR_Macroblock _ANSI_ARGS_((int *SNRMBA, int *SNRMBAinc, 
  int MBA, int MBAmax, int *dct_type));

static void motion_compensation _ANSI_ARGS_((int MBA, int macroblock_type, 
 int motion_type, int PMV[2][2][2], int motion_vertical_field_select[2][2], 
 int dmvector[2], int stwtype, int dct_type));

static void skipped_macroblock _ANSI_ARGS_((int dc_dct_pred[3], 
  int PMV[2][2][2], int *motion_type, int motion_vertical_field_select[2][2],
  int *stwtype, int *macroblock_type));

static int slice _ANSI_ARGS_((int framenum, int MBAmax));

static int start_of_slice _ANSI_ARGS_ ((int MBAmax, int *MBA,
  int *MBAinc, int dc_dct_pred[3], int PMV[2][2][2]));

static int decode_macroblock _ANSI_ARGS_((int *macroblock_type, 
  int *stwtype, int *stwclass, int *motion_type, int *dct_type,
  int PMV[2][2][2], int dc_dct_pred[3], 
  int motion_vertical_field_select[2][2], int dmvector[2]));


/* decode one frame or field picture */
void Decode_Picture(bitstream_framenum, sequence_framenum)
int bitstream_framenum, sequence_framenum;
{

  if (picture_structure==FRAME_PICTURE && Second_Field)
  {
    /* recover from illegal number of field pictures */
    printf("odd number of field pictures\n");
    Second_Field = 0;
  }

  /* IMPLEMENTATION: update picture buffer pointers */
  Update_Picture_Buffers();

#ifdef VERIFY 
  Check_Headers(bitstream_framenum, sequence_framenum);
#endif /* VERIFY */

  /* ISO/IEC 13818-4 section 2.4.5.4 "frame buffer intercept method" */
  /* (section number based on November 1995 (Dallas) draft of the 
      conformance document) */
  if(Ersatz_Flag)
    Substitute_Frame_Buffer(bitstream_framenum, sequence_framenum);

  /* form spatial scalable picture */
 
  /* form spatial scalable picture */
  /* ISO/IEC 13818-2 section 7.7: Spatial scalability */
  if (base.pict_scal && !Second_Field) 
  {
    Spatial_Prediction();
  }

  /* decode picture data ISO/IEC 13818-2 section 6.2.3.7 */
  picture_data(bitstream_framenum);

  /* write or display current or previously decoded reference frame */
  /* ISO/IEC 13818-2 section 6.1.1.11: Frame reordering */
  frame_reorder(bitstream_framenum, sequence_framenum);

  if (picture_structure!=FRAME_PICTURE)
    Second_Field = !Second_Field;
}


/* decode all macroblocks of the current picture */
/* stages described in ISO/IEC 13818-2 section 7 */
static void picture_data(framenum)
int framenum;
{
  int MBAmax;
  int ret;

  /* number of macroblocks per picture */
  MBAmax = mb_width*mb_height;

  if (picture_structure!=FRAME_PICTURE)
    MBAmax>>=1; /* field picture has half as mnay macroblocks as frame */

  for(;;)
  {
    if((ret=slice(framenum, MBAmax))<0)
      return;
  }

}



/* decode all macroblocks of the current picture */
/* ISO/IEC 13818-2 section 6.3.16 */
static int slice(framenum, MBAmax)
int framenum, MBAmax;
{
  int MBA; 
  int MBAinc, macroblock_type, motion_type, dct_type;
  int dc_dct_pred[3];
  int PMV[2][2][2], motion_vertical_field_select[2][2];
  int dmvector[2];
  int stwtype, stwclass;
  int SNRMBA, SNRMBAinc;
  int ret;

  MBA = 0; /* macroblock address */
  MBAinc = 0;

  if((ret=start_of_slice(MBAmax, &MBA, &MBAinc, dc_dct_pred, PMV))!=1)
    return(ret);

  if (Two_Streams && enhan.scalable_mode==SC_SNR)
  {
    SNRMBA=0;
    SNRMBAinc=0;
  }

  Fault_Flag=0;

  for (;;)
  {

    /* this is how we properly exit out of picture */
    if (MBA>=MBAmax)
      return(-1); /* all macroblocks decoded */

#ifdef TRACE
    if (Trace_Flag)
      printf("frame %d, MB %d\n",framenum,MBA);
#endif /* TRACE */

#ifdef DISPLAY
    if (!progressive_frame && picture_structure==FRAME_PICTURE 
      && MBA==(MBAmax>>1) && framenum!=0 && Output_Type==T_X11 
       && !Display_Progressive_Flag)
    {
      Display_Second_Field();
    }
#endif

    ld = &base;

    if (MBAinc==0)
    {
      if (base.scalable_mode==SC_DP && base.priority_breakpoint==1)
          ld = &enhan;

      if (!Show_Bits(23) || Fault_Flag) /* next_start_code or fault */
      {
resync: /* if Fault_Flag: resynchronize to next next_start_code */
        Fault_Flag = 0;
        return(0);     /* trigger: go to next slice */
      }
      else /* neither next_start_code nor Fault_Flag */
      {
        if (base.scalable_mode==SC_DP && base.priority_breakpoint==1)
          ld = &enhan;

        /* decode macroblock address increment */
        MBAinc = Get_macroblock_address_increment();

        if (Fault_Flag) goto resync;
      }
    }

    if (MBA>=MBAmax)
    {
      /* MBAinc points beyond picture dimensions */
      if (!Quiet_Flag)
        printf("Too many macroblocks in picture\n");
      return(-1);
    }

    if (MBAinc==1) /* not skipped */
    {
      ret = decode_macroblock(&macroblock_type, &stwtype, &stwclass,
              &motion_type, &dct_type, PMV, dc_dct_pred, 
              motion_vertical_field_select, dmvector);

      if(ret==-1)
        return(-1);
   
      if(ret==0)
        goto resync;

    }
    else /* MBAinc!=1: skipped macroblock */
    {      
      /* ISO/IEC 13818-2 section 7.6.6 */
      skipped_macroblock(dc_dct_pred, PMV, &motion_type, 
        motion_vertical_field_select, &stwtype, &macroblock_type);
    }

    /* SCALABILITY: SNR */
    /* ISO/IEC 13818-2 section 7.8 */
    /* NOTE: we currently ignore faults encountered in this routine */
    if (Two_Streams && enhan.scalable_mode==SC_SNR)
      Decode_SNR_Macroblock(&SNRMBA, &SNRMBAinc, MBA, MBAmax, &dct_type);

    /* ISO/IEC 13818-2 section 7.6 */
    motion_compensation(MBA, macroblock_type, motion_type, PMV, 
      motion_vertical_field_select, dmvector, stwtype, dct_type);


    /* advance to next macroblock */
    MBA++;
    MBAinc--;
 
    /* SCALABILITY: SNR */
    if (Two_Streams && enhan.scalable_mode==SC_SNR)
    {
      SNRMBA++;
      SNRMBAinc--;
    }

    if (MBA>=MBAmax)
      return(-1); /* all macroblocks decoded */
  }
}

 
/* ISO/IEC 13818-2 section 6.3.17.1: Macroblock modes */
static void macroblock_modes(pmacroblock_type,pstwtype,pstwclass,
  pmotion_type,pmotion_vector_count,pmv_format,pdmv,pmvscale,pdct_type)
  int *pmacroblock_type, *pstwtype, *pstwclass;
  int *pmotion_type, *pmotion_vector_count, *pmv_format, *pdmv, *pmvscale;
  int *pdct_type;
{
  int macroblock_type;
  int stwtype, stwcode, stwclass;
  int motion_type = 0;
  int motion_vector_count, mv_format, dmv, mvscale;
  int dct_type;
  static unsigned char stwc_table[3][4]
    = { {6,3,7,4}, {2,1,5,4}, {2,5,7,4} };
  static unsigned char stwclass_table[9]
    = {0, 1, 2, 1, 1, 2, 3, 3, 4};

  /* get macroblock_type */
  macroblock_type = Get_macroblock_type();

  if (Fault_Flag) return;

  /* get spatial_temporal_weight_code */
  if (macroblock_type & MB_WEIGHT)
  {
    if (spatial_temporal_weight_code_table_index==0)
      stwtype = 4;
    else
    {
      stwcode = Get_Bits(2);
#ifdef TRACE
      if (Trace_Flag)
      {
        printf("spatial_temporal_weight_code (");
        printf_Bits(stwcode,2,2);
        printf("): %d\n",stwcode);
      }
#endif /* TRACE */
      stwtype = stwc_table[spatial_temporal_weight_code_table_index-1][stwcode];
    }
  }
  else
    stwtype = (macroblock_type & MB_CLASS4) ? 8 : 0;

  /* SCALABILITY: derive spatial_temporal_weight_class (Table 7-18) */
  stwclass = stwclass_table[stwtype];

  /* get frame/field motion type */
  if (macroblock_type & (MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD))
  {
    if (picture_structure==FRAME_PICTURE) /* frame_motion_type */
    {
      motion_type = frame_pred_frame_dct ? MC_FRAME : Get_Bits(2);
#ifdef TRACE
      if (!frame_pred_frame_dct && Trace_Flag)
      {
        printf("frame_motion_type (");
        printf_Bits(motion_type,2,2);
        printf("): %s\n",motion_type==MC_FIELD?"Field":
                         motion_type==MC_FRAME?"Frame":
                         motion_type==MC_DMV?"Dual_Prime":"Invalid");
      }
#endif /* TRACE */
    }
    else /* field_motion_type */
    {
      motion_type = Get_Bits(2);
#ifdef TRACE
      if (Trace_Flag)
      {
        printf("field_motion_type (");
        printf_Bits(motion_type,2,2);
        printf("): %s\n",motion_type==MC_FIELD?"Field":
                         motion_type==MC_16X8?"16x8 MC":
                         motion_type==MC_DMV?"Dual_Prime":"Invalid");
      }
#endif /* TRACE */
    }
  }
  else if ((macroblock_type & MACROBLOCK_INTRA) && concealment_motion_vectors)
  {
    /* concealment motion vectors */
    motion_type = (picture_structure==FRAME_PICTURE) ? MC_FRAME : MC_FIELD;
  }
#if 0
  else
  {
    printf("maroblock_modes(): unknown macroblock type\n");
    motion_type = -1;
  }
#endif

  /* derive motion_vector_count, mv_format and dmv, (table 6-17, 6-18) */
  if (picture_structure==FRAME_PICTURE)
  {
    motion_vector_count = (motion_type==MC_FIELD && stwclass<2) ? 2 : 1;
    mv_format = (motion_type==MC_FRAME) ? MV_FRAME : MV_FIELD;
  }
  else
  {
    motion_vector_count = (motion_type==MC_16X8) ? 2 : 1;
    mv_format = MV_FIELD;
  }

  dmv = (motion_type==MC_DMV); /* dual prime */

  /* field mv predictions in frame pictures have to be scaled
   * ISO/IEC 13818-2 section 7.6.3.1 Decoding the motion vectors
   * IMPLEMENTATION: mvscale is derived for later use in motion_vectors()
   * it displaces the stage:
   *
   *    if((mv_format=="field")&&(t==1)&&(picture_structure=="Frame picture"))
   *      prediction = PMV[r][s][t] DIV 2;
   */

  mvscale = ((mv_format==MV_FIELD) && (picture_structure==FRAME_PICTURE));

  /* get dct_type (frame DCT / field DCT) */
  dct_type = (picture_structure==FRAME_PICTURE)
             && (!frame_pred_frame_dct)
             && (macroblock_type & (MACROBLOCK_PATTERN|MACROBLOCK_INTRA))
             ? Get_Bits(1)
             : 0;

#ifdef TRACE
  if (Trace_Flag  && (picture_structure==FRAME_PICTURE)
             && (!frame_pred_frame_dct)
             && (macroblock_type & (MACROBLOCK_PATTERN|MACROBLOCK_INTRA)))
    printf("dct_type (%d): %s\n",dct_type,dct_type?"Field":"Frame");
#endif /* TRACE */

  /* return values */
  *pmacroblock_type = macroblock_type;
  *pstwtype = stwtype;
  *pstwclass = stwclass;
  *pmotion_type = motion_type;
  *pmotion_vector_count = motion_vector_count;
  *pmv_format = mv_format;
  *pdmv = dmv;
  *pmvscale = mvscale;
  *pdct_type = dct_type;
}


/* move/add 8x8-Block from block[comp] to backward_reference_frame */
/* copy reconstructed 8x8 block from block[comp] to current_frame[]
 * ISO/IEC 13818-2 section 7.6.8: Adding prediction and coefficient data
 * This stage also embodies some of the operations implied by:
 *   - ISO/IEC 13818-2 section 7.6.7: Combining predictions
 *   - ISO/IEC 13818-2 section 6.1.3: Macroblock
*/
static void Add_Block(comp,bx,by,dct_type,addflag)
int comp,bx,by,dct_type,addflag;
{
  int cc,i, j, iincr;
  unsigned char *rfp;
  short *bp;

  
  /* derive color component index */
  /* equivalent to ISO/IEC 13818-2 Table 7-1 */
  cc = (comp<4) ? 0 : (comp&1)+1; /* color component index */

  if (cc==0)
  {
    /* luminance */

    if (picture_structure==FRAME_PICTURE)
      if (dct_type)
      {
        /* field DCT coding */
        rfp = current_frame[0]
              + Coded_Picture_Width*(by+((comp&2)>>1)) + bx + ((comp&1)<<3);
        iincr = (Coded_Picture_Width<<1) - 8;
      }
      else
      {
        /* frame DCT coding */
        rfp = current_frame[0]
              + Coded_Picture_Width*(by+((comp&2)<<2)) + bx + ((comp&1)<<3);
        iincr = Coded_Picture_Width - 8;
      }
    else
    {
      /* field picture */
      rfp = current_frame[0]
            + (Coded_Picture_Width<<1)*(by+((comp&2)<<2)) + bx + ((comp&1)<<3);
      iincr = (Coded_Picture_Width<<1) - 8;
    }
  }
  else
  {
    /* chrominance */

    /* scale coordinates */
    if (chroma_format!=CHROMA444)
      bx >>= 1;
    if (chroma_format==CHROMA420)
      by >>= 1;
    if (picture_structure==FRAME_PICTURE)
    {
      if (dct_type && (chroma_format!=CHROMA420))
      {
        /* field DCT coding */
        rfp = current_frame[cc]
              + Chroma_Width*(by+((comp&2)>>1)) + bx + (comp&8);
        iincr = (Chroma_Width<<1) - 8;
      }
      else
      {
        /* frame DCT coding */
        rfp = current_frame[cc]
              + Chroma_Width*(by+((comp&2)<<2)) + bx + (comp&8);
        iincr = Chroma_Width - 8;
      }
    }
    else
    {
      /* field picture */
      rfp = current_frame[cc]
            + (Chroma_Width<<1)*(by+((comp&2)<<2)) + bx + (comp&8);
      iincr = (Chroma_Width<<1) - 8;
    }
  }

  bp = ld->block[comp];

  if (addflag)
  {
    for (i=0; i<8; i++)
    {
      for (j=0; j<8; j++)
      {
        *rfp = Clip[*bp++ + *rfp];
        rfp++;
      }

      rfp+= iincr;
    }
  }
  else
  {
    for (i=0; i<8; i++)
    {
      for (j=0; j<8; j++)
        *rfp++ = Clip[*bp++ + 128];

      rfp+= iincr;
    }
  }
}


/* ISO/IEC 13818-2 section 7.8 */
static void Decode_SNR_Macroblock(SNRMBA, SNRMBAinc, MBA, MBAmax, dct_type)
  int *SNRMBA, *SNRMBAinc;
  int MBA, MBAmax;
  int *dct_type;
{
  int SNRmacroblock_type, SNRcoded_block_pattern, SNRdct_type, dummy; 
  int slice_vert_pos_ext, quantizer_scale_code, comp, code;

  ld = &enhan;

  if (*SNRMBAinc==0)
  {
    if (!Show_Bits(23)) /* next_start_code */
    {
      next_start_code();
      code = Show_Bits(32);

      if (code<SLICE_START_CODE_MIN || code>SLICE_START_CODE_MAX)
      {
        /* only slice headers are allowed in picture_data */
        if (!Quiet_Flag)
          printf("SNR: Premature end of picture\n");
        return;
      }

      Flush_Buffer32();

      /* decode slice header (may change quantizer_scale) */
      slice_vert_pos_ext = slice_header();

      /* decode macroblock address increment */
      *SNRMBAinc = Get_macroblock_address_increment();

      /* set current location */
      *SNRMBA =
        ((slice_vert_pos_ext<<7) + (code&255) - 1)*mb_width + *SNRMBAinc - 1;

      *SNRMBAinc = 1; /* first macroblock in slice: not skipped */
    }
    else /* not next_start_code */
    {
      if (*SNRMBA>=MBAmax)
      {
        if (!Quiet_Flag)
          printf("Too many macroblocks in picture\n");
        return;
      }

      /* decode macroblock address increment */
      *SNRMBAinc = Get_macroblock_address_increment();
    }
  }

  if (*SNRMBA!=MBA)
  {
    /* streams out of sync */
    if (!Quiet_Flag)
      printf("Cant't synchronize streams\n");
    return;
  }

  if (*SNRMBAinc==1) /* not skipped */
  {
    macroblock_modes(&SNRmacroblock_type, &dummy, &dummy,
      &dummy, &dummy, &dummy, &dummy, &dummy,
      &SNRdct_type);

    if (SNRmacroblock_type & MACROBLOCK_PATTERN)
      *dct_type = SNRdct_type;

    if (SNRmacroblock_type & MACROBLOCK_QUANT)
    {
      quantizer_scale_code = Get_Bits(5);
      ld->quantizer_scale =
        ld->q_scale_type ? Non_Linear_quantizer_scale[quantizer_scale_code] : quantizer_scale_code<<1;
    }

    /* macroblock_pattern */
    if (SNRmacroblock_type & MACROBLOCK_PATTERN)
    {
      SNRcoded_block_pattern = Get_coded_block_pattern();

      if (chroma_format==CHROMA422)
        SNRcoded_block_pattern = (SNRcoded_block_pattern<<2) | Get_Bits(2); /* coded_block_pattern_1 */
      else if (chroma_format==CHROMA444)
        SNRcoded_block_pattern = (SNRcoded_block_pattern<<6) | Get_Bits(6); /* coded_block_pattern_2 */
    }
    else
      SNRcoded_block_pattern = 0;

    /* decode blocks */
    for (comp=0; comp<block_count; comp++)
    {
      Clear_Block(comp);

      if (SNRcoded_block_pattern & (1<<(block_count-1-comp)))
        Decode_MPEG2_Non_Intra_Block(comp);
    }
  }
  else /* SNRMBAinc!=1: skipped macroblock */
  {
    for (comp=0; comp<block_count; comp++)
      Clear_Block(comp);
  }

  ld = &base;
}



/* IMPLEMENTATION: set scratch pad macroblock to zero */
static void Clear_Block(comp)
int comp;
{
  short *Block_Ptr;
  int i;

  Block_Ptr = ld->block[comp];

  for (i=0; i<64; i++)
    *Block_Ptr++ = 0;
}


/* SCALABILITY: add SNR enhancement layer block data to base layer */
/* ISO/IEC 13818-2 section 7.8.3.4: Addition of coefficients from the two layes */
static void Sum_Block(comp)
int comp;
{
  short *Block_Ptr1, *Block_Ptr2;
  int i;

  Block_Ptr1 = base.block[comp];
  Block_Ptr2 = enhan.block[comp];

  for (i=0; i<64; i++)
    *Block_Ptr1++ += *Block_Ptr2++;
}


/* limit coefficients to -2048..2047 */
/* ISO/IEC 13818-2 section 7.4.3 and 7.4.4: Saturation and Mismatch control */
static void Saturate(Block_Ptr)
short *Block_Ptr;
{
  int i, sum, val;

  sum = 0;

  /* ISO/IEC 13818-2 section 7.4.3: Saturation */
  for (i=0; i<64; i++)
  {
    val = Block_Ptr[i];

    if (val>2047)
      val = 2047;
    else if (val<-2048)
      val = -2048;

    Block_Ptr[i] = val;
    sum+= val;
  }

  /* ISO/IEC 13818-2 section 7.4.4: Mismatch control */
  if ((sum&1)==0)
    Block_Ptr[63]^= 1;

}


/* reuse old picture buffers as soon as they are no longer needed 
   based on life-time axioms of MPEG */
static void Update_Picture_Buffers()
{                           
  int cc;              /* color component index */
  unsigned char *tmp;  /* temporary swap pointer */

  for (cc=0; cc<3; cc++)
  {
    /* B pictures do not need to be save for future reference */
    if (picture_coding_type==B_TYPE)
    {
      current_frame[cc] = auxframe[cc];
    }
    else
    {
      /* only update at the beginning of the coded frame */
      if (!Second_Field)
      {
        tmp = forward_reference_frame[cc];

        /* the previously decoded reference frame is stored
           coincident with the location where the backward 
           reference frame is stored (backwards prediction is not
           needed in P pictures) */
        forward_reference_frame[cc] = backward_reference_frame[cc];
        
        /* update pointer for potential future B pictures */
        backward_reference_frame[cc] = tmp;
      }

      /* can erase over old backward reference frame since it is not used
         in a P picture, and since any subsequent B pictures will use the 
         previously decoded I or P frame as the backward_reference_frame */
      current_frame[cc] = backward_reference_frame[cc];
    }

    /* IMPLEMENTATION:
       one-time folding of a line offset into the pointer which stores the
       memory address of the current frame saves offsets and conditional 
       branches throughout the remainder of the picture processing loop */
    if (picture_structure==BOTTOM_FIELD)
      current_frame[cc]+= (cc==0) ? Coded_Picture_Width : Chroma_Width;
  }
}


/* store last frame */

void Output_Last_Frame_of_Sequence(Framenum)
int Framenum;
{
  if (Second_Field)
    printf("last frame incomplete, not stored\n");
  else
    Write_Frame(backward_reference_frame,Framenum-1);
}



static void frame_reorder(Bitstream_Framenum, Sequence_Framenum)
int Bitstream_Framenum, Sequence_Framenum;
{
  /* tracking variables to insure proper output in spatial scalability */
  static int Oldref_progressive_frame, Newref_progressive_frame;

  if (Sequence_Framenum!=0)
  {
    if (picture_structure==FRAME_PICTURE || Second_Field)
    {
      if (picture_coding_type==B_TYPE)
        Write_Frame(auxframe,Bitstream_Framenum-1);
      else
      {
        Newref_progressive_frame = progressive_frame;
        progressive_frame = Oldref_progressive_frame;

        Write_Frame(forward_reference_frame,Bitstream_Framenum-1);

        Oldref_progressive_frame = progressive_frame = Newref_progressive_frame;
      }
    }
#ifdef DISPLAY
    else if (Output_Type==T_X11)
    {
      if(!Display_Progressive_Flag)
        Display_Second_Field();
    }
#endif
  }
  else
    Oldref_progressive_frame = progressive_frame;

}


/* ISO/IEC 13818-2 section 7.6 */
static void motion_compensation(MBA, macroblock_type, motion_type, PMV, 
  motion_vertical_field_select, dmvector, stwtype, dct_type)
int MBA;
int macroblock_type;
int motion_type;
int PMV[2][2][2];
int motion_vertical_field_select[2][2];
int dmvector[2];
int stwtype;
int dct_type;
{
  int bx, by;
  int comp;

  /* derive current macroblock position within picture */
  /* ISO/IEC 13818-2 section 6.3.1.6 and 6.3.1.7 */
  bx = 16*(MBA%mb_width);
  by = 16*(MBA/mb_width);

  /* motion compensation */
  if (!(macroblock_type & MACROBLOCK_INTRA))
    form_predictions(bx,by,macroblock_type,motion_type,PMV,
      motion_vertical_field_select,dmvector,stwtype);
  
  /* SCALABILITY: Data Partitioning */
  if (base.scalable_mode==SC_DP)
    ld = &base;

  /* copy or add block data into picture */
  for (comp=0; comp<block_count; comp++)
  {
    /* SCALABILITY: SNR */
    /* ISO/IEC 13818-2 section 7.8.3.4: Addition of coefficients from 
       the two a layers */
    if (Two_Streams && enhan.scalable_mode==SC_SNR)
      Sum_Block(comp); /* add SNR enhancement layer data to base layer */

    /* MPEG-2 saturation and mismatch control */
    /* base layer could be MPEG-1 stream, enhancement MPEG-2 SNR */
    /* ISO/IEC 13818-2 section 7.4.3 and 7.4.4: Saturation and Mismatch control */
    if ((Two_Streams && enhan.scalable_mode==SC_SNR) || ld->MPEG2_Flag)
      Saturate(ld->block[comp]);

    /* ISO/IEC 13818-2 section Annex A: inverse DCT */
    if (Reference_IDCT_Flag)
      Reference_IDCT(ld->block[comp]);
    else
      Fast_IDCT(ld->block[comp]);
    
    /* ISO/IEC 13818-2 section 7.6.8: Adding prediction and coefficient data */
    Add_Block(comp,bx,by,dct_type,(macroblock_type & MACROBLOCK_INTRA)==0);
  }

}



/* ISO/IEC 13818-2 section 7.6.6 */
static void skipped_macroblock(dc_dct_pred, PMV, motion_type, 
  motion_vertical_field_select, stwtype, macroblock_type)
int dc_dct_pred[3];
int PMV[2][2][2];
int *motion_type;
int motion_vertical_field_select[2][2];
int *stwtype;
int *macroblock_type;
{
  int comp;
  
  /* SCALABILITY: Data Paritioning */
  if (base.scalable_mode==SC_DP)
    ld = &base;

  for (comp=0; comp<block_count; comp++)
    Clear_Block(comp);

  /* reset intra_dc predictors */
  /* ISO/IEC 13818-2 section 7.2.1: DC coefficients in intra blocks */
  dc_dct_pred[0]=dc_dct_pred[1]=dc_dct_pred[2]=0;

  /* reset motion vector predictors */
  /* ISO/IEC 13818-2 section 7.6.3.4: Resetting motion vector predictors */
  if (picture_coding_type==P_TYPE)
    PMV[0][0][0]=PMV[0][0][1]=PMV[1][0][0]=PMV[1][0][1]=0;

  /* derive motion_type */
  if (picture_structure==FRAME_PICTURE)
    *motion_type = MC_FRAME;
  else
  {
    *motion_type = MC_FIELD;

    /* predict from field of same parity */
    /* ISO/IEC 13818-2 section 7.6.6.1 and 7.6.6.3: P field picture and B field
       picture */
    motion_vertical_field_select[0][0]=motion_vertical_field_select[0][1] = 
      (picture_structure==BOTTOM_FIELD);
  }

  /* skipped I are spatial-only predicted, */
  /* skipped P and B are temporal-only predicted */
  /* ISO/IEC 13818-2 section 7.7.6: Skipped macroblocks */
  *stwtype = (picture_coding_type==I_TYPE) ? 8 : 0;

 /* IMPLEMENTATION: clear MACROBLOCK_INTRA */
  *macroblock_type&= ~MACROBLOCK_INTRA;

}


/* return==-1 means go to next picture */
/* the expression "start of slice" is used throughout the normative
   body of the MPEG specification */
static int start_of_slice(MBAmax, MBA, MBAinc, 
  dc_dct_pred, PMV)
int MBAmax;
int *MBA;
int *MBAinc;
int dc_dct_pred[3];
int PMV[2][2][2];
{
  unsigned int code;
  int slice_vert_pos_ext;

  ld = &base;

  Fault_Flag = 0;

  next_start_code();
  code = Show_Bits(32);

  if (code<SLICE_START_CODE_MIN || code>SLICE_START_CODE_MAX)
  {
    /* only slice headers are allowed in picture_data */
    if (!Quiet_Flag)
      printf("start_of_slice(): Premature end of picture\n");

    return(-1);  /* trigger: go to next picture */
  }

  Flush_Buffer32(); 

  /* decode slice header (may change quantizer_scale) */
  slice_vert_pos_ext = slice_header();

 
  /* SCALABILITY: Data Partitioning */
  if (base.scalable_mode==SC_DP)
  {
    ld = &enhan;
    next_start_code();
    code = Show_Bits(32);

    if (code<SLICE_START_CODE_MIN || code>SLICE_START_CODE_MAX)
    {
      /* only slice headers are allowed in picture_data */
      if (!Quiet_Flag)
        printf("DP: Premature end of picture\n");
      return(-1);    /* trigger: go to next picture */
    }

    Flush_Buffer32();

    /* decode slice header (may change quantizer_scale) */
    slice_vert_pos_ext = slice_header();

    if (base.priority_breakpoint!=1)
      ld = &base;
  }

  /* decode macroblock address increment */
  *MBAinc = Get_macroblock_address_increment();

  if (Fault_Flag) 
  {
    printf("start_of_slice(): MBAinc unsuccessful\n");
    return(0);   /* trigger: go to next slice */
  }

  /* set current location */
  /* NOTE: the arithmetic used to derive macroblock_address below is
   *       equivalent to ISO/IEC 13818-2 section 6.3.17: Macroblock
   */
  *MBA = ((slice_vert_pos_ext<<7) + (code&255) - 1)*mb_width + *MBAinc - 1;
  *MBAinc = 1; /* first macroblock in slice: not skipped */

  /* reset all DC coefficient and motion vector predictors */
  /* reset all DC coefficient and motion vector predictors */
  /* ISO/IEC 13818-2 section 7.2.1: DC coefficients in intra blocks */
  dc_dct_pred[0]=dc_dct_pred[1]=dc_dct_pred[2]=0;
  
  /* ISO/IEC 13818-2 section 7.6.3.4: Resetting motion vector predictors */
  PMV[0][0][0]=PMV[0][0][1]=PMV[1][0][0]=PMV[1][0][1]=0;
  PMV[0][1][0]=PMV[0][1][1]=PMV[1][1][0]=PMV[1][1][1]=0;

  /* successfull: trigger decode macroblocks in slice */
  return(1);
}


/* ISO/IEC 13818-2 sections 7.2 through 7.5 */
static int decode_macroblock(macroblock_type, stwtype, stwclass,
  motion_type, dct_type, PMV, dc_dct_pred, 
  motion_vertical_field_select, dmvector)
int *macroblock_type; 
int *stwtype;
int *stwclass;
int *motion_type; 
int *dct_type;
int PMV[2][2][2]; 
int dc_dct_pred[3]; 
int motion_vertical_field_select[2][2];
int dmvector[2];
{
  /* locals */
  int quantizer_scale_code; 
  int comp;

  int motion_vector_count; 
  int mv_format; 
  int dmv; 
  int mvscale;
  int coded_block_pattern;

  /* SCALABILITY: Data Patitioning */
  if (base.scalable_mode==SC_DP)
  {
    if (base.priority_breakpoint<=2)
      ld = &enhan;
    else
      ld = &base;
  }

  /* ISO/IEC 13818-2 section 6.3.17.1: Macroblock modes */
  macroblock_modes(macroblock_type, stwtype, stwclass,
    motion_type, &motion_vector_count, &mv_format, &dmv, &mvscale,
    dct_type);

  if (Fault_Flag) return(0);  /* trigger: go to next slice */

  if (*macroblock_type & MACROBLOCK_QUANT)
  {
    quantizer_scale_code = Get_Bits(5);

#ifdef TRACE
    if (Trace_Flag)
    {
      printf("quantiser_scale_code (");
      printf_Bits(quantizer_scale_code,5,5);
      printf("): %d\n",quantizer_scale_code);
    }
#endif /* TRACE */

    /* ISO/IEC 13818-2 section 7.4.2.2: Quantizer scale factor */
    if (ld->MPEG2_Flag)
      ld->quantizer_scale =
      ld->q_scale_type ? Non_Linear_quantizer_scale[quantizer_scale_code] 
       : (quantizer_scale_code << 1);
    else
      ld->quantizer_scale = quantizer_scale_code;

    /* SCALABILITY: Data Partitioning */
    if (base.scalable_mode==SC_DP)
      /* make sure base.quantizer_scale is valid */
      base.quantizer_scale = ld->quantizer_scale;
  }

  /* motion vectors */


  /* ISO/IEC 13818-2 section 6.3.17.2: Motion vectors */

  /* decode forward motion vectors */
  if ((*macroblock_type & MACROBLOCK_MOTION_FORWARD) 
    || ((*macroblock_type & MACROBLOCK_INTRA) 
    && concealment_motion_vectors))
  {
    if (ld->MPEG2_Flag)
      motion_vectors(PMV,dmvector,motion_vertical_field_select,
        0,motion_vector_count,mv_format,f_code[0][0]-1,f_code[0][1]-1,
        dmv,mvscale);
    else
      motion_vector(PMV[0][0],dmvector,
      forward_f_code-1,forward_f_code-1,0,0,full_pel_forward_vector);
  }

  if (Fault_Flag) return(0);  /* trigger: go to next slice */

  /* decode backward motion vectors */
  if (*macroblock_type & MACROBLOCK_MOTION_BACKWARD)
  {
    if (ld->MPEG2_Flag)
      motion_vectors(PMV,dmvector,motion_vertical_field_select,
        1,motion_vector_count,mv_format,f_code[1][0]-1,f_code[1][1]-1,0,
        mvscale);
    else
      motion_vector(PMV[0][1],dmvector,
        backward_f_code-1,backward_f_code-1,0,0,full_pel_backward_vector);
  }

  if (Fault_Flag) return(0);  /* trigger: go to next slice */

  if ((*macroblock_type & MACROBLOCK_INTRA) && concealment_motion_vectors)
    Flush_Buffer(1); /* remove marker_bit */

  if (base.scalable_mode==SC_DP && base.priority_breakpoint==3)
    ld = &enhan;

  /* macroblock_pattern */
  /* ISO/IEC 13818-2 section 6.3.17.4: Coded block pattern */
  if (*macroblock_type & MACROBLOCK_PATTERN)
  {
    coded_block_pattern = Get_coded_block_pattern();

    if (chroma_format==CHROMA422)
    {
      /* coded_block_pattern_1 */
      coded_block_pattern = (coded_block_pattern<<2) | Get_Bits(2); 

#ifdef TRACE
       if (Trace_Flag)
       {
         printf("coded_block_pattern_1: ");
         printf_Bits(coded_block_pattern,2,2);
         printf(" (%d)\n",coded_block_pattern&3);
       }
#endif /* TRACE */
     }
     else if (chroma_format==CHROMA444)
     {
      /* coded_block_pattern_2 */
      coded_block_pattern = (coded_block_pattern<<6) | Get_Bits(6); 

#ifdef TRACE
      if (Trace_Flag)
      {
        printf("coded_block_pattern_2: ");
        printf_Bits(coded_block_pattern,6,6);
        printf(" (%d)\n",coded_block_pattern&63);
      }
#endif /* TRACE */
    }
  }
  else
    coded_block_pattern = (*macroblock_type & MACROBLOCK_INTRA) ? 
      (1<<block_count)-1 : 0;

  if (Fault_Flag) return(0);  /* trigger: go to next slice */

  /* decode blocks */
  for (comp=0; comp<block_count; comp++)
  {
    /* SCALABILITY: Data Partitioning */
    if (base.scalable_mode==SC_DP)
    ld = &base;

    Clear_Block(comp);

    if (coded_block_pattern & (1<<(block_count-1-comp)))
    {
      if (*macroblock_type & MACROBLOCK_INTRA)
      {
        if (ld->MPEG2_Flag)
          Decode_MPEG2_Intra_Block(comp,dc_dct_pred);
        else
          Decode_MPEG1_Intra_Block(comp,dc_dct_pred);
      }
      else
      {
        if (ld->MPEG2_Flag)
          Decode_MPEG2_Non_Intra_Block(comp);
        else
          Decode_MPEG1_Non_Intra_Block(comp);
      }

      if (Fault_Flag) return(0);  /* trigger: go to next slice */
    }
  }

  if(picture_coding_type==D_TYPE)
  {
    /* remove end_of_macroblock (always 1, prevents startcode emulation) */
    /* ISO/IEC 11172-2 section 2.4.2.7 and 2.4.3.6 */
    marker_bit("D picture end_of_macroblock bit");
  }

  /* reset intra_dc predictors */
  /* ISO/IEC 13818-2 section 7.2.1: DC coefficients in intra blocks */
  if (!(*macroblock_type & MACROBLOCK_INTRA))
    dc_dct_pred[0]=dc_dct_pred[1]=dc_dct_pred[2]=0;

  /* reset motion vector predictors */
  if ((*macroblock_type & MACROBLOCK_INTRA) && !concealment_motion_vectors)
  {
    /* intra mb without concealment motion vectors */
    /* ISO/IEC 13818-2 section 7.6.3.4: Resetting motion vector predictors */
    PMV[0][0][0]=PMV[0][0][1]=PMV[1][0][0]=PMV[1][0][1]=0;
    PMV[0][1][0]=PMV[0][1][1]=PMV[1][1][0]=PMV[1][1][1]=0;
  }

  /* special "No_MC" macroblock_type case */
  /* ISO/IEC 13818-2 section 7.6.3.5: Prediction in P pictures */
  if ((picture_coding_type==P_TYPE) 
    && !(*macroblock_type & (MACROBLOCK_MOTION_FORWARD|MACROBLOCK_INTRA)))
  {
    /* non-intra mb without forward mv in a P picture */
    /* ISO/IEC 13818-2 section 7.6.3.4: Resetting motion vector predictors */
    PMV[0][0][0]=PMV[0][0][1]=PMV[1][0][0]=PMV[1][0][1]=0;

    /* derive motion_type */
    /* ISO/IEC 13818-2 section 6.3.17.1: Macroblock modes, frame_motion_type */
    if (picture_structure==FRAME_PICTURE)
      *motion_type = MC_FRAME;
    else
    {
      *motion_type = MC_FIELD;
      /* predict from field of same parity */
      motion_vertical_field_select[0][0] = (picture_structure==BOTTOM_FIELD);
    }
  }

  if (*stwclass==4)
  {
    /* purely spatially predicted macroblock */
    /* ISO/IEC 13818-2 section 7.7.5.1: Resetting motion vector predictions */
    PMV[0][0][0]=PMV[0][0][1]=PMV[1][0][0]=PMV[1][0][1]=0;
    PMV[0][1][0]=PMV[0][1][1]=PMV[1][1][0]=PMV[1][1][1]=0;
  }

  /* successfully decoded macroblock */
  return(1);

} /* decode_macroblock */


