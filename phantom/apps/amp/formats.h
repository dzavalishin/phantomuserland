/* this file is a part of amp software, (C) tomislav uzelac 1996,1997
*/
 
/* formats.h
 *
 * Created by: tomislav uzelac Dec 22 1996
 */
extern void wav_end(struct AUDIO_HEADER *header);
extern void wav_begin(void);

#ifdef FORMATS
void wav_end(struct AUDIO_HEADER *header);
void wav_begin(void);
#endif /* FORMATS */
