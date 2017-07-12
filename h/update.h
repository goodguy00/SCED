/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/h/RCS/update.h,v 1.0 1997/05/06 20:42:55 schenney Exp $
 *
 * $Log: update.h,v $
 * Revision 1.0  1997/05/06 20:42:55  schenney
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
**  sced: A Constraint Based Object Scene Editor
**
**  update.h : header for update constraint functions.
*/

#ifndef __SCED_UPDATE__
#define __SCED_UPDATE__

extern Boolean  Edit_Update_Constraint_Specs(ConstraintPtr, VectorPtr,
                                             FeaturePtr, VectorPtr);
extern Boolean  Edit_Update_Object_Constraints(ConstraintPtr, int, EditInfoPtr);
extern Boolean  Edit_Update_Active_Object_Cons(ConstraintPtr, int, EditInfoPtr,
                                               VectorPtr);
extern Boolean  Edit_Update_Constraints(EditInfoPtr);


#endif /* __SCED_UPDATE__ */
