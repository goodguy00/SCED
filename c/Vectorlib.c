/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/Vectorlib.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: Vectorlib.c,v $
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

/****************************************************************/
/*                                                              */
/*  Vector functions.                                           */
/*  Matrix manipulation functions.                              */
/*  Written by Stephen Chenney. Dec 1993.                       */
/*                                                              */
/****************************************************************/
/*                                                              */
/*                                                              */
/****************************************************************/

#include <stdio.h>  /* For sscanf. */
#include <math.h>
#include <Vector.h>
#include <Vector4.h>


/* A small number for floating point comparisons. */
/* A floating point == 0.0 function. */
#define IsZero(f)   (((f) < VECT_EPSILON) && ((f) > -VECT_EPSILON))


int
VRead(char *s, Vector* v)
/****************************************************************/
/*                                                              */
/*  Reads a vector from a file. (may be stdin).                 */
/*  Returns 0 on failure, 1 otherwise.                          */
/*                                                              */
/****************************************************************/
{
    if ((sscanf(s, "%lf %lf %lf", &((*v).x), &((*v).y), &((*v).z))) != 3)
        return 0;
    else
        return 1;
}

int
V4Read(char *s, Vector4 *v)
{
    if ( sscanf(s, "%lf %lf %lf %lf",
                &((*v).x), &((*v).y), &((*v).z), &((*v).w)) != 4 )
        return 0;
    else
        return 1;
}


Matrix
MMMul(Matrix *m1, Matrix *m2)
/****************************************************************/
/*                                                              */
/*  Multiplies 2 matrices: r = m1m2.                            */
/*                                                              */
/****************************************************************/
{
    Matrix r;
    Matrix  col;

    MTrans(*m2, col);

    r.x.x = VDot(m1->x, col.x);
    r.x.y = VDot(m1->x, col.y);
    r.x.z = VDot(m1->x, col.z);
    r.y.x = VDot(m1->y, col.x);
    r.y.y = VDot(m1->y, col.y);
    r.y.z = VDot(m1->y, col.z);
    r.z.x = VDot(m1->z, col.x);
    r.z.y = VDot(m1->z, col.y);
    r.z.z = VDot(m1->z, col.z);

    return r;
}


Matrix4
M4M4Mul(Matrix4 *m1, Matrix4 *m2)
{
    Matrix4 r;
    Matrix4 col;

    M4Trans(*m2, col);

    r.x.x = V4Dot(m1->x, col.x);
    r.x.y = V4Dot(m1->x, col.y);
    r.x.z = V4Dot(m1->x, col.z);
    r.x.w = V4Dot(m1->x, col.w);
    r.y.x = V4Dot(m1->y, col.x);
    r.y.y = V4Dot(m1->y, col.y);
    r.y.z = V4Dot(m1->y, col.z);
    r.y.w = V4Dot(m1->y, col.w);
    r.z.x = V4Dot(m1->z, col.x);
    r.z.y = V4Dot(m1->z, col.y);
    r.z.z = V4Dot(m1->z, col.z);
    r.z.w = V4Dot(m1->z, col.w);
    r.w.x = V4Dot(m1->w, col.x);
    r.w.y = V4Dot(m1->w, col.y);
    r.w.z = V4Dot(m1->w, col.z);
    r.w.w = V4Dot(m1->w, col.w);

    return r;
}


Matrix
MInvert(Matrix *m)
/****************************************************************/
/*                                                              */
/*  Inverts the matrix m and puts it in r.                      */
/*  If the matrix m is singular, returns the zero matrix.       */
/*                                                              */
/****************************************************************/
{
    Matrix r;

    /* This does it by the "formula" method.*/
    /* a[i][j] = det(adjoint[i][j])/det(b)  */

    double temp;    /* for the determinant. */
    double a11, a22, a33; /* Various adjoint determinants. */

    a11 = m->y.y * m->z.z - m->y.z * m->z.y;
    a22 = m->x.y * m->z.z - m->x.z * m->z.y;
    a33 = m->x.y * m->y.z - m->x.z * m->y.y;
    temp =  m->x.x * a11 - m->y.x * a22 + m->z.x * a33;
    
    /* Test for singularity. */
    if (IsZero(temp))
    {
        r.x.x=r.x.y=r.x.z=r.y.x=r.y.y=r.y.z=r.z.x=r.z.y=r.z.z=0;
        return r;
    }

    r.x.x = a11 / temp;
    r.x.y = -a22 / temp;
    r.x.z = a33 / temp;
    r.y.x = (m->y.z * m->z.x - m->y.x * m->z.z) / temp;
    r.y.y = (m->x.x * m->z.z - m->x.z * m->z.x) / temp;
    r.y.z = (m->x.z * m->y.x - m->x.x * m->y.z) / temp;
    r.z.x = (m->y.x * m->z.y - m->y.y * m->z.x) / temp;
    r.z.y = (m->x.y * m->z.x - m->x.x * m->z.y) / temp;
    r.z.z = (m->x.x * m->y.y - m->x.y * m->y.x) / temp;

    return r;
}


/*
**  The following code comes from the Graphics Gems II code library.
**  inverse.c to be precise.
**
**  NOTE: It only works for Affine matrices.
*/
Matrix4
M4Invert(Matrix4 *m)
{
    Matrix4 r;
    register  double    det_1;
              double    pos, neg, temp;

#define ACCUMULATE    \
    if ( temp >= 0.0 )  \
        pos += temp;  \
    else              \
        neg += temp;

    /*
     * Calculate the determinant of submatrix A and determine if the
     * the matrix is singular as limited by the double precision
     * floating-point data representation.
     */
    pos = neg = 0.0;
    temp =  m->x.x * m->y.y * m->z.z;
    ACCUMULATE
    temp =  m->x.y * m->y.z * m->z.x;
    ACCUMULATE
    temp =  m->x.z * m->y.x * m->z.y;
    ACCUMULATE
    temp = -m->x.z * m->y.y * m->z.x;
    ACCUMULATE
    temp = -m->x.y * m->y.x * m->z.z;
    ACCUMULATE
    temp = -m->x.x * m->y.z * m->z.y;
    ACCUMULATE
    det_1 = pos + neg;

    /* Is the submatrix A singular? */
    if ( ( det_1 == 0.0 ) || ( fabs( det_1 / (pos - neg) ) < VECT_EPSILON) )
    {
        /* Matrix M has no inverse */
        r.x.x = r.x.y = r.x.z = r.x.w = r.y.x = r.y.y = r.y.z = r.y.w =
        r.z.x = r.z.y = r.z.z = r.z.w = r.w.x = r.w.y = r.w.z = r.w.w = 0.0;
        return r;
    }

    /* Calculate inverse(A) = adj(A) / det(A) */
    det_1 = 1.0 / det_1;
    r.x.x =   ( m->y.y * m->z.z - m->y.z * m->z.y ) * det_1;
    r.y.x = - ( m->y.x * m->z.z - m->y.z * m->z.x ) * det_1;
    r.z.x =   ( m->y.x * m->z.y - m->y.y * m->z.x ) * det_1;
    r.x.y = - ( m->x.y * m->z.z - m->x.z * m->z.y ) * det_1;
    r.y.y =   ( m->x.x * m->z.z - m->x.z * m->z.x ) * det_1;
    r.z.y = - ( m->x.x * m->z.y - m->x.y * m->z.x ) * det_1;
    r.x.z =   ( m->x.y * m->y.z - m->x.z * m->y.y ) * det_1;
    r.y.z = - ( m->x.x * m->y.z - m->x.z * m->y.x ) * det_1;
    r.z.z =   ( m->x.x * m->y.y - m->x.y * m->y.x ) * det_1;

    /* Calculate -C * inverse(A) */
    r.w.x = - ( m->w.x * r.x.x + m->w.y * r.y.x + m->w.z * r.z.x );
    r.w.y = - ( m->w.x * r.x.y + m->w.y * r.y.y + m->w.z * r.z.y );
    r.w.z = - ( m->w.x * r.x.z + m->w.y * r.y.z + m->w.z * r.z.z );

    /* Fill in last column */
    r.x.w = r.y.w = r.z.w = 0.0;
    r.w.w = 1.0;

    return r;
}








