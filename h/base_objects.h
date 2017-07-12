/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/h/RCS/base_objects.h,v 1.0 1997/05/06 20:40:54 schenney Exp $
 *
 * $Log: base_objects.h,v $
 * Revision 1.0  1997/05/06 20:40:54  schenney
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
**  base_objects.h : Header file for base object handling.
**
**  Created: 13/03/94
**
*/

#ifndef __BASE_OBJ_HEADER__
#define __BASE_OBJ_HEADER__

/*
**   Typedefs.
*/
/*  A type for the list of base objects. */
typedef BaseObjectPtr *BaseObjectList;


/*
**  extern Variables
*/
extern BaseObjectList   base_objects;
extern int              num_base_objects;
extern int              num_wire_base_objects;
extern int              num_csg_base_objects;
extern int              num_agg_base_objects;

/* Functions from base_select.c */
extern void Base_Select_Popup(GenericObject, int);
extern void Base_Add_Select_Option(BaseObjectPtr);
extern void Base_Select_Destroy_Widget(GenericObject, int);

/* From base_objects.c */
extern void Base_Export(Widget, BaseObjectPtr);

/*
**  Functions from light.c, only used by base_objects.
*/
extern void Light_Create_Point_Light(ObjectInstancePtr);
extern void Light_Create_Area_Light(ObjectInstancePtr);
extern void Light_Create_Dir_Light(ObjectInstancePtr);
extern void Light_Create_Spot_Light(ObjectInstancePtr);
extern void Light_Destroy_Object(ObjectInstancePtr);

#endif

