/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/h/RCS/instance_list.h,v 1.0 1997/05/06 20:42:15 schenney Exp $
 *
 * $Log: instance_list.h,v $
 * Revision 1.0  1997/05/06 20:42:15  schenney
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
**  instance_list.h : header for instance list manipulation functions.
**
*/

#ifndef __INSTANCE_LIST__
#define __INSTANCE_LIST__

extern void         Insert_Element(InstanceList*, ObjectInstancePtr);
extern void         Insert_Instance_List(InstanceList*, InstanceList*);
extern InstanceList Append_Element(InstanceList*, ObjectInstancePtr);
extern void         Append_Instance_List(InstanceList*, InstanceList);
extern InstanceList Delete_Element(InstanceList);
extern InstanceList Find_Object_In_Instances(ObjectInstancePtr, InstanceList);
extern InstanceList Merge_Selection_Lists(InstanceList, InstanceList);
extern InstanceList Remove_Selection_List(InstanceList, InstanceList);
extern ObjectInstancePtr    Delete_Instance(InstanceList);
extern void         Free_Selection_List(InstanceList);
extern int          Instance_List_Length(InstanceList);


#endif

