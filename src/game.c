/*
 * game.c
 *
 * Copyright (C) an0nym0us
 *
 * This software is distributed under the terms of the GNU General Public
 * License ("GPL") version 3, as published by the Free Software Foundation.
 *
 */

#include <stdio.h>
#include <string.h>

#include <ppu-lv2.h>
#include <ppu-types.h>

#include <sysutil/game.h>

#include "common.h"
#include "debug.h"
#include "game.h"


void
gameInitialization ( gameData *gdata )
{
  dbgprintf ( "initializing" ) ;

  gdata->dir = ( char* ) malloc ( 32 + 1 ) ;
  gdata->content = ( char* ) malloc ( 128 + 1 ) ;
  gdata->usrdir = ( char* ) malloc ( 128 + 1 ) ;

  memset ( &gdata->size, 0x00, ( size_t ) sizeof( sysGameContentSize ) ) ;

  dbgprintf ( "initialized" ) ;
}

void
gameFinish ( gameData *gdata )
{
  dbgprintf ( "finishing" ) ;

  free ( gdata->dir ) ;
  free ( gdata->content ) ;
  free ( gdata->usrdir ) ;

  dbgprintf ( "finished" ) ;
}

void
gameGetPath ( gameData *gdata )
{
  static s32 ret = 0 ;

  if ( ( gdata->dir == NULL ) || ( gdata->content == NULL ) || ( gdata->usrdir == NULL ) )
  {
    errprintf ( "dir: %p, content: %p, usrdir: %p", gdata->dir, gdata->content, gdata->usrdir ) ;
    return ;
  }

  argprintf ( "sysGameBootCheck ( %p, %p, %p, %p )", &gdata->type, &gdata->attrib, &gdata->size, gdata->dir ) ;

  if ( ( ret = sysGameBootCheck ( &gdata->type, &gdata->attrib, &gdata->size, gdata->dir ) ) != 0 )
  {
    argprintf ( "sysGameBootCheck returned: 0x%x", ret ) ;
  }

  argprintf ( "type: 0x%x", gdata->type ) ;
  argprintf ( "attrib: 0x%x", gdata->attrib ) ;
  argprintf ( "size: 0x%x", gdata->size.sizeKB ) ;
  argprintf ( "dir: %s", gdata->dir ) ;

  argprintf ( "sysGameContentPermit ( %p, %p )", gdata->content, gdata->usrdir ) ;

  if ( ( ret = sysGameContentPermit ( gdata->content, gdata->usrdir ) ) != 0 )
  {
    argprintf ( "sysGameContentPermit returned: 0x%x", ret ) ;
  }

  argprintf ( "content: %s", gdata->content ) ;
  argprintf ( "usrdir: %s", gdata->usrdir ) ;

}
