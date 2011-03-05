/* this file is a part of amp software, (C) tomislav uzelac 1996,1997
*/

extern int ffwd(struct AUDIO_HEADER *header, int nframes);
extern int rew(struct AUDIO_HEADER *header, int nframes);

#ifdef POSITION
int ffwd(struct AUDIO_HEADER *header, int nframes);
int rew(struct AUDIO_HEADER *header, int nframes);
#endif
