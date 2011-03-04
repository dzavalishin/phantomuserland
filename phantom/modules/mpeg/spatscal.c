
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>

#include "config.h"
#include "global.h"

/* private prototypes */
static void Read_Lower_Layer_Component_Framewise _ANSI_ARGS_((int comp, int lw, int lh));
static void Read_Lower_Layer_Component_Fieldwise _ANSI_ARGS_((int comp, int lw, int lh));
static void Make_Spatial_Prediction_Frame _ANSI_ARGS_((int progressive_frame,
  int llprogressive_frame, unsigned char *fld0, unsigned char *fld1,
  short *tmp, unsigned char *dst, int llx0, int lly0, int llw, int llh,
  int horizontal_size, int vertical_size, int vm, int vn, int hm, int hn,
  int aperture));
static void Deinterlace _ANSI_ARGS_((unsigned char *fld0, unsigned char *fld1,
  int j0, int lx, int ly, int aperture));
static void Subsample_Vertical _ANSI_ARGS_((unsigned char *s, short *d,
  int lx, int lys, int lyd, int m, int n, int j0, int dj));
static void Subsample_Horizontal _ANSI_ARGS_((short *s, unsigned char *d,
  int x0, int lx, int lxs, int lxd, int ly, int m, int n));



/* get reference frame */
void Spatial_Prediction()
{

  if(Frame_Store_Flag)
  {
    Read_Lower_Layer_Component_Framewise(0,lower_layer_prediction_horizontal_size,
      lower_layer_prediction_vertical_size);      /* Y */
    Read_Lower_Layer_Component_Framewise(1,lower_layer_prediction_horizontal_size>>1,
      lower_layer_prediction_vertical_size>>1);   /* Cb ("U") */
    Read_Lower_Layer_Component_Framewise(2,lower_layer_prediction_horizontal_size>>1,
      lower_layer_prediction_vertical_size>>1);   /* Cr ("V") */
  }
  else
  {
    Read_Lower_Layer_Component_Fieldwise(0,lower_layer_prediction_horizontal_size,
      lower_layer_prediction_vertical_size);      /* Y */
    Read_Lower_Layer_Component_Fieldwise(1,lower_layer_prediction_horizontal_size>>1,
      lower_layer_prediction_vertical_size>>1);   /* Cb ("U") */
    Read_Lower_Layer_Component_Fieldwise(2,lower_layer_prediction_horizontal_size>>1,
      lower_layer_prediction_vertical_size>>1);   /* Cr ("V") */
  }


  Make_Spatial_Prediction_Frame  /* Y */
    (progressive_frame,lower_layer_progressive_frame,llframe0[0],llframe1[0],
     lltmp,current_frame[0],lower_layer_horizontal_offset,
     lower_layer_vertical_offset,
     lower_layer_prediction_horizontal_size,
     lower_layer_prediction_vertical_size,
     horizontal_size,vertical_size,vertical_subsampling_factor_m,
     vertical_subsampling_factor_n,horizontal_subsampling_factor_m,
     horizontal_subsampling_factor_n,
     picture_structure!=FRAME_PICTURE); /* this changed from CD to DIS */

  Make_Spatial_Prediction_Frame  /* Cb */
    (progressive_frame,lower_layer_progressive_frame,llframe0[1],llframe1[1],
     lltmp,current_frame[1],lower_layer_horizontal_offset/2,
     lower_layer_vertical_offset/2,
     lower_layer_prediction_horizontal_size>>1,
     lower_layer_prediction_vertical_size>>1,
     horizontal_size>>1,vertical_size>>1,vertical_subsampling_factor_m,
     vertical_subsampling_factor_n,horizontal_subsampling_factor_m,
     horizontal_subsampling_factor_n,1);

  Make_Spatial_Prediction_Frame  /* Cr */
    (progressive_frame,lower_layer_progressive_frame,llframe0[2],llframe1[2],
     lltmp,current_frame[2],lower_layer_horizontal_offset/2,
     lower_layer_vertical_offset/2,
     lower_layer_prediction_horizontal_size>>1,
     lower_layer_prediction_vertical_size>>1,
     horizontal_size>>1,vertical_size>>1,vertical_subsampling_factor_m,
     vertical_subsampling_factor_n,horizontal_subsampling_factor_m,
     horizontal_subsampling_factor_n,1);

}

static void Read_Lower_Layer_Component_Framewise(comp,lw,lh)
int comp;
int lw, lh;
{
    int fd;
    char fname[256], c;
    char ext[3][3] = {".Y",".U",".V"};
    /*  char *ext = {".Y",".U",".V"}; */
    int i,j;

    StringPrint(fname,sizeof(fname),Lower_Layer_Picture_Filename,True_Framenum);
    StringCat(fname,ext[comp]);
#ifdef VERBOSE
    if (Verbose_Flag>1)
        printf("reading %s\n",fname);
#endif //VERBOSE

    fd=open(fname,O_RDONLY);
    if (fd==NULL) ThreadExit(-1);

    for (j=0; j<lh; j++) {
        for (i=0; i<lw; i++)
        {
            read( fd, &c, 1 );
            llframe0[comp][lw*j+i]=c;
        }
        if (! lower_layer_progressive_frame) {
            j++;
            for (i=0; i<lw; i++)
            {
                read( fd, &c, 1 );
                llframe1[comp][lw*j+i]=c;
            }
        }
    }
    close(fd);
}




static void Read_Lower_Layer_Component_Fieldwise(comp,lw,lh)
int comp;
int lw, lh;
{
    int fd;
    char fname[256], c;
    char ext[3][3] = {".Y",".U",".V"};
    /*  char *ext = {".Y",".U",".V"}; */
    int i,j;

    StringPrint(fname,sizeof(fname),Lower_Layer_Picture_Filename,True_Framenum,lower_layer_progressive_frame ? 'f':'a');
    StringCat(fname,ext[comp]);
#ifdef VERBOSE
    if (Verbose_Flag>1)
        printf("reading %s\n",fname);
#endif //VERBOSE
    fd=open(fname,O_RDONLY);
    if (fd==NULL) ThreadExit(-1);
    for (j=0; j<lh; j+=lower_layer_progressive_frame?1:2)
        for (i=0; i<lw; i++)
        {
            read( fd, &c, 1 );
            llframe0[comp][lw*j+i]=c;
        }
    close(fd);

    if (! lower_layer_progressive_frame) {
        StringPrint(fname,sizeof(fname),Lower_Layer_Picture_Filename,True_Framenum,'b');
        StringCat(fname,ext[comp]);
#ifdef VERBOSE
        if (Verbose_Flag>1)
            printf("reading %s\n",fname);
#endif //VERBOSE
        fd=open(fname,O_RDONLY);
        if (fd==NULL) ThreadExit(-1);
        for (j=1; j<lh; j+=2)
            for (i=0; i<lw; i++)
            {
                read( fd, &c, 1 );
                llframe1[comp][lw*j+i]=c;
            }
        close(fd);
    }
}


/* form spatial prediction */
static void Make_Spatial_Prediction_Frame(progressive_frame,
  llprogressive_frame,fld0,fld1,tmp,dst,llx0,lly0,llw,llh,horizontal_size,
  vertical_size,vm,vn,hm,hn,aperture)
int progressive_frame,llprogressive_frame;
unsigned char *fld0,*fld1;
short *tmp;
unsigned char *dst;
int llx0,lly0,llw,llh,horizontal_size,vertical_size,vm,vn,hm,hn,aperture;
{
  int w, h, x0, llw2, llh2;

  llw2 = (llw*hn)/hm;
  llh2 = (llh*vn)/vm;

  if (llprogressive_frame)
  {
    /* progressive -> progressive / interlaced */
    Subsample_Vertical(fld0,tmp,llw,llh,llh2,vm,vn,0,1);
  }
  else if (progressive_frame)
  {
    /* interlaced -> progressive */
    if (lower_layer_deinterlaced_field_select)
    {
      Deinterlace(fld1,fld0,0,llw,llh,aperture);
      Subsample_Vertical(fld1,tmp,llw,llh,llh2,vm,vn,0,1);
    }
    else
    {
      Deinterlace(fld0,fld1,1,llw,llh,aperture);
      Subsample_Vertical(fld0,tmp,llw,llh,llh2,vm,vn,0,1);
    }
  }
  else
  {
    /* interlaced -> interlaced */
    Deinterlace(fld0,fld1,1,llw,llh,aperture);
    Deinterlace(fld1,fld0,0,llw,llh,aperture);
    Subsample_Vertical(fld0,tmp,llw,llh,llh2,vm,vn,0,2);
    Subsample_Vertical(fld1,tmp,llw,llh,llh2,vm,vn,1,2);
  }

    /* vertical limits */
    if (lly0<0)
    {
      tmp-= llw*lly0;
      llh2+= lly0;
      if (llh2<0)
        llh2 = 0;
      h = (vertical_size<llh2) ? vertical_size : llh2;
    }
    else
    {
      dst+= horizontal_size*lly0;
      h= vertical_size - lly0;
      if (h>llh2)
        h = llh2;
    }

    /* horizontal limits */
    if (llx0<0)
    {
      x0 = -llx0;
      llw2+= llx0;
      if (llw2<0)
        llw2 = 0;
      w = (horizontal_size<llw2) ? horizontal_size : llw2;
    }
    else
    {
      dst+= llx0;
      x0 = 0;
      w = horizontal_size - llx0;
      if (w>llw2)
        w = llw2;
    }

  Subsample_Horizontal(tmp,dst,x0,w,llw,horizontal_size,h,hm,hn);
}

/* deinterlace one field (interpolate opposite parity samples)
 *
 * deinterlacing is done in-place: if j0=1, fld0 contains the input field in
 * its even lines and the odd lines are interpolated by this routine
 * if j0=0, the input field is in the odd lines and the even lines are
 * interpolated
 *
 * fld0: field to be deinterlaced
 * fld1: other field (referenced by the two field aperture filter)
 * j0:   0: interpolate even (top) lines, 1: interpolate odd (bottom) lines
 * lx:   width of fld0 and fld1
 * ly:   height of the deinterlaced field (has to be even)
 * aperture: 1: use one field aperture filter (two field otherwise)
 */
static void Deinterlace(fld0,fld1,j0,lx,ly,aperture)
unsigned char *fld0,*fld1;
int j0,lx,ly; /* ly has to be even */
int aperture;
{
  int i,j,v;
  unsigned char *p0, *p0m1, *p0p1, *p1, *p1m2, *p1p2;

  /* deinterlace one field */
  for (j=j0; j<ly; j+=2)
  {
    p0 = fld0+lx*j;
    p0m1 = (j==0)    ? p0+lx : p0-lx;
    p0p1 = (j==ly-1) ? p0-lx : p0+lx;

    if (aperture)
      for (i=0; i<lx; i++)
        p0[i] = (unsigned int)(p0m1[i] + p0p1[i] + 1)>>1;
    else
    {
      p1 = fld1 + lx*j;
      p1m2 = (j<2)     ? p1 : p1-2*lx;
      p1p2 = (j>=ly-2) ? p1 : p1+2*lx;
      for (i=0; i<lx; i++)
      {
        v = 8*(p0m1[i]+p0p1[i]) + 2*p1[i] - p1m2[i] - p1p2[i];
        p0[i] = Clip[(v + ((v>=0) ? 8 : 7))>>4];
      }
    }
  }
}

/* vertical resampling */
static void Subsample_Vertical(s,d,lx,lys,lyd,m,n,j0,dj)
unsigned char *s;
short *d;
int lx, lys, lyd, m, n, j0, dj;
{
  int i, j, c1, c2, jd;
  unsigned char *s1, *s2;
  short *d1;

  for (j=j0; j<lyd; j+=dj)
  {
    d1 = d + lx*j;
    jd = (j*m)/n;
    s1 = s + lx*jd;
    s2 = (jd<lys-1)? s1+lx : s1;
    c2 = (16*((j*m)%n) + (n>>1))/n;
    c1 = 16 - c2;
    for (i=0; i<lx; i++)
      d1[i] = c1*s1[i] + c2*s2[i];
  }
}

/* horizontal resampling */
static void Subsample_Horizontal(s,d,x0,lx,lxs,lxd,ly,m,n)
short *s;
unsigned char *d;
int x0, lx, lxs, lxd, ly, m, n;
{
  int i, i1, j, id, c1, c2, v;
  short *s1, *s2;
  unsigned char *d1;

  for (i1=0; i1<lx; i1++)
  {
    d1 = d + i1;
    i = x0 + i1;
    id = (i*m)/n;
    s1 = s+id;
    s2 = (id<lxs-1) ? s1+1 : s1;
    c2 = (16*((i*m)%n) + (n>>1))/n;
    c1 = 16 - c2;
    for (j=0; j<ly; j++)
    {
      v = c1*(*s1) + c2*(*s2);
      *d1 = (v + ((v>=0) ? 128 : 127))>>8;
      d1+= lxd;
      s1+= lxs;
      s2+= lxs;
    }
  }
}


