/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/h/RCS/edge_table.h,v 1.0 1997/05/06 20:41:25 schenney Exp $
 *
 * $Log: edge_table.h,v $
 * Revision 1.0  1997/05/06 20:41:25  schenney
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

/*
**  edge_table.h: header for functions for manipulating edge tables.
*/

#ifndef __EDGE_TABLE__
#define __EDGE_TABLE__

extern EdgeTable    Edge_Table_Create(int);
extern EdgePtr      Edge_Table_Add(EdgeTable, int, int);
extern EdgePtr      Edge_Table_Get(EdgeTable, int, int);
extern void         Edge_Table_Free(EdgeTable, int);

extern void         Edge_Table_Build(WireframePtr);
extern EdgeTable    Edge_Table_Copy(EdgeTable, int);
extern int          Edge_Table_Count(EdgeTable, int);

#endif
