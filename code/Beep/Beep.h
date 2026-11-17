#ifndef __BEEP_H__
#define __BEEP_H__

#include "zf_common_headfile.h"

#define Beep P33_10

#define SHORT_RING_TIME 3

void Beep_Start(void);
void Beep_Init(void);
void Beep_Stop(void);
void Beep_ShortRing(void);

#endif
