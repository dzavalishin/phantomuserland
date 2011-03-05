/* this file is a part of amp software, (C) tomislav uzelac 1996,1997
*/
/* dump.h
 * 
 * Last modified by: tomislav uzelac May 31 1997
 */

extern void dump(int *length);
extern void show_header(struct AUDIO_HEADER *header);

#ifdef DUMP
void dump(int *length);
void show_header(struct AUDIO_HEADER *header);
/*
static char *t_modes[] = {
        "stereo","joint_stereo","dual_channel","single_channel"};
*/
#endif /* DUMP */
