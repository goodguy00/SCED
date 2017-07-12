/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/h/RCS/torus.h,v 1.0 1997/05/06 20:42:45 schenney Exp $
 *
 * $Log: torus.h,v $
 * Revision 1.0  1997/05/06 20:42:45  schenney
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
**  torus.h: header for torus functions.
*/

extern void Torus_Create_Function(ObjectInstancePtr);
extern void Torus_Destroy_Function(ObjectInstancePtr);
extern void Torus_Copy_Hook(ObjectInstancePtr, ObjectInstancePtr);
extern void Torus_Calculate_Vertices(VectorPtr, int, double);
extern Vector   Torus_Transform_Vertex(ObjectInstancePtr, VectorPtr);
extern void Torus_Calculate_Vertex_Normals(ObjectInstancePtr, Vector*, int,
                                           Boolean);

