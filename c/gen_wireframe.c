/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/gen_wireframe.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: gen_wireframe.c,v $
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
**  gen_wireframe.c: Functions to generate wireframe representations of objects.
**
**  Created: 05/03/93
**
**  External functions:
**
**  Wireframe*
**  Generic_Sphere_Wireframe();
**  Returns a pointer to a NEW wireframe structure for a generic sphere.
**  NULL on failure.
**
**  Wireframe*
**  Generic_Cylinder_Wireframe();
**  Returns a pointer to a NEW wireframe structure for a generic cylinder.
**  NULL on failure.
**
**  Wireframe*
**  Generic_Cone_Wireframe();
**  Returns a pointer to a NEW wireframe structure for a generic cone.
**  NULL on failure.
**
**  Wireframe*
**  Generic_Cube_Wireframe();
**  Returns a pointer to a NEW wireframe structure for a generic cube.
**  NULL on failure.
**
**  Wireframe*
**  Generic_Square_Wireframe();
**  Returns a pointer to a NEW wireframe structure for a generic square.
**  NULL on failure.
**
**  Wireframe*
**  Generic_Plane_Wireframe();
**  Returns a pointer to a NEW wireframe structure for a generic plane.
**  NULL on failure.
**
**  Wireframe*
**  Generic_Light_Wireframe()
**  Returns a pointer to a light wireframe structure.
**
**  Wireframe*
**  Generic_Torus_Wireframe()
**  Returns a pointer to a generic torus wireframe. Tube radius 1.0,
**  mean radius 3.0.
*/


#include <math.h>
#include <sced.h>
#include <gen_wireframe.h>



/*  WireframePtr
**  Generic_Cube_Wireframe();
**  Returns a pointer to a NEW wireframe structure for a generic cube.
*/
WireframePtr
Generic_Cube_Wireframe()
{
    WireframePtr    result = New(Wireframe, 1);
    int i;

    /*  A cube wireframe has 6 faces, 12 edges and 8 vertices.
    **  There are an additional 7 reference points, one in the center and
    **  one for the center of each face.
    */
    result->num_faces = 6;
    result->num_vertices = 15;
    result->num_real_verts = 8;
    result->num_normals = 6;

    /* These are postitioned and numbered as follows:
    **
    ** Vertices:    0 at (1, 1, 1)
    **              1 at (-1, 1, 1)
    **              2 at (-1, -1, 1)
    **              3 at (1, -1, 1)
    **              4 at (1, 1, -1)
    **              5 at (-1, 1, -1)
    **              6 at (-1, -1, -1)
    **              7 at (1, -1, -1)
    **              8 at (1, 0, 0)
    **              9 at (-1, 0, 0)
    **              10 at (0, 1, 0)
    **              11 at (0, -1, 0)
    **              12 at (0, 0, 1)
    **              13 at (0, 0, -1)
    **              14 at (0, 0, 0)
    **
    **                  ____________
    **  Faces:         /|          /|
    **                / |   4     / |
    **               /___________/  |
    **              |   |    1   |  |
    **              | 3 |        | 2|
    **              |   |  0     |  |
    **              |   |________|__|
    **              |  /         | /
    **              | /     5    |/
    **              |____________|
    **
    */

    /* Allocate vertices first. */
    result->vertices = New(Vector, result->num_vertices);
    VNew(1, 1, 1, result->vertices[0]);
    VNew(-1, 1, 1, result->vertices[1]);
    VNew(-1, -1, 1, result->vertices[2]);
    VNew(1, -1, 1, result->vertices[3]);
    VNew(1, 1, -1, result->vertices[4]);
    VNew(-1, 1, -1, result->vertices[5]);
    VNew(-1, -1, -1, result->vertices[6]);
    VNew(1, -1, -1, result->vertices[7]);
    VNew(1, 0, 0, result->vertices[8]);
    VNew(-1, 0, 0, result->vertices[9]);
    VNew(0, 1, 0, result->vertices[10]);
    VNew(0, -1, 0, result->vertices[11]);
    VNew(0, 0, 1, result->vertices[12]);
    VNew(0, 0, -1, result->vertices[13]);
    VNew(0, 0, 0, result->vertices[14]);

    /* Allocate normals. */
    result->normals = New(Vector, result->num_normals);
    VNew(1, 0, 0, result->normals[0]);
    VNew(-1, 0, 0, result->normals[1]);
    VNew(0, 1, 0, result->normals[2]);
    VNew(0, -1, 0, result->normals[3]);
    VNew(0, 0, 1, result->normals[4]);
    VNew(0, 0, -1, result->normals[5]);

    /* Allocate faces. */
    result->faces = New(Face, result->num_faces);
    for ( i = 0 ; i < result->num_faces ; i++ )
    {
        result->faces[i].num_vertices = 4;
        result->faces[i].vertices = New(int, 4);
        result->faces[i].normals = New(int, 4);
        result->faces[i].face_attribs = NULL;
    }

    result->faces[0].vertices[0] = 0;   result->faces[0].normals[0] = 0;
    result->faces[0].vertices[1] = 4;   result->faces[0].normals[1] = 0;
    result->faces[0].vertices[2] = 7;   result->faces[0].normals[2] = 0;
    result->faces[0].vertices[3] = 3;   result->faces[0].normals[3] = 0;
    VNew(1, 0, 0, result->faces[0].normal);

    result->faces[1].vertices[0] = 1;   result->faces[1].normals[0] = 1;
    result->faces[1].vertices[1] = 2;   result->faces[1].normals[1] = 1;
    result->faces[1].vertices[2] = 6;   result->faces[1].normals[2] = 1;
    result->faces[1].vertices[3] = 5;   result->faces[1].normals[3] = 1;
    VNew(-1, 0, 0, result->faces[1].normal);

    result->faces[2].vertices[0] = 0;   result->faces[2].normals[0] = 2;
    result->faces[2].vertices[1] = 1;   result->faces[2].normals[1] = 2;
    result->faces[2].vertices[2] = 5;   result->faces[2].normals[2] = 2;
    result->faces[2].vertices[3] = 4;   result->faces[2].normals[3] = 2;
    VNew(0, 1, 0, result->faces[2].normal);

    result->faces[3].vertices[0] = 2;   result->faces[3].normals[0] = 3;
    result->faces[3].vertices[1] = 3;   result->faces[3].normals[1] = 3;
    result->faces[3].vertices[2] = 7;   result->faces[3].normals[2] = 3;
    result->faces[3].vertices[3] = 6;   result->faces[3].normals[3] = 3;
    VNew(0, -1, 0, result->faces[3].normal);

    result->faces[4].vertices[0] = 3;   result->faces[4].normals[0] = 4;
    result->faces[4].vertices[1] = 2;   result->faces[4].normals[1] = 4;
    result->faces[4].vertices[2] = 1;   result->faces[4].normals[2] = 4;
    result->faces[4].vertices[3] = 0;   result->faces[4].normals[3] = 4;
    VNew(0, 0, 1, result->faces[4].normal);

    result->faces[5].vertices[0] = 4;   result->faces[5].normals[0] = 5;
    result->faces[5].vertices[1] = 5;   result->faces[5].normals[1] = 5;
    result->faces[5].vertices[2] = 6;   result->faces[5].normals[2] = 5;
    result->faces[5].vertices[3] = 7;   result->faces[5].normals[3] = 5;
    VNew(0, 0, -1, result->faces[5].normal);

    result->edges = NULL;
    result->num_attribs = 0;
    result->attribs = NULL;

    return result;

}


/*  WireframePtr
**  Generic_Square_Wireframe();
**  Returns a pointer to a NEW wireframe structure for a generic square.
**  NULL on failure.
*/
WireframePtr
Generic_Square_Wireframe()
{
    WireframePtr    result = New(Wireframe, 1);
    int             i, j;

    /*  A square wireframe has 8 faces, 5 vertices. */
    /*  There are no additional reference vertices. */
    result->num_faces = 8;
    result->num_vertices = 5;
    result->num_normals = 2;
    result->num_real_verts = 5;


    /*  It looks like this:
    **
    **
    **                  2 (-1,-1,0) ____________1_____ 1 (-1, 1, 0)
    **                             /\              __/
    **                            /  6    1    __5- /
    **                           /    \_   __--    /
    **                          2  2   _4(0,0,0)0 /
    **                         /   _7--   \_     /
    **                        /__--    3    4_  0
    **                       /________3_______\/
    **             3 (1, -1, 0)              0 (1,1,0)
    **
    */

    /* Allocate vertices first. */
    result->vertices = New(Vector, result->num_vertices);
    VNew(1, 1, 0, result->vertices[0]);
    VNew(-1, 1, 0, result->vertices[1]);
    VNew(-1, -1, 0, result->vertices[2]);
    VNew(1, -1, 0, result->vertices[3]);
    VNew(0, 0, 0, result->vertices[4]);

    result->normals = New(Vector, result->num_normals);
    VNew(0, 0, 1, result->normals[0]);
    VNew(0, 0, -1, result->normals[1]);

    /* Finally, allocate the faces. */
    result->faces = New(Face, result->num_faces);
    for ( i = 0 ; i < result->num_faces ; i++ )
    {
        result->faces[i].num_vertices = 3;
        result->faces[i].vertices = New(int, 3);
        result->faces[i].normals = New(int, 3);
        result->faces[i].vertices[2] = 4;
        for ( j = 0 ; j < 3 ; j++ )
	    if ( i / 4 )
		result->faces[i].normals[j] = 1;
	    else
		result->faces[i].normals[j] = 0;
        if ( i / 4 )
            VNew(0, 0, -1, result->faces[i].normal);
        else
            VNew(0, 0, 1, result->faces[i].normal);
        result->faces[i].face_attribs = NULL;
    }

    result->faces[0].vertices[0] = 1; result->faces[0].vertices[1] = 0;
    result->faces[1].vertices[0] = 2; result->faces[1].vertices[1] = 1;
    result->faces[2].vertices[0] = 3; result->faces[2].vertices[1] = 2;
    result->faces[3].vertices[0] = 0; result->faces[3].vertices[1] = 3;
    result->faces[4].vertices[0] = 0; result->faces[4].vertices[1] = 1;
    result->faces[5].vertices[0] = 1; result->faces[5].vertices[1] = 2;
    result->faces[6].vertices[0] = 2; result->faces[6].vertices[1] = 3;
    result->faces[7].vertices[0] = 3; result->faces[7].vertices[1] = 0;

    result->edges = NULL;
    result->num_attribs = 0;
    result->attribs = NULL;

    return result;
}



/*  WireframePtr
**  Generic_Plane_Wireframe();
**  Returns a pointer to a NEW wireframe structure for a generic plane.
**  NULL on failure.
*/
WireframePtr
Generic_Plane_Wireframe()
{
    WireframePtr    result = New(Wireframe, 1);
    int             i, j;

    /*  A plane wireframe has 1 face, 6 edges and 12 vertices.  */
    /*  There is one additional reference vertex in the center. */
    result->num_faces = 10;
    result->num_vertices = 16;
    result->num_normals = 2;
    result->num_real_verts = 15;


    /*  It looks like an infinite square:
    **
    **
    **
    **                                   7 (-10, 0, 0)
    **                          8 (-10,-1,0)            6 (-10,1,0)
    **                                 _____________9____ 
    **                                /        /        /
    **                               /        /        /
    **         9 (-1,-10,0) ________/________/________/____5___ 5 (-1,10,0)
    **                     /       /        /        /         /
    **                    / 3,7   /        /        /         /
    **                   /       /        /        /         /
    **      10 (0,-10,0)/-------/--------/--------/-----4---/ 4 (0,10,0)
    **                 /       /        /        /         /
    **                6  2,6  /        /        /         7
    **   11 (1,-10,0)/_______/________/________/_____3___/ 3 (1,10,0)
    **                      /        /        /
    **                     0  0,4   1  1,5   2
    **                    /        /        /
    **                   /________/___8____/
    **          0 (10,-1, 0)              2 (10,1,0)
    **                    1 (10,0,0)
    **
    **  There is also a little arrow pointing in the normal direction.
    */

    /* Allocate vertices first. */
    result->vertices = New(Vector, result->num_vertices);

    VNew(10, -1, 0, result->vertices[0]);
    VNew(10, 0, 0, result->vertices[1]);
    VNew(10, 1, 0, result->vertices[2]);
    VNew(1, 10, 0, result->vertices[3]);
    VNew(0, 10, 0, result->vertices[4]);
    VNew(-1, 10, 0, result->vertices[5]);
    VNew(-10, 1, 0, result->vertices[6]);
    VNew(-10, 0, 0, result->vertices[7]);
    VNew(-10, -1, 0, result->vertices[8]);
    VNew(-1, -10, 0, result->vertices[9]);
    VNew(0, -10, 0, result->vertices[10]);
    VNew(1, -10, 0, result->vertices[11]);
    VNew(1,   0, 0, result->vertices[12]);
    VNew(-1,  0, 0, result->vertices[13]);
    VNew(0,   0, -0.5, result->vertices[14]);
    VNew(0, 0, 0, result->vertices[15]);

    result->normals = New(Vector, result->num_normals);
    VNew(0, 0, 1, result->normals[0]);
    VNew(0, 0, -1, result->normals[1]);

    result->faces = New(Face, result->num_faces);
    for ( i = 0 ; i < 8 ; i++ )
    {
        result->faces[i].num_vertices = 4;
        result->faces[i].vertices = New(int, 4);
        result->faces[i].normals = New(int, 4);
        for ( j = 0 ; j < 4 ; j++ )
	    if ( i / 4 )
		result->faces[i].normals[j] = 1;
	    else
		result->faces[i].normals[j] = 0;
        if ( i / 4 )
            VNew(0, 0, -1, result->faces[i].normal);
        else
            VNew(0, 0, 1, result->faces[i].normal);
        result->faces[i].face_attribs = NULL;
    }
    for ( ; i < result->num_faces ; i++ )
    {
        result->faces[i].num_vertices = 3;
        result->faces[i].vertices = New(int, 3);
        result->faces[i].normals = New(int, 3);
        for ( j = 0 ; j < 3 ; j++ )
            result->faces[i].normals[j] = 0;
        result->faces[i].face_attribs = NULL;
    }
    VNew(0, 1, 0, result->faces[8].normal);
    VNew(0, -1, 0, result->faces[9].normal);

    result->faces[0].vertices[0] = 1;   result->faces[0].vertices[1] = 0;
    result->faces[0].vertices[2] = 8;   result->faces[0].vertices[3] = 7;
    result->faces[1].vertices[0] = 2;   result->faces[1].vertices[1] = 1;
    result->faces[1].vertices[2] = 7;   result->faces[1].vertices[3] = 6;
    result->faces[2].vertices[0] = 11;  result->faces[2].vertices[1] = 10;
    result->faces[2].vertices[2] = 4;   result->faces[2].vertices[3] = 3;
    result->faces[3].vertices[0] = 10;  result->faces[3].vertices[1] = 9;
    result->faces[3].vertices[2] = 5;   result->faces[3].vertices[3] = 4;
    result->faces[4].vertices[0] = 0;   result->faces[4].vertices[1] = 1;
    result->faces[4].vertices[2] = 7;   result->faces[4].vertices[3] = 8;
    result->faces[5].vertices[0] = 1;   result->faces[5].vertices[1] = 2;
    result->faces[5].vertices[2] = 6;   result->faces[5].vertices[3] = 7;
    result->faces[6].vertices[0] = 3;   result->faces[6].vertices[1] = 4;
    result->faces[6].vertices[2] = 10;  result->faces[6].vertices[3] = 11;
    result->faces[7].vertices[0] = 4;   result->faces[7].vertices[1] = 5;
    result->faces[7].vertices[2] = 9;   result->faces[7].vertices[3] = 10;
    result->faces[8].vertices[0] = 12;  result->faces[8].vertices[1] = 13;
    result->faces[8].vertices[2] = 14;
    result->faces[9].vertices[0] = 14;  result->faces[9].vertices[1] = 13;
    result->faces[9].vertices[2] = 12;

    result->edges = NULL;
    result->num_attribs = 0;
    result->attribs = 0;

    return result;
}





/*  WireframePtr
**  Generic_Cylinder_Wireframe();
**  Returns a pointer to a NEW wireframe structure for a generic cylinder.
**  NULL on failure.
*/
WireframePtr
Generic_Cylinder_Wireframe()
{
    WireframePtr    result = New(Wireframe, 1);
    double          norm1, norm2;
    int             i, j;

    /*  A cylinder wireframe has 10 faces, 24 edges and 16 vertices.
    **  There are 3 reference vertices, one at the center of each endcap
    **  and one in the center of the object.
    */
    result->num_faces = 10;
    result->num_vertices = 19;
    result->num_normals = 10;
    result->num_real_verts = 16;


    /*  Vertices are arranged 0-7 around the top, 8-15 on the bottom.
    **  Faces are 0-7 around the sides, 8 on top and 9 on bottom.
    */

    /* Allocate vertices first. */
    result->vertices = New(Vector, result->num_vertices);
    VNew( 1,       0,       1, result->vertices[0]);
    VNew( M_SQRT1_2,  M_SQRT1_2,  1, result->vertices[1]);
    VNew( 0,       1,       1, result->vertices[2]);
    VNew(-M_SQRT1_2,  M_SQRT1_2,  1, result->vertices[3]);
    VNew(-1,       0,       1, result->vertices[4]);
    VNew(-M_SQRT1_2, -M_SQRT1_2,  1, result->vertices[5]);
    VNew( 0,      -1,       1, result->vertices[6]);
    VNew( M_SQRT1_2, -M_SQRT1_2,  1, result->vertices[7]);
    VNew( 1,       0,      -1, result->vertices[8]);
    VNew( M_SQRT1_2,  M_SQRT1_2, -1, result->vertices[9]);
    VNew( 0,       1,      -1, result->vertices[10]);
    VNew(-M_SQRT1_2,  M_SQRT1_2, -1, result->vertices[11]);
    VNew(-1,       0,      -1, result->vertices[12]);
    VNew(-M_SQRT1_2, -M_SQRT1_2, -1, result->vertices[13]);
    VNew( 0,      -1,      -1, result->vertices[14]);
    VNew( M_SQRT1_2, -M_SQRT1_2, -1, result->vertices[15]);
    VNew( 0, 0, 1, result->vertices[16]);
    VNew( 0, 0, -1, result->vertices[17]);
    VNew( 0, 0, 0, result->vertices[18]);

    result->normals = New(Vector, result->num_normals);
    VNew(         1,          0, 0,  result->normals[0]);
    VNew( M_SQRT1_2,  M_SQRT1_2, 0, result->normals[1]);
    VNew(         0,          1, 0, result->normals[2]);
    VNew(-M_SQRT1_2,  M_SQRT1_2, 0, result->normals[3]);
    VNew(        -1,          0, 0, result->normals[4]);
    VNew(-M_SQRT1_2, -M_SQRT1_2, 0, result->normals[5]);
    VNew(         0,         -1, 0, result->normals[6]);
    VNew( M_SQRT1_2, -M_SQRT1_2, 0, result->normals[7]);
    VNew(0, 0, 1,  result->normals[8]);
    VNew(0, 0, -1,  result->normals[9]);

    /* Finally, allocate the faces. */
    result->faces = New(Face, result->num_faces);
    for ( i = 0 ; i < 8 ; i++ )
    {
        result->faces[i].num_vertices = 4;
        result->faces[i].vertices = New(int, 4);
        result->faces[i].vertices[0] = i;
        result->faces[i].vertices[1] = i + 1;
        result->faces[i].vertices[2] = i + 9;
        result->faces[i].vertices[3] = i + 8;
        result->faces[i].normals = New(int, 4);
        result->faces[i].normals[0] = i;
        result->faces[i].normals[1] = i + 1;
        result->faces[i].normals[2] = i + 1;
        result->faces[i].normals[3] = i;
        result->faces[i].face_attribs = NULL;
    }
    result->faces[7].vertices[1] = 0;
    result->faces[7].normals[1] = 0;
    result->faces[7].vertices[2] = 8;
    result->faces[7].normals[2] = 0;

    norm1 = 1.0 / ( M_SQRT2 * sqrt( 2.0 - M_SQRT2 ) );
    norm2 = sqrt( 2.0 - M_SQRT2 ) / 2.0;

    VNew(norm1, norm2, 0, result->faces[0].normal);
    VNew(norm2, norm1, 0, result->faces[1].normal);
    VNew(-norm2, norm1, 0, result->faces[2].normal);
    VNew(-norm1, norm2, 0, result->faces[3].normal);
    VNew(-norm1, -norm2, 0, result->faces[4].normal);
    VNew(-norm2, -norm1, 0, result->faces[5].normal);
    VNew(norm2, -norm1, 0, result->faces[6].normal);
    VNew(norm1, -norm2, 0, result->faces[7].normal);

    for ( ; i < result->num_faces ; i++ )
    {
        result->faces[i].num_vertices = 8;
        result->faces[i].vertices = New(int, 8);
        result->faces[i].normals = New(int, 8);
        for ( j = 0 ; j < 8 ; j++ )
            result->faces[i].normals[j] = i;
        result->faces[i].face_attribs = NULL;
    }
    for ( i = 0 ; i < 8 ; i++ )
        result->faces[8].vertices[i] = 7 - i;
    VNew(0, 0, 1, result->faces[8].normal);
    for ( i = 0 ; i < 8 ; i++ )
        result->faces[9].vertices[i] = i + 8;
    VNew(0, 0, -1, result->faces[9].normal);

    result->edges = NULL;
    result->num_attribs = 0;
    result->attribs = NULL;

    return result;
}



/*  WireframePtr
**  Generic_Cone_Wireframe();
**  Returns a pointer to a NEW wireframe structure for a generic cone.
*/
WireframePtr
Generic_Cone_Wireframe()
{
    WireframePtr    result = New(Wireframe, 1);
    double          temp_d;
    double          sqrt1_5 = sqrt(0.2);
    int             i;

    /*  A cone wireframe has 9 faces, 16 edges and 11 vertices. */
    /*  There is a reference vertex at the center.              */
    result->num_faces = 9;
    result->num_vertices = 11;
    result->num_normals = 17;
    result->num_real_verts = 9;


    /*  Vertices are arranged 0 at the top, 1-8 on the bottom.
    **  Faces are 0-7 around the sides, 8 on the bottom.
    */

    /* Allocate vertices first. */
    result->vertices = New(Vector, result->num_vertices);
    VNew( 0,       0,       1, result->vertices[0]);
    VNew( 1,       0,      -1, result->vertices[1]);
    VNew( M_SQRT1_2,  M_SQRT1_2, -1, result->vertices[2]);
    VNew( 0,       1,      -1, result->vertices[3]);
    VNew(-M_SQRT1_2,  M_SQRT1_2, -1, result->vertices[4]);
    VNew(-1,       0,      -1, result->vertices[5]);
    VNew(-M_SQRT1_2, -M_SQRT1_2, -1, result->vertices[6]);
    VNew( 0,      -1,      -1, result->vertices[7]);
    VNew( M_SQRT1_2, -M_SQRT1_2, -1, result->vertices[8]);
    VNew( 0, 0, -1, result->vertices[9]);
    VNew( 0, 0, 0, result->vertices[10]);

    result->normals = New(Vector, result->num_normals);
    VNew(2 * sqrt1_5, 0, sqrt1_5, result->normals[0]);
    VNew(M_SQRT2 * sqrt1_5, M_SQRT2 * sqrt1_5, sqrt1_5, result->normals[1]);
    VNew(0, 2 * sqrt1_5, sqrt1_5, result->normals[2]);
    VNew(-M_SQRT2 * sqrt1_5, M_SQRT2 * sqrt1_5, sqrt1_5, result->normals[3]);
    VNew(-2 * sqrt1_5, 0, sqrt1_5, result->normals[4]);
    VNew(-M_SQRT2 * sqrt1_5, -M_SQRT2 * sqrt1_5, sqrt1_5, result->normals[5]);
    VNew(0, 2 * sqrt1_5, sqrt1_5, result->normals[6]);
    VNew(M_SQRT2 * sqrt1_5, -M_SQRT2 * sqrt1_5, sqrt1_5, result->normals[7]);
    VNew(M_SQRT2, 2 - M_SQRT2, M_SQRT1_2, result->normals[8]);
    VNew(2 - M_SQRT2, M_SQRT2, M_SQRT1_2, result->normals[9]);
    VNew(M_SQRT2 - 2, M_SQRT2, M_SQRT1_2, result->normals[10]);
    VNew(-M_SQRT2, 2 - M_SQRT2, M_SQRT1_2, result->normals[11]);
    VNew(-M_SQRT2, M_SQRT2 - 2, M_SQRT1_2, result->normals[12]);
    VNew( M_SQRT2 - 2, -M_SQRT2, M_SQRT1_2, result->normals[13]);
    VNew(2 - M_SQRT2, -M_SQRT2, M_SQRT1_2, result->normals[14]);
    VNew(M_SQRT2, M_SQRT2 - 2, M_SQRT1_2, result->normals[15]);
    VNew(0, 0, -1, result->normals[16]);

    result->faces = New(Face, result->num_faces);

    for ( i = 0 ; i < 8 ; i++ )
    {
        result->faces[i].num_vertices = 3;
        result->faces[i].vertices = New(int, 3);
        result->faces[i].normals = New(int, 3);
        result->faces[i].vertices[0] = 0;
        result->faces[i].normals[0] = i + 8;
        result->faces[i].vertices[1] = i + 2;
        result->faces[i].normals[1] = i + 1;
        result->faces[i].vertices[2] = i + 1;
        result->faces[i].normals[2] = i;
        result->faces[i].face_attribs = NULL;
    }
    result->faces[7].vertices[1] = 1;
    result->faces[7].normals[1] = 0;

    VNew(M_SQRT2, 2 - M_SQRT2, M_SQRT1_2, result->faces[0].normal);
    VNew(2 - M_SQRT2, M_SQRT2, M_SQRT1_2, result->faces[1].normal);
    VNew(M_SQRT2 - 2, M_SQRT2, M_SQRT1_2, result->faces[2].normal);
    VNew(-M_SQRT2, 2 - M_SQRT2, M_SQRT1_2, result->faces[3].normal);
    VNew(-M_SQRT2, M_SQRT2 - 2, M_SQRT1_2, result->faces[4].normal);
    VNew( M_SQRT2 - 2, -M_SQRT2, M_SQRT1_2, result->faces[5].normal);
    VNew(2 - M_SQRT2, -M_SQRT2, M_SQRT1_2, result->faces[6].normal);
    VNew(M_SQRT2, M_SQRT2 - 2, M_SQRT1_2, result->faces[7].normal);

    result->faces[8].num_vertices = 8;
    result->faces[8].vertices = New(int, 8);
    result->faces[8].normals = New(int, 8);
    for ( i = 0 ; i < 8 ; i++ )
    {
        result->faces[8].vertices[i] = i + 1;
        result->faces[8].normals[i] = 16;
    }
    VNew(0, 0, -1, result->faces[8].normal);
    result->faces[8].face_attribs = NULL;

    /* Normalize the normals. */
    for ( i = 0 ; i < result->num_faces - 1 ; i++ )
        VUnit(result->faces[i].normal, temp_d, result->faces[i].normal);

    result->edges = NULL;
    result->attribs = NULL;
    result->num_attribs = 0;

    return result;
}



/*  WireframePtr
**  Generic_Sphere_Wireframe();
**  Returns a pointer to a NEW wireframe structure for a generic sphere.
*/
WireframePtr
Generic_Sphere_Wireframe()
{
    WireframePtr    result = New(Wireframe, 1);
    double	    sqrt_1_3 = 1.0 / sqrt(3.0);
    Vector	    v1, v2;
    double	    temp_d;
    int             i;

    result->num_faces = 48;
    result->num_vertices = 27;
    result->num_normals = 26;
    result->num_real_verts = 26;


    /*  Vertices are arranged 0 at the top, 1 at the bottom, then in groups
    **  of 8 around each band from top to bottom.
    */

    result->vertices = New(Vector, result->num_vertices);
    VNew( 0.0      , 0.0      , 1.0      , result->vertices[0]);
    VNew( 0.0      , 0.0      ,-1.0      , result->vertices[1]);
    VNew( M_SQRT1_2, 0.0      , M_SQRT1_2, result->vertices[2]);
    VNew( sqrt_1_3 , sqrt_1_3 , sqrt_1_3 , result->vertices[3]);
    VNew( 0.0      , M_SQRT1_2, M_SQRT1_2, result->vertices[4]);
    VNew(-sqrt_1_3 , sqrt_1_3 , sqrt_1_3 , result->vertices[5]);
    VNew(-M_SQRT1_2, 0.0      , M_SQRT1_2, result->vertices[6]);
    VNew(-sqrt_1_3 ,-sqrt_1_3 , sqrt_1_3 , result->vertices[7]);
    VNew( 0.0      ,-M_SQRT1_2, M_SQRT1_2, result->vertices[8]);
    VNew( sqrt_1_3 ,-sqrt_1_3 , sqrt_1_3 , result->vertices[9]);
    VNew( 1.0      , 0.0      , 0.0      , result->vertices[10]);
    VNew( M_SQRT1_2, M_SQRT1_2, 0.0      , result->vertices[11]);
    VNew( 0.0      , 1.0      , 0.0      , result->vertices[12]);
    VNew(-M_SQRT1_2, M_SQRT1_2, 0.0      , result->vertices[13]);
    VNew(-1.0      , 0.0      , 0.0      , result->vertices[14]);
    VNew(-M_SQRT1_2,-M_SQRT1_2, 0.0      , result->vertices[15]);
    VNew( 0.0      ,-1.0      , 0.0      , result->vertices[16]);
    VNew( M_SQRT1_2,-M_SQRT1_2, 0.0      , result->vertices[17]);
    VNew( M_SQRT1_2, 0.0      ,-M_SQRT1_2, result->vertices[18]);
    VNew( sqrt_1_3 , sqrt_1_3 ,-sqrt_1_3 , result->vertices[19]);
    VNew( 0.0      , M_SQRT1_2,-M_SQRT1_2, result->vertices[20]);
    VNew(-sqrt_1_3 , sqrt_1_3 ,-sqrt_1_3 , result->vertices[21]);
    VNew(-M_SQRT1_2, 0.0      ,-M_SQRT1_2, result->vertices[22]);
    VNew(-sqrt_1_3 ,-sqrt_1_3 ,-sqrt_1_3 , result->vertices[23]);
    VNew( 0.0      ,-M_SQRT1_2,-M_SQRT1_2, result->vertices[24]);
    VNew( sqrt_1_3 ,-sqrt_1_3 ,-sqrt_1_3 , result->vertices[25]);
    VNew( 0.0      , 0.0      , 0.0      , result->vertices[26]);

    result->normals = New(Vector, result->num_normals);
    for ( i = 0 ; i < result->num_normals ; i++ )
        result->normals[i] = result->vertices[i];

    /* Finally, allocate the faces. */
    result->faces = New(Face, result->num_faces);

    for ( i = 0 ; i < 8 ; i++ )
    {
        result->faces[i].num_vertices = 3;
        result->faces[i].vertices = New(int, 3);
        result->faces[i].normals = New(int, 3);
        result->faces[i].vertices[0] = 0;
        result->faces[i].vertices[1] = i + 3;
        result->faces[i].vertices[2] = i + 2;
        result->faces[i].normals[0] = 0;
        result->faces[i].normals[1] = i + 3;
        result->faces[i].normals[2] = i + 2;
        result->faces[i].face_attribs = NULL;
    }
    result->faces[7].vertices[1] = 2;
    result->faces[7].normals[1] = 2;

    for ( i = 40 ; i < 48 ; i++ )
    {
        result->faces[i].num_vertices = 3;
        result->faces[i].vertices = New(int, 3);
        result->faces[i].normals = New(int, 3);
        result->faces[i].vertices[0] = 1;
        result->faces[i].vertices[1] = i - 22;
        result->faces[i].vertices[2] = i - 21;
        result->faces[i].normals[0] = 1;
        result->faces[i].normals[1] = i - 22;
        result->faces[i].normals[2] = i - 21;
        result->faces[i].face_attribs = NULL;
    }
    result->faces[47].vertices[2] = 18;
    result->faces[47].normals[2] = 18;

#define FNew(f, v1, v2, v3) { \
	result->faces[f].num_vertices = 3; \
	result->faces[f].vertices = New(int, 3); \
	result->faces[f].normals = New(int, 3); \
	result->faces[f].vertices[0] = v1; \
	result->faces[f].vertices[1] = v2; \
	result->faces[f].vertices[2] = v3; \
	result->faces[f].normals[0] = v1; \
	result->faces[f].normals[1] = v2; \
	result->faces[f].normals[2] = v3; \
	result->faces[f].face_attribs = NULL; \
    }

    FNew(8, 10, 2, 3)
    FNew(9, 10, 3, 11)
    FNew(10, 10, 17, 9)
    FNew(11, 10, 9, 2)
    FNew(12, 12, 4, 5)
    FNew(13, 12, 5, 13)
    FNew(14, 12, 11, 3)
    FNew(15, 12, 3, 4)
    FNew(16, 14, 6, 7)
    FNew(17, 14, 7, 15)
    FNew(18, 14, 13, 5)
    FNew(19, 14, 5, 6)
    FNew(20, 16, 8, 9)
    FNew(21, 16, 9, 17)
    FNew(22, 16, 15, 7)
    FNew(23, 16, 7, 8)

    FNew(24, 10, 25, 17)
    FNew(25, 10, 18, 25)
    FNew(26, 10, 19, 18)
    FNew(27, 10, 11, 19)
    FNew(28, 12, 19, 11)
    FNew(29, 12, 20, 19)
    FNew(30, 12, 21, 20)
    FNew(31, 12, 13, 21)
    FNew(32, 14, 21, 13)
    FNew(33, 14, 22, 21)
    FNew(34, 14, 23, 22)
    FNew(35, 14, 15, 23)
    FNew(36, 16, 23, 15)
    FNew(37, 16, 24, 23)
    FNew(38, 16, 25, 24)
    FNew(39, 16, 17, 25)

#undef FNew

    /* Set the face normals. */
    for ( i = 0 ; i < result->num_faces ; i++ )
    {
	VSub(result->vertices[result->faces[i].vertices[0]],
	     result->vertices[result->faces[i].vertices[1]], v1);
	VSub(result->vertices[result->faces[i].vertices[2]],
	     result->vertices[result->faces[i].vertices[1]], v2);
	VCross(v1, v2, result->faces[i].normal);
        VUnit(result->faces[i].normal, temp_d, result->faces[i].normal);
    }

    result->edges = NULL;
    result->num_attribs = 0;
    result->attribs = NULL;

    return result;
}


/*  WireframePtr
**  Generic_Light_Wireframe();
**  Returns a pointer to a NEW wireframe structure for a light.
*/
WireframePtr
Generic_Light_Wireframe()
{
    WireframePtr    result = New(Wireframe, 1);
    int             i, j;

    /*  A light wireframe has 6 faces, 3 edges and 7 vertices.  */
    /*  There are no additional reference vertices.             */
    result->num_faces = 6;
    result->num_vertices = 7;
    result->num_normals = 1;    /* Dummy place holder. */
    result->num_real_verts = 6;

    /* Allocate vertices first. */
    result->vertices = New(Vector, result->num_vertices);
    VNew(0.5, 0, 0, result->vertices[0]);
    VNew(-0.5, 0, 0, result->vertices[1]);
    VNew(0, 0.5, 0, result->vertices[2]);
    VNew(0, -0.5, 0, result->vertices[3]);
    VNew(0, 0, 0.50, result->vertices[4]);
    VNew(0, 0, -0.50, result->vertices[5]);
    VNew(0, 0, 0, result->vertices[6]);

    result->normals= New(Vector, 1);
    VNew(0, 0, 1, result->normals[0]);

    /* Finally, allocate the faces. */
    result->faces = New(Face, result->num_faces);
    for ( i = 0 ; i < 6 ; i++ )
    {
        result->faces[i].num_vertices = 4;
        result->faces[i].vertices = New(int, 4);
        result->faces[i].normals = New(int, 4);
        for ( j = 0 ; j < 4 ; j++ )
            result->faces[i].normals[j] = 0;
        result->faces[i].face_attribs = NULL;
    }

    result->faces[0].vertices[0] = 4;   result->faces[0].vertices[1] = 2;
    result->faces[0].vertices[2] = 5;   result->faces[0].vertices[3] = 3;
    VNew(1, 0, 0, result->faces[0].normal);
    result->faces[1].vertices[0] = 4;   result->faces[1].vertices[1] = 3;
    result->faces[1].vertices[2] = 5;   result->faces[1].vertices[3] = 2;
    VNew(-1, 0, 0, result->faces[1].normal);
    result->faces[2].vertices[0] = 4;   result->faces[2].vertices[1] = 1;
    result->faces[2].vertices[2] = 5;   result->faces[2].vertices[3] = 0;
    VNew(0, 1, 0, result->faces[2].normal);
    result->faces[3].vertices[0] = 5;   result->faces[3].vertices[1] = 1;
    result->faces[3].vertices[2] = 4;   result->faces[3].vertices[3] = 0;
    VNew(0, -1, 0, result->faces[3].normal);
    result->faces[4].vertices[0] = 1;   result->faces[4].vertices[1] = 2;
    result->faces[4].vertices[2] = 2;   result->faces[4].vertices[3] = 3;
    VNew(0, 0, 1, result->faces[4].normal);
    result->faces[5].vertices[0] = 0;   result->faces[5].vertices[1] = 2;
    result->faces[5].vertices[2] = 1;   result->faces[5].vertices[3] = 3;
    VNew(0, 0, -1, result->faces[5].normal);

    result->edges = NULL;
    result->attribs = NULL;
    result->num_attribs = 0;

    return result;
}



/*  WireframePtr
**  Generic_Spot_Light_Wireframe();
**  Returns a pointer to a NEW wireframe structure for a spotlight wireframe.
**  A spotlight is virtually the same as a cone, but has a different origin
**  and is considered open.
**  NULL on failure.
*/
WireframePtr
Generic_Spot_Light_Wireframe()
{
    WireframePtr    result = New(Wireframe, 1);
    double          temp_d;
    int             i;

    /*  A spotlight wireframe has 8 faces and 10 vertices.  */
    /*  There is a reference vertex at the center.          */
    result->num_faces = 8;
    result->num_vertices = 11;
    result->num_normals = 1;    /* Dummy. Lights aren't ever exported. */
    result->num_real_verts = 9;


    /*  Vertices are arranged 0 at the top, 1-8 on the bottom.
    **  Faces are 0-7 around the sides.
    */

    /* Allocate vertices first. */
    result->vertices = New(Vector, result->num_vertices);
    VNew( 1,       0,      -1, result->vertices[0]);
    VNew( M_SQRT1_2,  M_SQRT1_2, -1, result->vertices[1]);
    VNew( 0,       1,      -1, result->vertices[2]);
    VNew(-M_SQRT1_2,  M_SQRT1_2, -1, result->vertices[3]);
    VNew(-1,       0,      -1, result->vertices[4]);
    VNew(-M_SQRT1_2, -M_SQRT1_2, -1, result->vertices[5]);
    VNew( 0,      -1,      -1, result->vertices[6]);
    VNew( M_SQRT1_2, -M_SQRT1_2, -1, result->vertices[7]);
    VNew( 0, 0, -1, result->vertices[8]);
    VNew( 0, 0, 0, result->vertices[9]);

    result->normals = New(Vector, result->num_normals);
    VNew(0, 0, -1, result->normals[0]);

    /* Finally, allocate the faces. */
    result->faces = New(Face, result->num_faces);
    for ( i = 0 ; i < result->num_faces ; i++ )
    {
        result->faces[i].num_vertices = 3;
        result->faces[i].vertices = New(int, 3);
        result->faces[i].vertices[0] = 9;
        result->faces[i].vertices[1] = i + 1;
        result->faces[i].vertices[2] = i;
        result->faces[i].normals = New(int, 3);
        result->faces[i].normals[0] = 0;
        result->faces[i].normals[1] = 0;
        result->faces[i].normals[2] = 0;
        result->faces[i].face_attribs = NULL;
    }
    result->faces[7].vertices[1] = 0;
    VNew(M_SQRT2, 2 - M_SQRT2, M_SQRT1_2, result->faces[0].normal);
    VNew(2 - M_SQRT2, M_SQRT2, M_SQRT1_2, result->faces[1].normal);
    VNew(M_SQRT2 - 2, M_SQRT2, M_SQRT1_2, result->faces[2].normal);
    VNew(-M_SQRT2, 2 - M_SQRT2, M_SQRT1_2, result->faces[3].normal);
    VNew(-M_SQRT2, M_SQRT2 - 2, M_SQRT1_2, result->faces[4].normal);
    VNew( M_SQRT2 - 2, -M_SQRT2, M_SQRT1_2, result->faces[5].normal);
    VNew(2 - M_SQRT2, -M_SQRT2, M_SQRT1_2, result->faces[6].normal);
    VNew(M_SQRT2, M_SQRT2 - 2, M_SQRT1_2, result->faces[7].normal);

    /* Normalize the normals. */
    for ( i = 0 ; i < result->num_faces ; i++ )
        VUnit(result->faces[i].normal, temp_d, result->faces[i].normal);

    result->edges = NULL;
    result->attribs = NULL;
    result->num_attribs = 0;

    return result;
}


/*  Create a wireframe for a torus with tube radius 1 and a mean radius 3.
*/
WireframePtr
Generic_Torus_Wireframe()
{
    return Dense_Torus_Wireframe(0);
}


/*  Create a wireframe for a camera object.
*/
WireframePtr
Generic_Camera_Wireframe()
{
    WireframePtr    result = New(Wireframe, 1);
    int             i;

    result->num_faces = 6;
    result->num_vertices = 7;
    result->num_normals = 1;
    result->num_real_verts = 6;

    /* Allocate vertices first. */
    result->vertices = New(Vector, result->num_vertices);
    VNew( 0, 0, 1, result->vertices[0]);
    VNew( 1, 1, 0, result->vertices[1]);
    VNew(-1, 1, 0, result->vertices[2]);
    VNew(-1,-1, 0, result->vertices[3]);
    VNew( 1,-1, 0, result->vertices[4]);
    VNew( 1, 0, 0, result->vertices[5]);
    VNew( 0, 0, 0, result->vertices[6]);

    result->normals = New(Vector, 1);
    VNew(0, 0, 1, result->normals[0]);

    /* Finally, allocate the faces. */
    result->faces = New(Face, result->num_faces);
    for ( i = 0 ; i < result->num_faces - 1 ; i++ )
    {
        result->faces[i].num_vertices = 3;
        result->faces[i].vertices = New(int, 3);
        result->faces[i].vertices[0] = 0;
        result->faces[i].vertices[1] = i + 1;
        result->faces[i].vertices[2] = i;
        result->faces[i].normals = New(int, 3);
        result->faces[i].normals[0] = 0;
        result->faces[i].normals[1] = 0;
        result->faces[i].normals[2] = 0;
        result->faces[i].face_attribs = NULL;
    }
    result->faces[0].vertices[2] = 5;
    result->faces[5].num_vertices = 5;
    result->faces[5].vertices = New(int, 5);
    result->faces[5].normals = New(int, 5);
    for ( i = 0 ; i < 5 ; i++ )
    {
        result->faces[5].vertices[i] = i + 1;
        result->faces[5].normals[i] = 0;
    }
    result->faces[5].face_attribs = NULL;

    VNew( M_SQRT2,       0, M_SQRT2, result->faces[0].normal);
    VNew(       0, M_SQRT2, M_SQRT2, result->faces[1].normal);
    VNew(-M_SQRT2,       0, M_SQRT2, result->faces[2].normal);
    VNew(       0,-M_SQRT2, M_SQRT2, result->faces[3].normal);
    VNew( M_SQRT2,       0, M_SQRT2, result->faces[4].normal);
    VNew( 0, 0, -1, result->faces[5].normal);

    result->edges = NULL;
    result->attribs = NULL;
    result->num_attribs = 0;

    return result;
}


WireframePtr
Generic_Direction_Wireframe()
{
    WireframePtr    result = New(Wireframe, 1);
    int             i;

    /*  A direction wireframe has 4 faces and 6 vertices.   */
    result->num_faces = 4;
    result->num_vertices = 6;
    result->num_normals = 1;
    result->num_real_verts = 6;

    /* Allocate vertices first. */
    result->vertices = New(Vector, result->num_vertices);
    VNew( 0, 0, -1, result->vertices[0]);
    VNew( 0.5, 0.5, -1, result->vertices[1]);
    VNew(-0.5, 0.5, -1, result->vertices[2]);
    VNew(-0.5,-0.5, -1, result->vertices[3]);
    VNew( 0.5,-0.5, -1, result->vertices[4]);
    VNew( 0, 0, 0, result->vertices[5]);

    result->normals = New(Vector, 1);
    VNew(0, 0, 1, result->normals[0]);

    /* Finally, allocate the faces. */
    result->faces = New(Face, result->num_faces);
    for ( i = 0 ; i < result->num_faces ; i++ )
    {
        result->faces[i].num_vertices = 3;
        result->faces[i].vertices = New(int, 3);
        result->faces[i].normals = New(int, 3);
        result->faces[i].face_attribs = NULL;
        result->faces[i].vertices[0] = 5;
        result->faces[i].normals[0] = 0;
        result->faces[i].normals[1] = 0;
        result->faces[i].normals[2] = 0;
    }
    result->faces[0].vertices[1] = 1;
    result->faces[0].vertices[2] = 3;
    result->faces[1].vertices[1] = 3;
    result->faces[1].vertices[2] = 1;
    result->faces[2].vertices[1] = 2;
    result->faces[2].vertices[2] = 4;
    result->faces[3].vertices[1] = 4;
    result->faces[3].vertices[2] = 2;

    VNew( M_SQRT2,-M_SQRT2, 0, result->faces[0].normal);
    VNew(-M_SQRT2, M_SQRT2, 0, result->faces[1].normal);
    VNew( M_SQRT2, M_SQRT2, 0, result->faces[2].normal);
    VNew(-M_SQRT2,-M_SQRT2, 0, result->faces[3].normal);

    result->edges = NULL;
    result->attribs = NULL;
    result->num_attribs = 0;

    return result;
}


WireframePtr
Generic_Triangle_Wireframe()
{
    WireframePtr    result = New(Wireframe, 1);

    /*  A triangle wireframe has 1 face and 4 vertices. */
    /*  There are no additional reference vertices. */
    result->num_faces = 1;
    result->num_vertices = 4;
    result->num_normals = 1;
    result->num_real_verts = 3;


    /* Allocate vertices first. */
    result->vertices = New(Vector, result->num_vertices);
    VNew(0, 0, 0, result->vertices[0]);
    VNew(0.5, M_SQRT1_2, 0, result->vertices[1]);
    VNew(1, 0, 0, result->vertices[2]);
    VNew(0, 0, 0, result->vertices[3]);

    result->normals = New(Vector, result->num_normals);
    VNew(0, 0, 1, result->normals[0]);

    /* Allocate the face. */
    result->faces = New(Face, result->num_faces);
    result->faces[0].num_vertices = 3;
    result->faces[0].vertices = New(int, 3);
    result->faces[0].normals = New(int, 3);
    result->faces[0].vertices[0] = 0;
    result->faces[0].vertices[1] = 1;
    result->faces[0].vertices[2] = 2;
    result->faces[0].normals[0] = 0;
    result->faces[0].normals[1] = 0;
    result->faces[0].normals[2] = 0;
    VNew(0, 0, 1, result->faces[0].normal);
    result->faces[0].face_attribs = NULL;

    result->edges = NULL;
    result->num_attribs = 0;
    result->attribs = NULL;

    return result;
}


WireframePtr
Generic_Bezier_Wireframe()
{
    return Dense_Bezier_Wireframe(0);
}
