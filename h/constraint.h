/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/h/RCS/constraint.h,v 1.0 1997/05/06 20:41:05 schenney Exp $
 *
 * $Log: constraint.h,v $
 * Revision 1.0  1997/05/06 20:41:05  schenney
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
**  constraint.h : header file for constraint management functions.
*/




/* Functions from constraint.c. */
extern void     Constraint_Solve_System(ObjectInstancePtr, ConstraintPtr, int,
                                        ResultantPtr);
extern void     Constraint_Solve_System_Ptrs(ConstraintPtr*, int, ResultantPtr);
extern int      Point_Satisfies_Constraint(VectorPtr, ResultantPtr,
                                           ConstraintPtr);
extern double   Distance_Point_To_Plane(VectorPtr, VectorPtr, VectorPtr);
extern Vector   Find_Required_Motion(VectorPtr, ResultantPtr, Boolean, int *);

extern Vector   Closest_Sphere_Point(VectorPtr, VectorPtr, VectorPtr,
                                     double*, double*);
