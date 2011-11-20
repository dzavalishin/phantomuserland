#include "amp.h"



struct AUDIO_HEADER {
    int ID;
    int layer;
    int protection_bit;
    int bitrate_index;
    int sampling_frequency;
    int padding_bit;
    int private_bit;
    int mode;
    int mode_extension;
    int copyright;
    int original;
    int emphasis;
};

struct SIDE_INFO {
    int main_data_begin;
    int scfsi[2][4];
    int part2_3_length[2][2];
    int big_values[2][2];
    int global_gain[2][2];
    int scalefac_compress[2][2];
    int window_switching_flag[2][2];
    int block_type[2][2];
    int mixed_block_flag[2][2];
    int table_select[2][2][3];
    int subblock_gain[2][2][3];
    int region0_count[2][2];
    int region1_count[2][2];
    int preflag[2][2];
    int scalefac_scale[2][2];
    int count1table_select[2][2];
};

extern int in_fd,out_fd;
extern void statusDisplay(struct AUDIO_HEADER *header, int frameNo);
extern int decodeMPEG(void);
extern void initialise_globals(void);
extern void report_header_error(int err);
extern int scalefac_l[2][2][22];
extern int scalefac_s[2][2][13][3];
extern int t_b8_l[2][3][22];
extern int t_b8_s[2][3][13];
extern short t_bitrate[2][3][15];
extern int is[2][578];
extern float xr[2][32][18];
extern int *t_l,*t_s;
extern int nch;
extern int t_sampling_frequency[2][3];
extern int A_QUIET,A_SHOW_CNT,A_FORMAT_WAVE,A_DUMP_BINARY;
extern int A_WRITE_TO_AUDIO,A_WRITE_TO_FILE;
extern short pcm_sample[64];
extern int A_AUDIO_PLAY;
extern int A_SET_VOLUME,A_SHOW_TIME;
extern int A_MSG_STDOUT;
extern int A_DOWNMIX;

/* GUI CONTROL STUFF */
extern int GUI_PLAY;
extern int GUI_PLAYING;
extern int GUI_PAUSE;
extern int GUI_PAUSED;
extern int GUI_STOP;
extern int GUI_STOPPED;
extern int GUI_FD_TO_PLAY;
extern int GUI_NEXT_FILE_READY;

/* GUI control stuff */
extern int send_fd;
extern int receive_fd;

extern int stop_flag;
extern int quit_flag;

#ifdef AUDIO

int in_fd,out_fd;
 
int scalefac_l[2][2][22];
int scalefac_s[2][2][13][3];

int is[2][578];
float xr[2][32][18];

int *t_l,*t_s;
int nch;
int t_sampling_frequency[2][3] = {
{ 22050 , 24000 , 16000},
{ 44100 , 48000 , 32000}
};

/* GUI control stuff */
int send_fd;
int receive_fd;

int stop_flag;
int quit_flag;

int GUI_PLAY,GUI_PLAYING,GUI_STOP,GUI_STOPPED,GUI_PAUSE,GUI_PAUSED;
int GUI_FD_TO_PLAY,GUI_NEXT_FILE_READY;

int A_QUIET,A_SHOW_CNT,A_FORMAT_WAVE,A_DUMP_BINARY;
int A_WRITE_TO_FILE;
int A_AUDIO_PLAY;
int A_SET_VOLUME, A_SHOW_TIME;
int A_MSG_STDOUT;
int A_DOWNMIX;

short pcm_sample[64];

short t_bitrate[2][3][15] = {{
{0,32,48,56,64,80,96,112,128,144,160,176,192,224,256},
{0,8,16,24,32,40,48,56,64,80,96,112,128,144,160},
{0,8,16,24,32,40,48,56,64,80,96,112,128,144,160}
},{
{0,32,64,96,128,160,192,224,256,288,320,352,384,416,448},
{0,32,48,56,64,80,96,112,128,160,192,224,256,320,384},
{0,32,40,48,56,64,80,96,112,128,160,192,224,256,320}
}};

/* the last sfb is given implicitly on pg.28. of the standard. scalefactors 
 * for that one are 0, pretab also 
 */
/* leftmost index denotes ID, so first three tables are for MPEG2 (header->ID==0)
 * and the other three are for MPEG1 (header->ID==1)
 */
/* 22.05, 24, 16 */
int t_b8_l[2][3][22]={{ /* table B.8b ISO/IEC 11172-3 */
{5,11,17,23,29,35,43,53,65,79,95,115,139,167,199,237,283,335,395,463,521,575},
{5,11,17,23,29,35,43,53,65,79,95,113,135,161,193,231,277,331,393,463,539,575},
{5,11,17,23,29,35,43,53,65,79,95,115,139,167,199,237,283,335,395,463,521,575}
},{
{3,7,11,15,19,23,29,35,43,51,61,73,89,109,133,161,195,237,287,341,417,575},
{3,7,11,15,19,23,29,35,41,49,59,71,87,105,127,155,189,229,275,329,383,575},
{3,7,11,15,19,23,29,35,43,53,65,81,101,125,155,193,239,295,363,447,549,575}
}};   
int t_b8_s[2][3][13]={{ /* table B.8b ISO/IEC 11172-3 */
{3,7,11,17,23,31,41,55,73,99,131,173,191},
{3,7,11,17,25,35,47,61,79,103,135,179,191},
{3,7,11,17,25,35,47,61,79,103,133,173,191}
},{
{3,7,11,15,21,29,39,51,65,83,105,135,191},
{3,7,11,15,21,27,37,49,63,79,99,125,191},
{3,7,11,15,21,29,41,57,77,103,137,179,191}
}};

int  args(int argc,char **argv);
void initialise_decoder(void);
int decodeMPEG(void);
void initialise_globals(void);
void report_header_error(int err);
int  setup_audio(struct AUDIO_HEADER *header);
void close_audio(void);
int ready_audio(void);

void play(char *inFileStr, char *outFileStr);

#endif /* AUDIO */
