#ifndef SS_SAVE_H
#define SS_SAVE_H

#include "ss_base.h"

#define DEFAULT_HISCORE 20000

/* High score from SRAM, or DEFAULT_HISCORE if the save is missing or corrupt. */
int save_load_hiscore(void);
void save_store_hiscore(int hiscore);

#endif
