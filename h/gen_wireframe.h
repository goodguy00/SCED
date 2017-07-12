/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/h/RCS/gen_wireframe.h,v 1.0 1997/05/06 20:42:04 schenney Exp $
 *
 * $Log: gen_wireframe.h,v $
 * Revision 1.0  1997/05/06 20:42:04  schenney
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
**  gen_wireframe.h : header file for functions from gen_wireframe.c.
**
**  Created: 13/03/94
**
**  Each returns a pointer to a new wireframe structure for that object.
**  NULL is returned on failure.
*/

#ifndef __GEN_WIRE__
#define __GEN_WIRE__

extern WireframePtr Generic_Sphere_Wireframe();
extern WireframePtr Generic_Cylinder_Wireframe();
extern WireframePtr Generic_Cone_Wireframe();
extern WireframePtr Generic_Cube_Wireframe();
extern WireframePtr Generic_Triangle_Wireframe();
extern WireframePtr Generic_Bezier_Wireframe();
extern WireframePtr Generic_Square_Wireframe();
extern WireframePtr Generic_Plane_Wireframe();
extern WireframePtr Generic_Light_Wireframe();
extern WireframePtr Generic_Spot_Light_Wireframe();
extern WireframePtr Generic_Torus_Wireframe();
extern WireframePtr Generic_Camera_Wireframe();
extern WireframePtr Generic_Direction_Wireframe();

extern WireframePtr Dense_Sphere_Wireframe(int, WireframePtr);
extern WireframePtr Dense_Cylinder_Wireframe(int);
extern WireframePtr Dense_Cone_Wireframe(int);
extern WireframePtr Dense_Spot_Wireframe(int);
extern WireframePtr Dense_Torus_Wireframe(int);
extern WireframePtr Dense_Bezier_Wireframe(int);
extern WireframePtr Dense_CSG_Wireframe(int, BaseObjectPtr, Boolean);

#endif

