#ifndef __PROTO_H
#define __PROTO_H

/* From: util.c */
void die(char *, ...);
void warn(char *, ...);
void msg(char *, ...);
void debugSetup(char *);
void debugOptions();

/* From: audioIO_<OSTYPE>.c */
void audioOpen(int frequency, int stereo, int volume);
void audioSetVolume(int);
void audioFlush();
void audioClose();
int  audioWrite(char *, int);
int  getAudioFd();
void audioBufferOn(int);


/* From: buffer.c */
void printout(void);
int  audioBufferOpen(int, int, int);
void audioBufferClose();
void audioBufferWrite(char *, int);
void audioBufferFlush();

/* From: audio.c */
void displayUsage();

#endif // __PROTO_H

