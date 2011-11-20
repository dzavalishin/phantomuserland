/* #define DEBUG */
/* subspic.c, Frame buffer substitution routines */

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
#include <unistd.h>
#include <stdlib.h>
#include "fcntl.h"

#include "config.h"
#include "global.h"

/* private prototypes*/
static void Read_Frame _ANSI_ARGS_((char *filename,
  unsigned char *frame_buffer[], int framenum));
static void Copy_Frame _ANSI_ARGS_((unsigned char *src, unsigned char *dst,
  int width, int height, int parity, int incr));
static int Read_Components _ANSI_ARGS_ ((char *filename,
  unsigned char *frame[3], int framenum));
static int Read_Component _ANSI_ARGS_ ((char *fname, unsigned char *frame,
  int width, int height));
static int Extract_Components _ANSI_ARGS_ ((char *filename,
  unsigned char *frame[3], int framenum));


/* substitute frame buffer routine */
void Substitute_Frame_Buffer (bitstream_framenum, sequence_framenum)
int bitstream_framenum;
int sequence_framenum;
{
  /* static tracking variables */
  static int previous_temporal_reference;
  static int previous_bitstream_framenum;
  static int previous_anchor_temporal_reference;
  static int previous_anchor_bitstream_framenum;
  static int previous_picture_coding_type;
  static int bgate;

  /* local temporary variables */
  int substitute_display_framenum;


#ifdef DEBUG
  Print("SUB: seq fn(%d) bitfn(%d) tempref(%d) picstr(%d) type(%d)\n",
    sequence_framenum, bitstream_framenum, temporal_reference,
    picture_structure, picture_coding_type);
#endif

  /* we don't substitute at the first picture of a sequence */
  if((sequence_framenum!=0)||(Second_Field))
  {
    /* only at the start of the frame */
    if ((picture_structure==FRAME_PICTURE)||(!Second_Field))
    {
      if(picture_coding_type==P_TYPE)
      {
        /* the most recently decoded reference frame needs substituting */
        substitute_display_framenum = bitstream_framenum - 1;

        Read_Frame(Substitute_Picture_Filename, forward_reference_frame,
          substitute_display_framenum);
      }
      /* only the first B frame in a consequitve set of B pictures
         loads a substitute backward_reference_frame since all subsequent
         B frames predict from the same reference pictures */
      else if((picture_coding_type==B_TYPE)&&(bgate!=1))
      {
        substitute_display_framenum =
          (previous_temporal_reference - temporal_reference)
            + bitstream_framenum - 1;

        Read_Frame(Substitute_Picture_Filename, backward_reference_frame,
          substitute_display_framenum);
      }
    } /* P fields can predict from the two most recently decoded fields, even
         from the first field of the same frame being decoded */
    else if(Second_Field && (picture_coding_type==P_TYPE))
    {
      /* our favourite case: the IP field picture pair */
      if((previous_picture_coding_type==I_TYPE)&&(picture_coding_type==P_TYPE))
      {
        substitute_display_framenum = bitstream_framenum;
      }
      else /* our more generic P field picture pair */
      {
        substitute_display_framenum =
          (temporal_reference - previous_anchor_temporal_reference)
            + bitstream_framenum - 1;
      }

      Read_Frame(Substitute_Picture_Filename, current_frame, substitute_display_framenum);
    }
#ifdef DEBUG
    else if((picture_coding_type!=B_TYPE)||(picture_coding_type!=D_TYPE))
    {
      Print("NO SUBS FOR THIS PICTURE\n");
    }
#endif
  }


  /* set b gate so we don't redundantly load next time around */
  if(picture_coding_type==B_TYPE)
    bgate = 1;
  else
    bgate = 0;

  /* update general tracking variables */
  if((picture_structure==FRAME_PICTURE)||(!Second_Field))
  {
    previous_temporal_reference  = temporal_reference;
    previous_bitstream_framenum  = bitstream_framenum;
  }

  /* update reference frame tracking variables */
  if((picture_coding_type!=B_TYPE) &&
    ((picture_structure==FRAME_PICTURE)||Second_Field))
  {
    previous_anchor_temporal_reference  = temporal_reference;
    previous_anchor_bitstream_framenum  = bitstream_framenum;
  }

  previous_picture_coding_type = picture_coding_type;

}


/* Note: fields are only Read to serve as the same-frame reference for
   a second field */
static void Read_Frame(fname,frame,framenum)
char *fname;
unsigned char *frame[];
int framenum;
{
  int parity;
  int rerr = 0;
  int field_mode;

  if(framenum<0)
    Print("ERROR: framenum (%d) is less than zero\n", framenum);


  if(Big_Picture_Flag)
    rerr = Extract_Components(fname, substitute_frame, framenum);
  else
    rerr = Read_Components(fname, substitute_frame, framenum);

  if(rerr!=0)
  {
    Print("was unable to substitute frame\n");
  }

  /* now copy to the appropriate buffer */
  /* first field (which we are attempting to substitute) must be
     of opposite field parity to the current one */
  if((Second_Field)&&(picture_coding_type==P_TYPE))
  {
    parity      = (picture_structure==TOP_FIELD ? 1:0);
    field_mode  = (picture_structure==FRAME_PICTURE ? 0:1);
  }
  else
  {
    /* Like frame structued pictures, B pictures only substitute an entire frame
       since both fields always predict from the same frame (with respect
       to forward/backwards directions) */
    parity = 0;
    field_mode = 0;
  }


  Copy_Frame(substitute_frame[0], frame[0], Coded_Picture_Width,
    Coded_Picture_Height, parity, field_mode);

  Copy_Frame(substitute_frame[1], frame[1], Chroma_Width, Chroma_Height,
    parity, field_mode);

  Copy_Frame(substitute_frame[2], frame[2], Chroma_Width, Chroma_Height,
    parity, field_mode);

#ifdef VERBOSE
  if(Verbose_Flag > NO_LAYER)
    Print("substituted %s %d\n",
      (field_mode ? (parity?"bottom field":"bottom field"):"frame"), framenum);
#endif
}




static int Read_Components(filename, frame, framenum)
char *filename;
unsigned char *frame[3];
int framenum;
{
  int err = 0;
  char outname[FILENAME_LENGTH];
  char name[FILENAME_LENGTH];

  StringPrint(outname,sizeof(outname), filename,framenum);


  StringPrint(name, sizeof(name), "%s.Y",outname);
  err += Read_Component(name, frame[0], Coded_Picture_Width,
    Coded_Picture_Height);

  StringPrint(name,sizeof(name), "%s.U",outname);
  err += Read_Component(name, frame[1], Chroma_Width, Chroma_Height);

  StringPrint(name,sizeof(name), "%s.V",outname);
  err += Read_Component(name, frame[2], Chroma_Width, Chroma_Height);

  return(err);
}


static int Read_Component(Filename, Frame, Width, Height)
char *Filename;
unsigned char *Frame;
int Width;
int Height;
{
  int Size;
  int Bytes_Read;
  int Infile;

  Size = Width*Height;

#ifdef DEBUG
  Print("SUBS: reading %s\n", Filename);
#endif

  if(!(Infile=Open(Filename,O_RDONLY|O_BINARY))<0)
  {
    Print("ERROR: unable to Open reference filename (%s)\n", Filename);
	return(-1);
  }

  Bytes_Read = Read(Infile, Frame, Size);

  if(Bytes_Read!=Size)
  {
    Print("was able to Read only %d bytes of %d of file %s\n",
      Bytes_Read, Size, Filename);
  }

  Close(Infile);
  return(0);
}


/* optimization: do not Open the big file each time. Open once at start
 of decoder, and Close at the very last frame */

/* Note: "big" files were used in E-mail exchanges almost exclusively by the
 MPEG Committee's syntax validation and conformance ad-hoc groups from
 the year 1993 until 1995 */
static int Extract_Components(filename, frame, framenum)
char *filename;
unsigned char *frame[3];
int framenum;
{
    /*  int err = 0; */
    int fd;
    int line;
    int size, offset;


    if((fd = open(filename, O_RDONLY)) < 0)
    {
        StringPrint(Error_Text,sizeof(Error_Text),"Couldn't Open %s\n",filename);
        return(-1);
    }

    /* compute size of each frame (in bytes) */
    size = (Coded_Picture_Width*Coded_Picture_Height);

    if(chroma_format==CHROMA444)
        size = (size * 3);
    else if(chroma_format==CHROMA422)
        size = (size * 2);
    else if(chroma_format==CHROMA420)
        size = ((size*3)>>1);
    else
        Print("ERROR: chroma_format (%d) not recognized\n", chroma_format);


    /* compute distance into "big" file */
    offset = size*framenum;

#ifdef DEBUG
    Print("EXTRACTING: frame(%d) offset(%d), size (%d) from %s\n",
          framenum, offset, size, filename);
#endif

    /* seek to location in big file where desired frame begins */
    /* note: this offset cannot exceed a few billion bytes due to the */
    /*       obvious limitations of 32-bit integers */
    lseek(fd, offset, 0);

    /* Y  */
    for (line=0; line<Coded_Picture_Height; line++)
    {
        read(fd, frame[0]+(line*Coded_Picture_Width), Coded_Picture_Width);
    }

    /* Cb */
    for (line=0; line<Chroma_Height; line++)
    {
        read(fd, frame[1]+(line*Chroma_Width), Chroma_Width);
    }

    /* Cr */
    for (line=0; line<Chroma_Height; line++)
    {
        read(fd, frame[2]+(line*Chroma_Width), Chroma_Width);
    }


    close(fd);
    return(0);
}


static void Copy_Frame(src, dst, width, height, parity, field_mode)
unsigned char *src;
unsigned char *dst;
int width;
int height;
int parity;        /* field parity (top or bottom) to overwrite */
int field_mode;    /* 0 = frame, 1 = field                      */
{
  int row, col;
  int s, d;
  int incr;

  s = d = 0;

#ifdef DEBUG
  Print("COPYING (w=%d, h=%d, parity=%d, field_mode=%d)\n",
    width,height,parity,field_mode);
#endif /* DEBUG */

  if(field_mode)
  {
    incr = 2;

    if(parity==0)
      s += width;
  }
  else
  {
    incr = 1;
  }

  for(row=0; row<height; row+=incr)
  {
    for(col=0; col<width; col++)
    {
      dst[d+col] = src[s+col];
    }

    d += (width*incr);
    s += (width*incr);
  }

}

