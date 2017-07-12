/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/constraint.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: constraint.c,v $
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
**  constraint.c : Constraint solver and other stuff.
*/

#include <math.h>
#include <sced.h>
#include <constraint.h>

/* A type for the constraint combination functions. */
typedef void
    (*CombineConstraintFunction)(ConstraintPtr, ConstraintPtr, ResultantPtr);


static void Combine_Constraints(ConstraintPtr, ConstraintPtr, ResultantPtr);

static void
    Combine_Plane_Plane_Constraints(ConstraintPtr, ConstraintPtr, ResultantPtr);
static void
    Combine_Plane_Line_Constraints(ConstraintPtr, ConstraintPtr, ResultantPtr);
static void
    Combine_Plane_Point_Constraints(ConstraintPtr, ConstraintPtr, ResultantPtr);
static void
    Combine_Plane_Sphere_Constraints(ConstraintPtr, ConstraintPtr, ResultantPtr);
static void
    Combine_Plane_Circle_Constraints(ConstraintPtr, ConstraintPtr, ResultantPtr);
static void
    Combine_Line_Line_Constraints(ConstraintPtr, ConstraintPtr, ResultantPtr);
static void
    Combine_Line_Point_Constraints(ConstraintPtr, ConstraintPtr, ResultantPtr);
static void
    Combine_Line_Sphere_Constraints(ConstraintPtr, ConstraintPtr, ResultantPtr);
static void
    Combine_Line_Circle_Constraints(ConstraintPtr, ConstraintPtr, ResultantPtr);
static void
    Combine_Point_Any_Constraints(ConstraintPtr, ConstraintPtr, ResultantPtr);
static void
    Combine_Sphere_Sphere_Constraints(ConstraintPtr, ConstraintPtr, ResultantPtr);
static void
    Combine_Sphere_Circle_Constraints(ConstraintPtr, ConstraintPtr, ResultantPtr);
static void
    Combine_Circle_Circle_Constraints(ConstraintPtr, ConstraintPtr, ResultantPtr);
static void
    Combine_Null_Constraints(ConstraintPtr, ConstraintPtr, ResultantPtr);
static void
    Combine_Inconsistent_Constraints(ConstraintPtr, ConstraintPtr, ResultantPtr);

/* Function to call given the 2 constraint types. The constraint types are
** used as indices into the array to determine which function to call.
*/
static CombineConstraintFunction    combination_functions[7][7] = {
    { Combine_Null_Constraints, Combine_Null_Constraints,
      Combine_Null_Constraints, Combine_Null_Constraints,
      Combine_Null_Constraints, Combine_Null_Constraints,
      Combine_Inconsistent_Constraints },
    { 0, Combine_Plane_Plane_Constraints, Combine_Plane_Line_Constraints,
      Combine_Plane_Point_Constraints, Combine_Plane_Sphere_Constraints,
      Combine_Plane_Circle_Constraints, Combine_Inconsistent_Constraints },
    { 0, 0, Combine_Line_Line_Constraints, Combine_Line_Point_Constraints,
      Combine_Line_Sphere_Constraints, Combine_Line_Circle_Constraints,
      Combine_Inconsistent_Constraints },
    { 0, 0, 0, Combine_Point_Any_Constraints,
      Combine_Point_Any_Constraints, Combine_Point_Any_Constraints,
      Combine_Inconsistent_Constraints },
    { 0, 0, 0, 0, Combine_Sphere_Sphere_Constraints,
      Combine_Sphere_Circle_Constraints, Combine_Inconsistent_Constraints },  
    { 0, 0, 0, 0, 0, Combine_Circle_Circle_Constraints,
      Combine_Inconsistent_Constraints },
    { Combine_Inconsistent_Constraints } };


/*  void
**  Constraint_Solve_System(ConstraintPtr *cons, int num, ResultantPtr result)
**  Solves those constraints that are active and puts the
**  resulting constraint in result.
*/
static void
Constraint_Combine(ConstraintPtr cons, ResultantPtr result)
{
    Resultant   result1, result2;

    if ( cons->c_status || cons->c_forced )
    {
        if ( result->feature_1.c_type == null_feature )
        {
            result->feature_1 = *cons;
            return;
        }
        else
        {
            Combine_Constraints(cons, &(result->feature_1), &result1);
            if ( result->feature_2.c_type == null_feature )
            {
                result2.feature_1.c_type = null_feature;
                result2.feature_2.c_type = null_feature;
            }
            else
                Combine_Constraints(cons, &(result->feature_2),&result2);
        }

        /* Now combine result1 and result2. */
        /* We can make use of the fact that there can only ever be
        ** two features. So either both are in result1 or there is one
        ** in result1 and one in result2.
        */
        *result = result1;
        if ( result2.feature_1.c_type != null_feature )
        {
            if ( result1.feature_1.c_type == null_feature ||
                 result1.feature_1.c_type == inconsistent_feature )
                result->feature_1 = result2.feature_1;
            else if ( result2.feature_1.c_type != inconsistent_feature )
                result->feature_2 = result2.feature_1;
        }
        if ( result2.feature_2.c_type != null_feature &&
             result2.feature_2.c_type != inconsistent_feature )
            result->feature_2 = result2.feature_2;
    }
}


void
Constraint_Solve_System(ObjectInstancePtr obj, ConstraintPtr cons, int num,
                        ResultantPtr result)
{
    int i;

    result->feature_1.c_type = null_feature;
    result->feature_2.c_type = null_feature;

    for ( i = 0 ; i < num ; i++ )
    {
        Constraint_Combine(cons + i, result);

        if ( result->feature_1.c_type == inconsistent_feature )
            return;
    }
}

void
Constraint_Solve_System_Ptrs(ConstraintPtr *cons, int num, ResultantPtr result)
{
    int i;

    result->feature_1.c_type = null_feature;
    result->feature_2.c_type = null_feature;

    for ( i = 0 ; i < num ; i++ )
    {
        Constraint_Combine(cons[i], result);

        if ( result->feature_1.c_type == inconsistent_feature )
            return;
    }
}

/*  void
**  Combine_Constraints(ConstraintPtr c1, ConstraintPtr c2, ResultantPtr result)
**  Combines the 2 constraints.  Basically just calls the appropriate
**  feature pair routine.
*/
static void
Combine_Constraints(ConstraintPtr c1, ConstraintPtr c2, ResultantPtr result)
{
    if ( c1->c_type <= c2->c_type )
        combination_functions[c1->c_type][c2->c_type](c1, c2, result);
    else
        combination_functions[c2->c_type][c1->c_type](c2, c1, result);
}


/*
**  The following Combine_????_????_Constraints functions do pretty much
**  what their names suggest.  They may return inconsistent_feature if required.
*/

static void
Combine_Null_Constraints(ConstraintPtr c1, ConstraintPtr c2, ResultantPtr result)
{
    result->feature_1.c_type = null_feature;
    result->feature_2.c_type = null_feature;
}


static void
Combine_Inconsistent_Constraints(ConstraintPtr c1, ConstraintPtr c2,
                                 ResultantPtr result)
{
    result->feature_1.c_type = inconsistent_feature;
    result->feature_2.c_type = null_feature;
}



static void
Combine_Plane_Plane_Constraints(ConstraintPtr c1, ConstraintPtr c2,
                                ResultantPtr result)
{
    Vector  cross;

    result->feature_2.c_type = null_feature;

    /* The result is inconsistent if the planes are parallel and not
    ** coincident, a plane if they are parallel and coincident and 
    ** their line of intersection otherwise.
    */

    VCross(c1->c_vector, c2->c_vector, cross);
    if ( VZero(cross) )
    {
        /* The planes are parallel. */
        if ( Point_Satisfies_Constraint(&(c2->c_point), NULL, c1) )
        {
            result->feature_1 = *c1;
            return;
        }

        result->feature_1.c_type = inconsistent_feature;
        return;
    }

    result->feature_1.c_type = line_feature;
    result->feature_1.c_vector = cross;

    if ( ! IsZero(cross.z) )
    {
        result->feature_1.c_point.x = ( c2->c_vector.y * c1->c_value -
                                        c1->c_vector.y * c2->c_value ) /
                                        cross.z;
        result->feature_1.c_point.y = ( c1->c_vector.x * c2->c_value -
                                        c2->c_vector.x * c1->c_value ) /
                                        cross.z;
        result->feature_1.c_point.z = 0;
    }
    else if ( ! IsZero(cross.y) )
    {
        result->feature_1.c_point.x = ( c1->c_vector.z * c2->c_value -
                                        c2->c_vector.z * c1->c_value ) /
                                        cross.y;
        result->feature_1.c_point.y = 0;
        result->feature_1.c_point.z = ( c2->c_vector.x * c1->c_value -
                                        c1->c_vector.x * c2->c_value ) /
                                        cross.y;
    }
    else
    {
        result->feature_1.c_point.x = 0;
        result->feature_1.c_point.y = ( c2->c_vector.z * c1->c_value -
                                        c1->c_vector.z * c2->c_value ) /
                                        cross.x;
        result->feature_1.c_point.z = ( c1->c_vector.y * c2->c_value -
                                        c2->c_vector.y * c1->c_value ) /
                                        cross.x;
    }
}


static void
Combine_Plane_Line_Constraints(ConstraintPtr c1, ConstraintPtr c2,
                               ResultantPtr result)
{
    double      dot;
    double      alpha;
    Vector      temp_v;

    result->feature_2.c_type = null_feature;

    /* The result is inconsistent if the line is parallel to but not in the
    ** plane, the line if it is parallel and in the plane, or the point
    ** of intersection otherwise.
    */

    dot = VDot( c1->c_vector, c2->c_vector );

    if ( IsZero(dot) )
    {
        /* The line and plane are parallel. */
        if ( Point_Satisfies_Constraint(&(c2->c_point), NULL, c1) )
            /* The line lies in the plane. */
            result->feature_1 = *c2;
        else
            result->feature_1.c_type = inconsistent_feature;
        return;

    }

    /* Find the intersection point. */
    result->feature_1.c_type = point_feature;
    alpha = ( VDot(c1->c_vector, c1->c_point) -
              VDot(c1->c_vector, c2->c_point) ) / dot;
    VScalarMul(c2->c_vector, alpha, temp_v);
    VAdd(temp_v, c2->c_point, result->feature_1.c_point);
}


static void
Combine_Plane_Point_Constraints(ConstraintPtr c1, ConstraintPtr c2,
                                ResultantPtr result)
{
    result->feature_2.c_type = null_feature;

    /* The result is either inconsistent if the point is not in the plane,
    ** or the point itself otherwise.
    */
    if ( Point_Satisfies_Constraint(&(c2->c_point), NULL, c1) )
        result->feature_1 = *c2;
    else
        result->feature_1.c_type = inconsistent_feature;
}


static void
Combine_Plane_Sphere_Constraints(ConstraintPtr c1, ConstraintPtr c2,
                                 ResultantPtr result)
{
    double  dist;
    double  abs_dist;

    result->feature_2.c_type = null_feature;

    dist =
      Distance_Point_To_Plane(&(c1->c_vector), &(c1->c_point), &(c2->c_point));
    abs_dist = fabs(dist);

    if ( DEqual(abs_dist, c2->c_radius) )
    {
        result->feature_1.c_type = point_feature;
        VScalarMul(c1->c_vector, dist, result->feature_1.c_point);
        VSub(c2->c_point, result->feature_1.c_point, result->feature_1.c_point);
        return;
    }
    else if ( abs_dist > c2->c_radius )
    {
        result->feature_1.c_type = inconsistent_feature;
        return;
    }
    else
    {
        result->feature_1.c_type = circle_feature;
        result->feature_1.c_vector = c1->c_vector;
        VScalarMul(c1->c_vector, dist, result->feature_1.c_point);
        VSub(c2->c_point, result->feature_1.c_point, result->feature_1.c_point);
        result->feature_1.c_value = VDot(result->feature_1.c_vector,
                                         result->feature_1.c_point);
        result->feature_1.c_radius = sqrt(c2->c_radius * c2->c_radius -
                                          dist * dist);
        return;
    }
}


static void
Combine_Plane_Circle_Constraints(ConstraintPtr c1, ConstraintPtr c2,
                                 ResultantPtr result)
{
    Resultant   plane_plane;

    c2->c_type = plane_feature;
    Combine_Plane_Plane_Constraints(c1, c2, &plane_plane);
    c2->c_type = circle_feature;

    /* plane_plane holds the intersection of the plane and the circle plane. */
    if ( plane_plane.feature_1.c_type == plane_feature )
    {
        result->feature_1 = *c2;
        result->feature_2.c_type = null_feature;
        return;
    }

    if ( plane_plane.feature_1.c_type == inconsistent_feature )
    {
        result->feature_1.c_type = inconsistent_feature;
        result->feature_2.c_type = null_feature;
        return;
    }

    /* Planes intersect in a line. */
    /* Intersect line with circle to get result. */
    Combine_Line_Circle_Constraints(&(plane_plane.feature_1), c2, result);
}


static void
Combine_Line_Line_Constraints(ConstraintPtr c1, ConstraintPtr c2, ResultantPtr result)
{
    Vector      cross;
    double      param1;
    Vector      temp_v1, temp_v2, temp_v3, temp_v4;
    Vector      point1;

    result->feature_2.c_type = null_feature;

    /* The result is inconsistent if the lines don't intersect, a point
    ** otherwise (their point of intersection).
    */

    VCross(c1->c_vector, c2->c_vector, cross);

    if ( VZero(cross) )
    {
        /* The lines are parallel. */
        if ( Point_Satisfies_Constraint(&(c2->c_point), NULL, c1) )
            result->feature_1 = *c1;
        else
            result->feature_1.c_type = inconsistent_feature;
        return;
    }

    /* They still may or may not intersect. */

    VSub(c1->c_point, c2->c_point, temp_v1);
    VCross(c2->c_vector, temp_v1, temp_v2);
    VCross(c1->c_vector, temp_v1, temp_v3);

    /* temp_v3 and temp_v2 are both perp to temp_v1. */
    /* If temp_v2 is 0, then the lines intersect at c1->c_point1. */
    /* If temp_v3 is 0, then the lines intersect at c1->c_point2. */
    /* If temp_v2 and temp_v3 are parallel, the lines intersect somewhere. */
    /* Otherwise they don't. */
    if ( VZero(temp_v2) )
    {
        result->feature_1.c_type = point_feature;
        result->feature_1.c_point = c1->c_point;
        return;
    }
    else if ( VZero(temp_v2) )
    {
        result->feature_1.c_type = point_feature;
        result->feature_1.c_point = c2->c_point;
        return;
    }

    VCross(temp_v2, temp_v3, temp_v4);
    if ( ! VZero(temp_v4) )
    {
        result->feature_1.c_type = inconsistent_feature;
        return;
    }


    /* Get the parameter for the point of intersection. */
    if ( ! IsZero(cross.z) )
        param1 = temp_v2.z / cross.z;
    else if ( ! IsZero(cross.y) )
        param1 = temp_v2.y / cross.y;
    else
        param1 = temp_v2.x / cross.x;

    VScalarMul(c1->c_vector, param1, temp_v1);
    VAdd(temp_v1, c1->c_point, point1);

    result->feature_1.c_type = point_feature;
    result->feature_1.c_point = point1;
}


static void
Combine_Line_Point_Constraints(ConstraintPtr c1, ConstraintPtr c2,
                               ResultantPtr result)
{
    /* The result is inconsistent if the point is not on the line,
    ** the point otherwise.
    */

    result->feature_2.c_type = null_feature;

    if ( Point_Satisfies_Constraint(&(c2->c_point), NULL, c1) )
        result->feature_1 = *c2;
    else
        result->feature_1.c_type = inconsistent_feature;

    return;
}


static void
Combine_Line_Sphere_Constraints(ConstraintPtr c1, ConstraintPtr c2,
                                 ResultantPtr result)
{
    Vector  temp_v;
    double  a, b, c;
    double  discrim;
    double  k;

    VSub(c1->c_point, c2->c_point, temp_v);
    a = VDot(c1->c_vector, c1->c_vector);
    b = VDot(c1->c_vector, temp_v);
    c = VDot(temp_v, temp_v) - c2->c_radius * c2->c_radius;

    discrim = b * b - a * c;

    if ( IsZero(a) )
    {
        result->feature_1.c_type = inconsistent_feature;
        result->feature_2.c_type = null_feature;
        return;
    }

    if ( IsZero(discrim) )
    {
        result->feature_1.c_type = point_feature;
        k = - b / a;
        VScalarMul(c1->c_vector, k, temp_v);
        VAdd(temp_v, c1->c_point, result->feature_1.c_point);

        result->feature_2.c_type = null_feature;
    }
    else if ( discrim < 0 )
    {
        result->feature_1.c_type = inconsistent_feature;
        result->feature_2.c_type = null_feature;
    }
    else
    {
        result->feature_1.c_type = point_feature;
        k = ( - b + sqrt(discrim) ) / a;
        VScalarMul(c1->c_vector, k, temp_v);
        VAdd(temp_v, c1->c_point, result->feature_1.c_point);

        result->feature_2.c_type = point_feature;
        k = ( - b - sqrt(discrim) ) / a;
        VScalarMul(c1->c_vector, k, temp_v);
        VAdd(temp_v, c1->c_point, result->feature_2.c_point);
    }
}


static void
Combine_Line_Circle_Constraints(ConstraintPtr c1, ConstraintPtr c2,
                                 ResultantPtr result)
{
    Resultant   line_sphere;

    /* Do line sphere first. */
    c2->c_type = sphere_feature;
    Combine_Line_Sphere_Constraints(c1, c2, &line_sphere);
    c2->c_type = circle_feature;

    if ( line_sphere.feature_1.c_type == inconsistent_feature )
    {
        result->feature_1.c_type = inconsistent_feature;
        result->feature_2.c_type = null_feature;
    }
    else if ( line_sphere.feature_2.c_type == null_feature )
    {
        result->feature_2.c_type = null_feature;
        if ( Point_Satisfies_Constraint(&(line_sphere.feature_1.c_point),
                                        NULL, c2) )
            result->feature_1 = line_sphere.feature_1;
        else
            result->feature_1.c_type = inconsistent_feature;
    }
    else
    {
        c2->c_type = plane_feature;
        if ( Point_Satisfies_Constraint(&(line_sphere.feature_1.c_point),
                                        NULL, c2) )
        {
            result->feature_1 = line_sphere.feature_1;
            if ( Point_Satisfies_Constraint(&(line_sphere.feature_2.c_point),
                                            NULL, c2) )
                result->feature_2 = line_sphere.feature_2;
            else
                result->feature_2.c_type = null_feature;
        }
        else
        {
            result->feature_2.c_type = null_feature;
            if ( Point_Satisfies_Constraint(&(line_sphere.feature_2.c_point),
                                            NULL, c2) )
                result->feature_1 = line_sphere.feature_2;
            else
                result->feature_1.c_type = inconsistent_feature;
        }
        c2->c_type = circle_feature;
    }
}


static void
Combine_Point_Any_Constraints(ConstraintPtr c1, ConstraintPtr c2, ResultantPtr result)
{
    result->feature_2.c_type = null_feature;

    if ( Point_Satisfies_Constraint(&(c1->c_point), NULL, c2) )
        result->feature_1 = *c1;
    else
        result->feature_1.c_type = inconsistent_feature;

    return;
}

static void
Combine_Sphere_Sphere_Constraints(ConstraintPtr c1, ConstraintPtr c2,
                                  ResultantPtr result)
{
    Vector  normal;
    double  c1_dist, c2_dist;
    double  length;
    double  circ_radius;

    result->feature_2.c_type = null_feature;

    VSub(c1->c_point, c2->c_point, normal);

    if ( VZero(normal) )
    {
        if ( c1->c_radius == c2->c_radius )
            result->feature_1 = *c1;
        else
            result->feature_1.c_type = inconsistent_feature;
        return;
    }

    VUnit(normal, length, normal);
    length = 1 / length;

    if ( length > c1->c_radius + c2->c_radius )
    {
        result->feature_1.c_type = inconsistent_feature;
        return;
    }

    /* They may intersect in a circle. Find the center distance. */
    c2_dist = ( c2->c_radius * c2->c_radius - c1->c_radius * c1->c_radius +
                length * length ) / ( 2 * length );
    c1_dist = c2_dist - length;

    /* Check for a valid center. */
    /*
    if ( ( c1_dist > 0 && c2_dist > 0 && c1_dist + c2_dist < c2->c_radius ) ||
         ( c1_dist < 0 && c2_dist < 0 && -c1_dist - c2_dist < c1->c_radius ) )
    */
    if ( fabs(c1_dist) > c1->c_radius || fabs(c2_dist) > c2->c_radius )
    {
        result->feature_1.c_type = inconsistent_feature;
        return;
    }

    VScalarMul(normal, c2_dist, result->feature_1.c_point);
    VAdd(result->feature_1.c_point, c2->c_point, result->feature_1.c_point);

    circ_radius = sqrt(c2->c_radius * c2->c_radius - c2_dist * c2_dist);
    if ( IsZero(circ_radius) )
    {
        result->feature_1.c_type = point_feature;
    }
    else
    {
        result->feature_1.c_type = circle_feature;
        result->feature_1.c_vector = normal;
        result->feature_1.c_value = VDot(normal, result->feature_1.c_point);
        result->feature_1.c_radius = circ_radius;
    }
}


static void
Combine_Circle_Coplanar_Circle_Constraints(ConstraintPtr c1, ConstraintPtr c2,
                                           ResultantPtr result)
{
    Resultant   sphere_sphere;
    Vector      perp_vect;
    double      temp_d;
    Vector      diff;

    if ( VEqual(c1->c_point, c2->c_point, diff) )
    {
        result->feature_2.c_type = null_feature;
        if ( DEqual(c1->c_radius, c2->c_radius) )
            result->feature_1 = *c1;
        else
            result->feature_1.c_type = inconsistent_feature;
        return;
    }

    /* Intersect them as spheres. */
    c1->c_type = sphere_feature;
    c2->c_type = sphere_feature;
    Combine_Sphere_Sphere_Constraints(c1, c2, &sphere_sphere);
    c1->c_type = circle_feature;
    c2->c_type = circle_feature;

    if ( sphere_sphere.feature_1.c_type == inconsistent_feature )
    {
        result->feature_1.c_type = inconsistent_feature;
        result->feature_2.c_type = null_feature;
    }
    else if ( sphere_sphere.feature_1.c_type == point_feature )
    {
        result->feature_1 = sphere_sphere.feature_1;
        result->feature_2.c_type = null_feature;
    }
    else /* Must be a circle. */
    {
        /* Find a vector perp to the centerline and plane normal. */
        VCross(c1->c_vector, sphere_sphere.feature_1.c_vector, perp_vect);
        VUnit(perp_vect, temp_d, perp_vect);
        VScalarMul(perp_vect, sphere_sphere.feature_1.c_radius, perp_vect);

        result->feature_1.c_type = point_feature;
        VAdd(sphere_sphere.feature_1.c_point, perp_vect,
             result->feature_1.c_point);

        VScalarMul(perp_vect, -1, perp_vect);
        result->feature_2.c_type = point_feature;
        VAdd(sphere_sphere.feature_1.c_point, perp_vect,
             result->feature_2.c_point);
    }
}


static void
Combine_Sphere_Circle_Constraints(ConstraintPtr c1, ConstraintPtr c2,
                                  ResultantPtr result)
{
    Resultant   plane_sphere;

    c2->c_type = plane_feature;
    Combine_Plane_Sphere_Constraints(c2, c1, &plane_sphere);
    c2->c_type = circle_feature;

    if ( plane_sphere.feature_1.c_type == inconsistent_feature )
    {
        result->feature_1.c_type = inconsistent_feature;
        result->feature_2.c_type = null_feature;
    }
    else if ( plane_sphere.feature_1.c_type == point_feature )
    {
        if ( Point_Satisfies_Constraint(&(plane_sphere.feature_1.c_point),
                                        NULL, c2) )
        {
            result->feature_1 = plane_sphere.feature_1;
            result->feature_2.c_type = null_feature;
        }
        else
        {
            result->feature_1.c_type = inconsistent_feature;
            result->feature_2.c_type = null_feature;
        }
    }
    else
        Combine_Circle_Coplanar_Circle_Constraints(&(plane_sphere.feature_1),
                                                   c2, result);
}


static void
Combine_Circle_Circle_Constraints(ConstraintPtr c1, ConstraintPtr c2,
                                  ResultantPtr result)
{
    Resultant   plane_circle;

    c1->c_type = plane_feature;
    Combine_Plane_Circle_Constraints(c1, c2, &plane_circle);
    c1->c_type = circle_feature;

    if ( plane_circle.feature_1.c_type == inconsistent_feature )
    {
        result->feature_1.c_type = inconsistent_feature;
        result->feature_2.c_type = null_feature;
    }
    else if ( plane_circle.feature_1.c_type == point_feature )
    {
        if ( Point_Satisfies_Constraint(&(plane_circle.feature_1.c_point),
                                        NULL, c1) )
        {
            result->feature_1 = plane_circle.feature_1;
            if ( plane_circle.feature_2.c_type == point_feature &&
                 Point_Satisfies_Constraint(&(plane_circle.feature_2.c_point),
                                            NULL, c1))
                result->feature_2 = plane_circle.feature_2;
            else
                result->feature_2.c_type = null_feature;
        }
        else if ( plane_circle.feature_2.c_type == point_feature )
        {
            if ( Point_Satisfies_Constraint(&(plane_circle.feature_2.c_point),
                                            NULL, c1))
                result->feature_1 = plane_circle.feature_2;
            else
                result->feature_1.c_type = inconsistent_feature;
            result->feature_2.c_type = null_feature;
        }
    }
    else
        Combine_Circle_Coplanar_Circle_Constraints(c1, c2, result);
}

