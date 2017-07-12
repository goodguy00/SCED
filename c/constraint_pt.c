/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/constraint_pt.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: constraint_pt.c,v $
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
**  constraint_pt.c : Constraint satisfaction functions.
*/


#include <math.h>
#include <sced.h>
#include <constraint.h>

/* A type for the constraint satisfied functions. */
typedef int (*ConstraintSatisfiedFunction)(VectorPtr, VectorPtr, VectorPtr,
                                           double*, double*);


static int  Point_On_Null(VectorPtr, VectorPtr, VectorPtr, double*, double*);
static int  Point_On_Plane(VectorPtr, VectorPtr, VectorPtr, double*, double*);
static int  Point_On_Line(VectorPtr, VectorPtr, VectorPtr, double*, double*);
static int  Point_On_Point(VectorPtr, VectorPtr, VectorPtr, double*, double*);
static int  Point_On_Sphere(VectorPtr, VectorPtr, VectorPtr, double*, double*);
static int  Point_On_Circle(VectorPtr, VectorPtr, VectorPtr, double*, double*);

/* Functions to determine whether a point satisfies a constraint. The
** constraint type is used to index the array.
*/
static ConstraintSatisfiedFunction  satisfied_functions[7] =
    { Point_On_Null, Point_On_Plane, Point_On_Line, Point_On_Point,
      Point_On_Sphere, Point_On_Circle, Point_On_Null };

/* A type for the nearest point functions. */
typedef Vector  (*ConstraintPointFunction)(VectorPtr, VectorPtr, VectorPtr,
                                           double*, double*);

static Vector
    Closest_Null_Point(VectorPtr, VectorPtr, VectorPtr, double*, double*);
static Vector
    Closest_Plane_Point(VectorPtr, VectorPtr, VectorPtr, double*, double*);
static Vector
    Closest_Line_Point(VectorPtr, VectorPtr, VectorPtr, double*, double*);
static Vector
    Closest_Point_Point(VectorPtr, VectorPtr, VectorPtr, double*, double*);
static Vector
    Closest_Circle_Point(VectorPtr, VectorPtr, VectorPtr, double*, double*);

/* Functions to determine the closest point that satisfies a constraint. */
static ConstraintPointFunction  constraint_point[7] =
    { Closest_Null_Point, Closest_Plane_Point, Closest_Line_Point,
      Closest_Point_Point, Closest_Sphere_Point, Closest_Circle_Point,
      Closest_Null_Point };


static int
Point_On_Null(VectorPtr pt, VectorPtr v1, VectorPtr v2, double *d1,
              double *d2)
{
    return 1;
}


static int
Point_On_Point(VectorPtr pt, VectorPtr v1, VectorPtr con_pt, double *d1,
               double *d2)
{
    return ( DEqual(pt->x, con_pt->x) &&
             DEqual(pt->y, con_pt->y) &&
             DEqual(pt->z, con_pt->z) );
}


/*  int
**  Point_On_Plane(...)
**  Returns TRUE if the point pt lies in the plane defined by plane_pt and
**  normal.
*/
static int
Point_On_Plane(VectorPtr pt, VectorPtr normal, VectorPtr v2,
               double *plane_pt, double *d2)
{
    double  temp1 = VDot(*normal, *pt);

    return DEqual(temp1, *plane_pt);
}


/*  int
**  Point_On_Line(...)
**  Returns TRUE if the point pt lies in the line defined by line_pt and dir.
*/
static int
Point_On_Line(VectorPtr pt, VectorPtr dir, VectorPtr line_pt, double *d1,
              double *d2)
{
    double  alpha;
    double  temp1, temp2;

    if ( ! IsZero(dir->x) )
    {
        alpha = ( pt->x - line_pt->x ) / dir->x;
        temp1 = dir->y * alpha + line_pt->y;
        temp2 = dir->z * alpha + line_pt->z;
        return ( DEqual(temp1, pt->y) && DEqual(temp2, pt->z) );
    }
    else if ( ! IsZero(dir->y) )
    {
        alpha = ( pt->y - line_pt->y ) / dir->y;
        temp1 = dir->z * alpha + line_pt->z;
        return ( DEqual(line_pt->x, pt->x) && DEqual(temp1, pt->z) );
    }
    else if ( ! IsZero(dir->z) )
        return ( DEqual(line_pt->x, pt->x) && DEqual(line_pt->y, pt->y) );

    return 0;

}

static int
Point_On_Sphere(VectorPtr pt, VectorPtr v1, VectorPtr center, double *d2,
                double *radius)
{
    double  pt_rad;
    double  rad_sq = (*radius) * (*radius);
    Vector  temp_v;

    VSub(*pt, *center, temp_v);
    pt_rad = VDot(temp_v, temp_v);

    return DEqual(pt_rad, rad_sq);
}


static int
Point_On_Circle(VectorPtr pt, VectorPtr norm, VectorPtr center, double *val,
                double *radius)
{
    return Point_On_Plane(pt, norm, center, val, radius) &&
           Point_On_Sphere(pt, norm, center, val, radius);
}


/*  int
**  Point_Satisfies_Constraint(VectorPtr pt, ResultantPtr res, ConstraintPtr con)
**  Returns TRUE if the point pt satisfies the constraint. Only one of
**  con or res need to be specified. The pt is checked against whichever is
**  given.
*/
int
Point_Satisfies_Constraint(VectorPtr pt, ResultantPtr res, ConstraintPtr con)
{
    ConstraintPtr con1, con2;

    if ( ! res )
    {
        con1 = con;
        con2 = NULL;
    }
    else
    {
        con1 = &(res->feature_1);
        if ( res->feature_2.c_type != null_feature )
            con2 = &(res->feature_2);
        else
            con2 = NULL;
    }

    if ( con2 )
    {
        /* Return preferred value. */
        if ( satisfied_functions[con1->c_type](pt, &(con1->c_vector),
                                               &(con1->c_point),
                                               &(con1->c_value),
                                               &(con1->c_radius)) )
            return 1;
        else if ( satisfied_functions[con2->c_type](pt, &(con2->c_vector),
                                                    &(con2->c_point),
                                                    &(con2->c_value),
                                                    &(con2->c_radius)) )
            return 2;
        else
            return 0;
    }
    else if ( satisfied_functions[con1->c_type](pt, &(con1->c_vector),
                                                &(con1->c_point),
                                                &(con1->c_value),
                                                &(con1->c_radius)) )
            return 1;
    else
        return 0;
}


double
Distance_Point_To_Plane(VectorPtr norm, VectorPtr plane_pt, VectorPtr pt)
{
    Vector  temp_v;

    VSub(*pt, *plane_pt, temp_v);
    return VDot(temp_v, *norm);
}


static Vector
Closest_Null_Point(VectorPtr pt, VectorPtr v1, VectorPtr v2, double *d1,
                   double *d2)
{
    return *pt;
}


static Vector
Closest_Plane_Point(VectorPtr pt, VectorPtr norm, VectorPtr plane_pt,
                    double *d1, double *d2)
{
    Vector  result;

    /* Need to find the perpendicular from the point to the plane. */
    double  distance = Distance_Point_To_Plane(norm, plane_pt, pt);
    VScalarMul(*norm, distance, result);
    VSub(*pt, result, result);

    return result;
}


static Vector
Closest_Line_Point(VectorPtr pt, VectorPtr dir, VectorPtr line_pt, double *d1,
                   double *d2)
{
    Vector  result;
    double  temp_d;

    /* Need to find a perpendicular from the point to the line. */
    VSub(*pt, *line_pt, result);
    temp_d = VDot(result, *dir) / VDot(*dir, *dir);
    VScalarMul(*dir, temp_d, result);
    VAdd(result, *line_pt, result);

    return result;
}

static Vector
Closest_Point_Point(VectorPtr pt, VectorPtr v1, VectorPtr point, double *d1,
                    double *d2)
{
    return *point;
}


Vector
Closest_Sphere_Point(VectorPtr pt, VectorPtr v1, VectorPtr center, double *d1,
                     double *radius)
{
    Vector  result;
    Vector  join_line;
    double  temp_v;

    /* Find the point on the sphere that lies on the line joining the
    ** center to the point.
    */
    VSub(*pt, *center, join_line);

    if ( VZero(join_line) )
        /* Choose at random. */
        VNew(1, 0, 0, join_line);
    else
        VUnit(join_line, temp_v, join_line);
    VScalarMul(join_line, *radius, join_line);
    VAdd(join_line, *center, result);

    return result;
}


static Vector
Closest_Circle_Point(VectorPtr pt, VectorPtr normal, VectorPtr center,
                     double *pl_pt, double *radius)
{
    Vector  plane_pt = Closest_Plane_Point(pt, normal, center, pl_pt, radius);
    Vector  join_line;
    double  temp_v;
    Vector  result;

    VSub(plane_pt, *center, join_line);
    if ( VZero(join_line) )
    {
        Vector  tmp_v;
        VNew(1, 0, 0, tmp_v);
        VCross(tmp_v, *normal, join_line);
        if ( VZero(join_line) )
        {
            VNew(0, 1, 0, tmp_v);
            VCross(tmp_v, *normal, join_line);
        }
    }
    VUnit(join_line, temp_v, join_line);
    VScalarMul(join_line, *radius, join_line);
    VAdd(join_line, *center, result);

    return result;
}


/*  Returns the displacement required to make the pt satisfy the constraint.
*/
Vector
Find_Required_Motion(VectorPtr pt, ResultantPtr con, Boolean prefer,
                     int *preferred)
{
    Vector  result;

    if ( con->feature_2.c_type == null_feature )
        result = constraint_point[con->feature_1.c_type]
                    (pt, &(con->feature_1.c_vector), &(con->feature_1.c_point),
                     &(con->feature_1.c_value), &(con->feature_1.c_radius));
    else if ( prefer )
    {
        if ( *preferred == 1 )
            result = constraint_point[con->feature_1.c_type]
                    (pt, &(con->feature_1.c_vector), &(con->feature_1.c_point),
                     &(con->feature_1.c_value), &(con->feature_1.c_radius));
        else
            result = constraint_point[con->feature_2.c_type]
                    (pt, &(con->feature_2.c_vector), &(con->feature_2.c_point),
                     &(con->feature_2.c_value), &(con->feature_2.c_radius));
    }
    else
    {
        Vector  v1, v2;
        double  d1, d2;

        VSub(con->feature_1.c_point, *pt, v1);
        VSub(con->feature_2.c_point, *pt, v2);
        d1 = VDot(v1, v1);
        d2 = VDot(v2, v2);
        if ( d1 <= d2 )
        {
            result = constraint_point[con->feature_1.c_type]
                    (pt, &(con->feature_1.c_vector), &(con->feature_1.c_point),
                     &(con->feature_1.c_value), &(con->feature_1.c_radius));
            if ( preferred )
                *preferred = 1;
        }
        else
        {
            result = constraint_point[con->feature_2.c_type]
                    (pt, &(con->feature_2.c_vector), &(con->feature_2.c_point),
                     &(con->feature_2.c_value), &(con->feature_2.c_radius));
            if ( preferred )
                *preferred = 2;
        }
    }

    VSub(result, *pt, result);
    return result;

}


/* A procedure that calls another procedure (func) for each specifying point in
** a constraint (feat). The other arguments are passed to the called function.
*/
void
Constraint_Manipulate_Specs(ConstraintPtr constr, ObjectInstancePtr obj,
                            void *ptr, void *ptr2, int num, SpecFunction func)
{
    int i;
    for ( i = 0 ; i < constr->c_num_specs ; i++ )
        func(constr->c_specs + i, obj, ptr, ptr2, num);
}

