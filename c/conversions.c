/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/conversions.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: conversions.c,v $
 * Revision 1.0  1997/05/06 20:30:22  schenney
 * Initial revision
 *
 */
#define PATCHLEVEL 0
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
**  conversions.c: Functions to convert between various vector spaces.
**
**  Created: 06/03/94
**
**  External Functions:
**
**  void
**  Convert_World_To_View(Vector *world_verts, Vertex *view_verts,
**                          short num_verts, Viewport *vp)
**  Takes points from world to view.
**
**  void
**  Convert_View_To_Screen(Vertex *verts, short num_verts, Viewport *vp,
**                          short width, short height, int magnification);
**  Takes points from view to screen.
**
**  void
**  Convert_Plane_World_To_View(ConstraintPtr world_plane, Viewport *vp,
**                              ConstraintPtr res)
**  Converts a plane from world into view.
**
**  void
**  Convert_Line_World_To_View(ConstraintPtr world_line, Viewport *vp,
**                             ConstraintPtr res)
**  Converts a line from world into view.  Used by the placement routines.
*/

#include <math.h>
#include <sced.h>


/*
**  void
**  Convert_World_To_View(Vector *world_verts, Vertex *view_verts,
**                          short num_verts, Viewport *vp)
**  Takes an array of vertices from world to view.
**  world_verts: the vertices in world.
**  view_verts: the vertices in view.
**  num_verts: the number of vertices to convert.
**  vp: the viewport to use for the transformation.
*/
void
Convert_World_To_View(VectorPtr world_verts, Vertex *view_verts, int num_verts,
                      Viewport *vp)
{
    register int        i;
    register Vector temp_v;

    for ( i = 0 ; i < num_verts; i++ )
    {
        MVMul(vp->world_to_view.matrix, world_verts[i], temp_v);
        VAdd(vp->world_to_view.displacement, temp_v, view_verts[i].view);
    }
}



/*
**  void
**  Convert_View_To_Screen(Vertex *verts, short num_verts, Viewport *vp,
**                          short width, short height, int magnification);
**  Takes an array of vertices from view to screen.
**  verts: the vertices to convert.
**  num_verts: the number of vertices to convert.
**  vp: the viewport to use for the transformation.
**  width, height: size of the screen.
**  magnification: the magnification factor for the window.
*/
void
Convert_View_To_Screen(Vertex *verts, int num_verts, Viewport *vp,
                        short width, short height, double magnification)
{
    register int    i;
    register double mid_x = width / 2;
    register double mid_y = height / 2;
    register double temp_d;
    register double sx, sy;

    for ( i =  0 ; i < num_verts ; i++ )
    {
        if ( vp->eye_distance + verts[i].view.z != 0 )
        {
            temp_d = fabs(vp->eye_distance /
                        (vp->eye_distance + verts[i].view.z));
            temp_d *= magnification;

            sx = mid_x + verts[i].view.x * temp_d;
            sy = mid_y - verts[i].view.y * temp_d;

            if ( sx > (double)MAX_SIGNED_SHORT )
                verts[i].screen.x = MAX_SIGNED_SHORT;
            else if ( sx < -(double)MAX_SIGNED_SHORT )
                verts[i].screen.x = -MAX_SIGNED_SHORT;
            else
                verts[i].screen.x = (short)sx;

            if ( sy > (double)MAX_SIGNED_SHORT )
                verts[i].screen.y = MAX_SIGNED_SHORT;
            else if ( sy < -(double)MAX_SIGNED_SHORT )
                verts[i].screen.y = -MAX_SIGNED_SHORT;
            else
                verts[i].screen.y = (short)sy;
        }
        else
        {
            verts[i].screen.x = 0;
            verts[i].screen.y = 0;
        }
    }
}



/*  void
**  Convert_Plane_World_To_View(ConstraintPtr world_plane, Viewport *vp,
**                              ConstraintPtr res)
**  Converts a plane from world into view.  Used by the placement routines.
*/
void
Convert_Plane_World_To_View(ConstraintPtr world_plane, Viewport *vp,
                            ConstraintPtr res)
{
    Matrix      transp;
    Vector      temp_v;

    /* Convert the point.*/
    MVMul(vp->world_to_view.matrix, world_plane->c_point, temp_v);
    VAdd(vp->world_to_view.displacement, temp_v, res->c_point);

    /* Convert the normal. */
    MTrans(vp->view_to_world.matrix, transp);
    MVMul(transp, world_plane->c_vector, res->c_vector);
}


/*  void
**  Convert_Line_World_To_View(ConstraintPtr world_line, Viewport *vp,
**                              ConstraintPtr res)
**  Converts a line from world into view.  Used by the placement routines.
*/
void
Convert_Line_World_To_View(ConstraintPtr world_line, Viewport *vp,
                           ConstraintPtr res)
{
    Vector  temp_v;

    /* Convert the direction.*/
    MVMul(vp->world_to_view.matrix, world_line->c_vector, res->c_vector);

    /* Convert the point.*/
    MVMul(vp->world_to_view.matrix, world_line->c_point, temp_v);
    VAdd(vp->world_to_view.displacement, temp_v, res->c_point);
}

