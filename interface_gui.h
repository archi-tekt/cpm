/* #############################################################################
 * header information for interface_gui.c
 * #############################################################################
 * Copyright (C) 2005-2009 Harry Brueckner
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Contact: Harry Brueckner <harry_b@mm.st>
 *          Muenchener Strasse 12a
 *          85253 Kleinberghofen
 *          Germany
 * #############################################################################
 */
#ifndef CPM_INTERFACE_GUI_H
#define CPM_INTERFACE_GUI_H


/* #############################################################################
 * includes
 */
#include <cdk/cdk.h>


/* #############################################################################
 * prototypes
 */
#define destroyScreen(message) destroyScreen_(__LINE__, message)
void destroyScreen_(int line, char* message);
void userInterface(void);


/* #############################################################################
 * definition of the interface layout
 */
#define SHOW_BOX        TRUE
#define SHOW_SHADOW     FALSE


/* new versions of CDK define this macro, old ones don't have it so we define
 * it empty to get things working.
 */
#ifdef FORCE_CDK_V4
    #define cpmObjOf(obj) obj
#else
  #ifdef ObjOf
    #define cpmObjOf(obj) ObjOf(obj)
  #else
    #define cpmObjOf(obj) obj
  #endif
#endif


#endif


/* #############################################################################
 */

