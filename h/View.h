/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/h/RCS/View.h,v 1.0 1997/05/06 20:40:35 schenney Exp $
 *
 * $Log: View.h,v $
 * Revision 1.0  1997/05/06 20:40:35  schenney
 * Initial revision
 *
 */
/*
**    ScEd: A Constraint Based Scene Editor.
**    Copyright (C) 1994-1998  Stephen Chenney (schenney@cs.berkeley.edu)
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program; if not, write to the Free Software
**    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#define PATCHLEVEL 0

/*
**  sced: A Constraint Based Object Scene Editor
**
**  View.h : Public header file for a simple widget used to draw in.
**
**  Created: 26/03/94
*/


#ifndef _View_h
#define _View_h

/****************************************************************
 *
 * View widget
 *
 ****************************************************************/

/* Resources:

 Name               Class               RepType     Default Value
 ----               -----               -------     -------------
 background         Background          Pixel       XtDefaultBackground
 border             BorderColor         Pixel       XtDefaultForeground
 borderWidth        BorderWidth         Dimension   1
 destroyCallback    Callback            Pointer     NULL
 height             Height              Dimension   0
 mappedWhenManaged  MappedWhenManaged   Boolean     True
 sensitive          Sensitive           Boolean     True
 width              Width               Dimension   0
 x                  Position            Position    0
 y                  Position            Position    0
 exposeCallback     Callback            Pointer     NULL
 desiredWidth       Width               Dimension   0
 desiredHeight      Height              Dimension   0
 maintainSize       Boolean             Boolean     FALSE
 foreground         Foreground          Pixel       XtDefaultForeground
 background         Background          Pixel       XtDefaultBackground


*/

/* define any special resource names here that are not in <X11/StringDefs.h> */

#define XtNexposeCallback "exposeCallback"
#define XtNdesiredWidth "desiredWidth"
#define XtNdesiredHeight "desiredHeight"
#define XtNmaintainSize "maintainSize"

#define XtCPointer "Pointer"

/* declare specific ViewWidget class and instance datatypes */

typedef struct _ViewClassRec*   ViewWidgetClass;
typedef struct _ViewRec*        ViewWidget;

/* declare the class constant */

extern WidgetClass viewWidgetClass;

#endif /* _View_h */
