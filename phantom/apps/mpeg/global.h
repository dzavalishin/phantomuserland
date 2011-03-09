/* global.h, global variables                                               */

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

#include "tabos_compat.h"

#include "mpeg2dec.h"

/* choose between declaration (GLOBAL undefined)
 * and definition (GLOBAL defined)
 * GLOBAL is defined in exactly one file mpeg2dec.c)
 */

#ifndef GLOBAL
#define EXTERN extern
#else
#define EXTERN
#endif

/* prototypes of global functions */
/* readpic.c */
void Substitute_Frame_Buffer _ANSI_ARGS_ ((int bitstream_framenum, 
  int sequence_framenum));

/* Get_Bits.c */
void Initialize_Buffer _ANSI_ARGS_((void));
void Fill_Buffer _ANSI_ARGS_((void));
unsigned int Show_Bits _ANSI_ARGS_((int n));
unsigned int Get_Bits1 _ANSI_ARGS_((void));
void Flush_Buffer _ANSI_ARGS_((int n));
unsigned int Get_Bits _ANSI_ARGS_((int n));
int Get_Byte _ANSI_ARGS_((void));
int Get_Word _ANSI_ARGS_((void));

/* systems.c */
void Next_Packet _ANSI_ARGS_((void));
int Get_Long _ANSI_ARGS_((void));
void Flush_Buffer32 _ANSI_ARGS_((void));
unsigned int Get_Bits32 _ANSI_ARGS_((void));


/* getblk.c */
void Decode_MPEG1_Intra_Block _ANSI_ARGS_((int comp, int dc_dct_pred[]));
void Decode_MPEG1_Non_Intra_Block _ANSI_ARGS_((int comp));
void Decode_MPEG2_Intra_Block _ANSI_ARGS_((int comp, int dc_dct_pred[]));
void Decode_MPEG2_Non_Intra_Block _ANSI_ARGS_((int comp));

/* gethdr.c */
int Get_Hdr _ANSI_ARGS_((void));
void next_start_code _ANSI_ARGS_((void));
int slice_header _ANSI_ARGS_((void));
void marker_bit _ANSI_ARGS_((char *text));

/* getpic.c */
void Decode_Picture _ANSI_ARGS_((int bitstream_framenum, 
  int sequence_framenum));
void Output_Last_Frame_of_Sequence _ANSI_ARGS_((int framenum));

/* getvlc.c */
int Get_macroblock_type _ANSI_ARGS_((void));
int Get_motion_code _ANSI_ARGS_((void));
int Get_dmvector _ANSI_ARGS_((void));
int Get_coded_block_pattern _ANSI_ARGS_((void));
int Get_macroblock_address_increment _ANSI_ARGS_((void));
int Get_Luma_DC_dct_diff _ANSI_ARGS_((void));
int Get_Chroma_DC_dct_diff _ANSI_ARGS_((void));

/* idct.c */
void Fast_IDCT _ANSI_ARGS_((short *block));
void Initialize_Fast_IDCT _ANSI_ARGS_((void));

/* Reference_IDCT.c */
void Initialize_Reference_IDCT _ANSI_ARGS_((void));
void Reference_IDCT _ANSI_ARGS_((short *block));

/* motion.c */
void motion_vectors _ANSI_ARGS_((int PMV[2][2][2], int dmvector[2],
  int motion_vertical_field_select[2][2], int s, int motion_vector_count, 
  int mv_format, int h_r_size, int v_r_size, int dmv, int mvscale));
void motion_vector _ANSI_ARGS_((int *PMV, int *dmvector,
  int h_r_size, int v_r_size, int dmv, int mvscale, int full_pel_vector));
void Dual_Prime_Arithmetic _ANSI_ARGS_((int DMV[][2], int *dmvector, int mvx, int mvy));

/* mpeg2dec.c */
void Error _ANSI_ARGS_((char *text));
void Warning _ANSI_ARGS_((char *text));
void Print_Bits _ANSI_ARGS_((int code, int bits, int len));

/* recon.c */
void form_predictions _ANSI_ARGS_((int bx, int by, int macroblock_type, 
  int motion_type, int PMV[2][2][2], int motion_vertical_field_select[2][2], 
  int dmvector[2], int stwtype));

/* spatscal.c */
void Spatial_Prediction _ANSI_ARGS_((void));

/* store.c */
void Write_Frame _ANSI_ARGS_((unsigned char *src[], int frame));

#ifdef DISPLAY
/* display.c */
void Initialize_Display_Process _ANSI_ARGS_((char *name));
void Terminate_Display_Process _ANSI_ARGS_((void));
void Display_Second_Field _ANSI_ARGS_((void));
void dither _ANSI_ARGS_((unsigned char *src[]));
void Initialize_Dither_Matrix _ANSI_ARGS_((void));
#endif

/* global variables */

EXTERN char Version[]
#ifdef GLOBAL
  ="mpeg2decode V1.2a, 96/07/19"
#endif
;

EXTERN char Author[]
#ifdef GLOBAL
  ="(C) 1996, MPEG Software Simulation Group"
#endif
;


/* zig-zag and alternate scan patterns */
EXTERN unsigned char scan[2][64]
#ifdef GLOBAL
=
{
  { /* Zig-Zag scan pattern  */
    0,1,8,16,9,2,3,10,17,24,32,25,18,11,4,5,
    12,19,26,33,40,48,41,34,27,20,13,6,7,14,21,28,
    35,42,49,56,57,50,43,36,29,22,15,23,30,37,44,51,
    58,59,52,45,38,31,39,46,53,60,61,54,47,55,62,63
  },
  { /* Alternate scan pattern */
    0,8,16,24,1,9,2,10,17,25,32,40,48,56,57,49,
    41,33,26,18,3,11,4,12,19,27,34,42,50,58,35,43,
    51,59,20,28,5,13,6,14,21,29,36,44,52,60,37,45,
    53,61,22,30,7,15,23,31,38,46,54,62,39,47,55,63
  }
}
#endif
;

/* default intra quantization matrix */
EXTERN unsigned char default_intra_quantizer_matrix[64]
#ifdef GLOBAL
=
{
  8, 16, 19, 22, 26, 27, 29, 34,
  16, 16, 22, 24, 27, 29, 34, 37,
  19, 22, 26, 27, 29, 34, 34, 38,
  22, 22, 26, 27, 29, 34, 37, 40,
  22, 26, 27, 29, 32, 35, 40, 48,
  26, 27, 29, 32, 35, 40, 48, 58,
  26, 27, 29, 34, 38, 46, 56, 69,
  27, 29, 35, 38, 46, 56, 69, 83
}
#endif
;

/* non-linear quantization coefficient table */
EXTERN unsigned char Non_Linear_quantizer_scale[32]
#ifdef GLOBAL
=
{
   0, 1, 2, 3, 4, 5, 6, 7,
   8,10,12,14,16,18,20,22,
  24,28,32,36,40,44,48,52,
  56,64,72,80,88,96,104,112
}
#endif
;

/* color space conversion coefficients
 * for YCbCr -> RGB mapping
 *
 * entries are {crv,cbu,cgu,cgv}
 *
 * crv=(255/224)*65536*(1-cr)/0.5
 * cbu=(255/224)*65536*(1-cb)/0.5
 * cgu=(255/224)*65536*(cb/cg)*(1-cb)/0.5
 * cgv=(255/224)*65536*(cr/cg)*(1-cr)/0.5
 *
 * where Y=cr*R+cg*G+cb*B (cr+cg+cb=1)
 */

/* ISO/IEC 13818-2 section 6.3.6 sequence_display_extension() */

EXTERN int Inverse_Table_6_9[8][4]
#ifdef GLOBAL
=
{
  {117504, 138453, 13954, 34903}, /* no sequence_display_extension */
  {117504, 138453, 13954, 34903}, /* ITU-R Rec. 709 (1990) */
  {104597, 132201, 25675, 53279}, /* unspecified */
  {104597, 132201, 25675, 53279}, /* reserved */
  {104448, 132798, 24759, 53109}, /* FCC */
  {104597, 132201, 25675, 53279}, /* ITU-R Rec. 624-4 System B, G */
  {104597, 132201, 25675, 53279}, /* SMPTE 170M */
  {117579, 136230, 16907, 35559}  /* SMPTE 240M (1987) */
}
#endif
;





/* output types (Output_Type) */
#define T_YUV   0
#define T_SIF   1
#define T_TGA   2
#define T_PPM   3
#define T_X11   4
#define T_X11HIQ 5
#define T_SPOON 6

/* decoder operation control variables */
EXTERN int Output_Type;
EXTERN int hiQdither;

/* decoder operation control flags */
EXTERN int Quiet_Flag;
EXTERN int Trace_Flag;
EXTERN int Fault_Flag;
EXTERN int Verbose_Flag;
EXTERN int Two_Streams;
EXTERN int Spatial_Flag;
EXTERN int Reference_IDCT_Flag;
EXTERN int Frame_Store_Flag;
EXTERN int System_Stream_Flag;
EXTERN int Display_Progressive_Flag;
EXTERN int Ersatz_Flag;
EXTERN int Big_Picture_Flag;
EXTERN int Verify_Flag;
EXTERN int Stats_Flag;
EXTERN int User_Data_Flag;
EXTERN int Main_Bitstream_Flag;


/* filenames */
EXTERN char *Output_Picture_Filename;
EXTERN char *Substitute_Picture_Filename;
EXTERN char *Main_Bitstream_Filename; 
EXTERN char *Enhancement_Layer_Bitstream_Filename; 


/* buffers for multiuse purposes */
EXTERN char Error_Text[256];
EXTERN unsigned char *Clip;

/* pointers to generic picture buffers */
EXTERN unsigned char *backward_reference_frame[3];
EXTERN unsigned char *forward_reference_frame[3];

EXTERN unsigned char *auxframe[3];
EXTERN unsigned char *current_frame[3];
EXTERN unsigned char *substitute_frame[3];


/* pointers to scalability picture buffers */
EXTERN unsigned char *llframe0[3];
EXTERN unsigned char *llframe1[3];

EXTERN short *lltmp;
EXTERN char *Lower_Layer_Picture_Filename;




/* non-normative variables derived from normative elements */
EXTERN int Coded_Picture_Width;
EXTERN int Coded_Picture_Height;
EXTERN int Chroma_Width;
EXTERN int Chroma_Height;
EXTERN int block_count;
EXTERN int Second_Field;
EXTERN int profile, level;

/* normative derived variables (as per ISO/IEC 13818-2) */
EXTERN int horizontal_size;
EXTERN int vertical_size;
EXTERN int mb_width;
EXTERN int mb_height;
EXTERN double bit_rate;
EXTERN double frame_rate; 



/* headers */

/* ISO/IEC 13818-2 section 6.2.2.1:  sequence_header() */
EXTERN int aspect_ratio_information;
EXTERN int frame_rate_code; 
EXTERN int bit_rate_value; 
EXTERN int vbv_buffer_size;
EXTERN int constrained_parameters_flag;

/* ISO/IEC 13818-2 section 6.2.2.3:  sequence_extension() */
EXTERN int profile_and_level_indication;
EXTERN int progressive_sequence;
EXTERN int chroma_format;
EXTERN int low_delay;
EXTERN int frame_rate_extension_n;
EXTERN int frame_rate_extension_d;

/* ISO/IEC 13818-2 section 6.2.2.4:  sequence_display_extension() */
EXTERN int video_format;  
EXTERN int color_description;
EXTERN int color_primaries;
EXTERN int transfer_characteristics;
EXTERN int matrix_coefficients;
EXTERN int display_horizontal_size;
EXTERN int display_vertical_size;

/* ISO/IEC 13818-2 section 6.2.3: picture_header() */
EXTERN int temporal_reference;
EXTERN int picture_coding_type;
EXTERN int vbv_delay;
EXTERN int full_pel_forward_vector;
EXTERN int forward_f_code;
EXTERN int full_pel_backward_vector;
EXTERN int backward_f_code;


/* ISO/IEC 13818-2 section 6.2.3.1: picture_coding_extension() header */
EXTERN int f_code[2][2];
EXTERN int intra_dc_precision;
EXTERN int picture_structure;
EXTERN int top_field_first;
EXTERN int frame_pred_frame_dct;
EXTERN int concealment_motion_vectors;

EXTERN int intra_vlc_format;

EXTERN int repeat_first_field;

EXTERN int chroma_420_type;
EXTERN int progressive_frame;
EXTERN int composite_display_flag;
EXTERN int v_axis;
EXTERN int field_sequence;
EXTERN int sub_carrier;
EXTERN int burst_amplitude;
EXTERN int sub_carrier_phase;



/* ISO/IEC 13818-2 section 6.2.3.3: picture_display_extension() header */
EXTERN int frame_center_horizontal_offset[3];
EXTERN int frame_center_vertical_offset[3];



/* ISO/IEC 13818-2 section 6.2.2.5: sequence_scalable_extension() header */
EXTERN int layer_id;
EXTERN int lower_layer_prediction_horizontal_size;
EXTERN int lower_layer_prediction_vertical_size;
EXTERN int horizontal_subsampling_factor_m;
EXTERN int horizontal_subsampling_factor_n;
EXTERN int vertical_subsampling_factor_m;
EXTERN int vertical_subsampling_factor_n;


/* ISO/IEC 13818-2 section 6.2.3.5: picture_spatial_scalable_extension() header */
EXTERN int lower_layer_temporal_reference;
EXTERN int lower_layer_horizontal_offset;
EXTERN int lower_layer_vertical_offset;
EXTERN int spatial_temporal_weight_code_table_index;
EXTERN int lower_layer_progressive_frame;
EXTERN int lower_layer_deinterlaced_field_select;






/* ISO/IEC 13818-2 section 6.2.3.6: copyright_extension() header */
EXTERN int copyright_flag;
EXTERN int copyright_identifier;
EXTERN int original_or_copy;
EXTERN int copyright_number_1;
EXTERN int copyright_number_2;
EXTERN int copyright_number_3;

/* ISO/IEC 13818-2 section 6.2.2.6: group_of_pictures_header()  */
EXTERN int drop_flag;
EXTERN int hour;
EXTERN int minute;
EXTERN int sec;
EXTERN int frame;
EXTERN int closed_gop;
EXTERN int broken_link;



/* layer specific variables (needed for SNR and DP scalability) */
EXTERN struct layer_data {
  /* bit input */
  int Infile;
  unsigned char Rdbfr[2048];
  unsigned char *Rdptr;
  unsigned char Inbfr[16];
  /* from mpeg2play */
  unsigned int Bfr;
  unsigned char *Rdmax;
  int Incnt;
  int Bitcnt;
  /* sequence header and quant_matrix_extension() */
  int intra_quantizer_matrix[64];
  int non_intra_quantizer_matrix[64];
  int chroma_intra_quantizer_matrix[64];
  int chroma_non_intra_quantizer_matrix[64];
  
  int load_intra_quantizer_matrix;
  int load_non_intra_quantizer_matrix;
  int load_chroma_intra_quantizer_matrix;
  int load_chroma_non_intra_quantizer_matrix;

  int MPEG2_Flag;
  /* sequence scalable extension */
  int scalable_mode;
  /* picture coding extension */
  int q_scale_type;
  int alternate_scan;
  /* picture spatial scalable extension */
  int pict_scal;
  /* slice/macroblock */
  int priority_breakpoint;
  int quantizer_scale;
  int intra_slice;
  short block[12][64];
} base, enhan, *ld;



#ifdef VERIFY
EXTERN int verify_sequence_header;
EXTERN int verify_group_of_pictures_header;
EXTERN int verify_picture_header;
EXTERN int verify_slice_header;
EXTERN int verify_sequence_extension;
EXTERN int verify_sequence_display_extension;
EXTERN int verify_quant_matrix_extension;
EXTERN int verify_sequence_scalable_extension;
EXTERN int verify_picture_display_extension;
EXTERN int verify_picture_coding_extension;
EXTERN int verify_picture_spatial_scalable_extension;
EXTERN int verify_picture_temporal_scalable_extension;
EXTERN int verify_copyright_extension;
#endif /* VERIFY */


EXTERN int Decode_Layer;

/* verify.c */
#ifdef VERIFY
void Check_Headers _ANSI_ARGS_((int Bitstream_Framenum, int Sequence_Framenum));
void Clear_Verify_Headers _ANSI_ARGS_((void));
#endif /* VERIFY */


EXTERN int global_MBA;
EXTERN int global_pic;
EXTERN int True_Framenum;
