// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//
//-----------------------------------------------------------------------------

#ifdef __GNUG__
#pragma implementation "i_net.h"
#endif

#include "d_net.h"
#include <stdio.h>
#include <stdlib.h>

#include "doomstat.h"
#include "i_system.h"
#include "d_event.h"
#include "d_net.h"
#include "m_argv.h"

#include "doomstat.h"


//
// I_InitNetwork
//
// Hardcoded to singleplayer
void I_InitNetwork (void)
{
    boolean		trueval = true;
    int			i;
    int			p;
    struct hostent*	hostentry;	// host information entry

    doomcom = malloc (sizeof (*doomcom) );
    memset (doomcom, 0, sizeof(*doomcom) );

    doomcom-> ticdup = 1;

    doomcom-> extratics = 0;

    // single player game
    netgame = false;
    doomcom->id = DOOMCOM_ID;
    doomcom->numplayers = doomcom->numnodes = 1;
    doomcom->deathmatch = false;
    doomcom->consoleplayer = 0;
    return;
}


void I_NetCmd (void)
{

}

