/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/dependency.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: dependency.c,v $
 * Revision 1.0  1997/05/06 20:30:22  schenney
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
**  dependency.c: Functions for manipulating dependencies.
*/

#include <sced.h>
#include <bezier.h>
#include <constraint.h>
#include <torus.h>

void
Add_Dependency(ObjectInstancePtr ref, ObjectInstancePtr obj)
{
    int i;

    if ( ref == obj )
        return;

    /* Add the dependency to the ref_objects list. */
    if ( ref->o_num_depend == 0 )
    {
        ref->o_dependents = New(Dependent, 1);
        ref->o_dependents[0].obj = obj;
        ref->o_dependents[0].count = 1;
        ref->o_num_depend++;
    }
    else
    {
        for ( i = 0 ; i < ref->o_num_depend ; i++ )
            if ( ref->o_dependents[i].obj == obj )
            {
                ref->o_dependents[i].count++;
                return;
            }

        ref->o_dependents = More(ref->o_dependents, Dependent,
                                 ref->o_num_depend + 1);
        ref->o_dependents[ref->o_num_depend].obj = obj;
        ref->o_dependents[ref->o_num_depend].count = 1;
        ref->o_num_depend++;
    }
}



/* If the spec is a reference spec and the referenced object passes the spec
** test, changes that spec into the type indicated by abs.
*/
static void
Constraint_Update_Spec(ConstraintSpecPtr spec, ObjectInstancePtr src,
                        void *test_func, void *test_arg, int abs)
{
    Vector  temp_v1;

    if ( ! Spec_Is_Dependent(spec->spec_type) ||
         ! ((ObjectTestFunction)test_func)(spec_object(spec), test_arg) )
        return;

    Dependencies_Remove_Object(spec, src, NULL, NULL, 0);

    if ( spec->spec_type == reference_spec )
    {
        Ref_Transform_Vector(spec_object(spec), spec->spec_vector, temp_v1)
    }
    else if ( spec->spec_type == vertex_spec )
    {
        Ref_Transform_Vector(spec_object(spec), 
                             control_part(spec_object(spec))->
                                 control_verts[(int)spec->spec_vector.x],
                             temp_v1)
    }
    else
        temp_v1 = Bezier_Calculate_Point(spec_object(spec), spec->spec_vector.x,
                                         spec->spec_vector.y);

    if ( abs )
    {
        spec->spec_type = absolute_spec;
        spec->spec_vector = temp_v1;
        spec->spec_data = NULL;
    }
    else
    {
        spec->spec_type = offset_spec;
        VSub(temp_v1, src->o_world_verts[src->o_num_vertices - 1],
             spec->spec_vector);
        spec->spec_data = NULL;
    }
}

void
Constraint_Change_References(ObjectInstancePtr src,
                             ObjectTestFunction test_func, void *test_arg)
{
    Constraint_Manipulate_Constraints(src, (void*)test_func, test_arg,
                                      Constraint_Update_Spec);
}
                             

static Boolean
Constraint_Remove_References_Test(ObjectInstancePtr obj, void *ptr)
{
    return ( obj == (ObjectInstancePtr)ptr );
}

/* Remove all references to obj from the src object's constraints. */
void
Constraint_Remove_References(ObjectInstancePtr src, ObjectInstancePtr obj)
{
    Constraint_Change_References(src, Constraint_Remove_References_Test,
                                 (void*)obj);
}


/* Removes one count of obj from the dependency list of spec_object(spec).
*/
void
Dependencies_Remove_Object(ConstraintSpecPtr spec, ObjectInstancePtr obj,
                           void *ptr, void *ptr2, int dummy)
{
    ObjectInstancePtr   src;
    int i, index;

    if ( ! Spec_Is_Dependent(spec->spec_type) )
        return;

    src = spec_object(spec);

    if ( src == obj )
        return;

    for ( index = 0 ;
          index < src->o_num_depend && src->o_dependents[index].obj != obj ;
          index++ );

    if ( index == src->o_num_depend )
        return;

    if ( src->o_dependents[index].count == 1 )
    {
        for ( i = index + 1 ; i < src->o_num_depend ; i++ )
            src->o_dependents[ i - 1 ] = src->o_dependents[i];
        src->o_num_depend--;
    }
    else
        src->o_dependents[index].count--;

    if ( ! src->o_num_depend ) free(src->o_dependents);
}


static Boolean
Remove_All_Object_Dependencies_Test(ObjectInstancePtr obj, void *data)
{
    return TRUE;
}

/* Removes all dependencies that obj has on other obejcts. */
void
Remove_All_Object_Dependencies(ObjectInstancePtr obj)
{
    Constraint_Change_References(obj, Remove_All_Object_Dependencies_Test,NULL);
}


/* Removes the indexed constraint. */
void
Remove_Constraint(ObjectInstancePtr obj, ConstraintPtr *cons, int *num, int index)
{
    int i;

    /* Remove dependencies. */
    Constraint_Manipulate_Specs((*cons) + index, obj, NULL, NULL, 0,
                                Dependencies_Remove_Object);

    /* Free specs. */
    if ( (*cons)[index].c_num_specs )
        free((*cons)[index].c_specs);
    free((*cons)[index].c_label);

    /* Remove it from the list. */
    if ( *num == 1 )
    {
        *num = 0;
        free(*cons);
        *cons = NULL;
        return;
    }

    for ( i = index + 1 ; i < *num ; i++ )
        (*cons)[i-1] = (*cons)[i];
    (*num)--;
}


/* Manipulates all the constraints on an object. */
void
Constraint_Manipulate_Constraints(ObjectInstancePtr src, void *arg1, void *arg2,
                                  SpecFunction func)
{
    int i, j;

    for ( i = 0 ; i < src->o_num_features ; i++ )
        for ( j = 0 ; j < src->o_features[i].num_constraints ; j++ )    
            Constraint_Manipulate_Specs(src->o_features[i].constraints + j,
                                        src, arg1, arg2, (i==0 ? 1 : 0), func);
}

                                  
