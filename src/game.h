
/*
 * Copyright (C) an0nym0us
 *
 * This software is distributed under the terms of the GNU General Public
 * License ("GPL") version 3, as published by the Free Software Foundation.
 */

#ifndef __GAME_H__
#define __GAME_H__

#include <sysutil/game.h>

#include "common.h"
#include "tools.h"

/* used to pass data between functions */
typedef struct _game_data
{
  int *exitapp;

  u32 type;
  u32 attrib;
  sysGameContentSize size;
  char *dir;
  char *content;
  char *usrdir;
} gameData;


void gameInitialization();
void gameGetPath(gameData *gdata);
void gameFinish();

#endif /* __GAME_H__ */
