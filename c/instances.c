/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/instances.c,v 1.2 1997/06/11 21:12:52 schenney Exp $
 *
 * $Log: instances.c,v $
 * Revision 1.2  1997/06/11 21:12:52  schenney
 * Added vertex normal calculation function for control pt objects.
 *
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
**  instances.c : Functions for instantiating and modifying object instances.
**
**  Created: 13/03/94
**
*/

#include <math.h>
#include <sced.h>
#include <add_constraint.h>
#include <attributes.h>
#include <bezier.h>
#include <instance_list.h>
#include <layers.h>
#include <torus.h>
#include <triangle.h>
#if HAVE_STRING_H
#include <string.h>
#elif HAVE_STRINGS_H
#include <strings.h>
#endif


static void Instance_Copy_Features(ObjectInstancePtr, ObjectInstancePtr);

/*  ObjectInstancePtr
**  Create_Instance(BaseObjectPtr base, String label)
**
**  Creates an instance of base type type base with name label.
**  The object has the identity transformation installed.
**  ONLY world vertices are filled in, and the bounding box is for these
**  coordinates.
**  The object has default attributes as inheritted from base.
**  Returns the new object, or NULL on failure.
*/
ObjectInstancePtr
Create_Instance(BaseObjectPtr base, String label)
{
    ObjectInstancePtr   res;
    short               num_verts;
    short               num_faces;
    int                 i;

    /* Create the instance and allocate lots of memory. */
    res = New(ObjectInstance, 1);

    object_count[base->b_class]++;

    res->o_label = Strdup(label);

    /* Install it in it's parents instance list. */
    Add_Instance_To_Base(res, base);
    res->o_parent = base;
    res->o_wireframe = base->b_wireframes[0];

    num_verts = res->o_wireframe->num_vertices;
    num_faces = res->o_wireframe->num_faces;

    /* Set the transformations to the identity. */
    NewIdentityMatrix(res->o_transform.matrix);
    NewIdentityMatrix(res->o_inverse);
    VNew(0.0, 0.0, 0.0, res->o_transform.displacement);

    res->o_aliases = NULL;  /* Only allocate space if required, because */
                            /* 99% of the time it will be unused.       */
    res->o_lods = NULL;     /* Same deal. */

    /* It also starts in the default (world) layer. */
    res->o_layer = 0;
    Layer_Add_Instance(NULL, res->o_layer, res);

    /* Copy the world vertices over from the parent. */
    res->o_num_vertices = num_verts;
    res->o_num_real = res->o_wireframe->num_real_verts;
    res->o_world_verts = New(Vector, num_verts);
    res->o_main_verts = New(Vertex, num_verts);
    for ( i = 0 ; i < res->o_num_vertices ; i++ )
        res->o_world_verts[i] = res->o_wireframe->vertices[i];

    /* Copy the world normals over also. */
    res->o_num_faces = num_faces;
    res->o_normals = New(Vector, num_faces);
    for ( i = 0 ; i < res->o_num_faces ; i++ )
        res->o_normals[i] = res->o_wireframe->faces[i].normal;

    /* Nothing depends on a new object. */
    res->o_num_depend = 0;
    res->o_dependents = NULL;

    res->o_flags = 0;
    res->o_dfs_mark = 0;

    res->o_parent->b_create_func(res);

    return res;
}


void
Create_Generic_Object(ObjectInstancePtr obj)
{
    /* Set all the attributes to the defaults. */
    obj->o_attribs = Attribute_New(NULL, TRUE);

    /* Create the features. */
    obj->o_num_features = 4;
    obj->o_features = New(Feature, 4);

    Feature_Create_Origin_Constraints(obj->o_features + origin_feature);
    Feature_Create_Major_Constraints(obj->o_features + major_feature, 0, 0, 1);
    Feature_Create_Minor_Constraints(obj->o_features + minor_feature, 1, 0, 0);
    if ( obj->o_parent->b_class == cone_obj )
        Feature_Create_Cone_Scale_Constraints(obj->o_features + scale_feature,
                                obj,
                                obj->o_world_verts + obj->o_parent->b_ref_num);
    else if ( obj->o_parent->b_class == cylinder_obj )
        Feature_Create_Cyl_Scale_Constraints(obj->o_features + scale_feature,
                                obj,
                                obj->o_world_verts + obj->o_parent->b_ref_num);
    else if ( obj->o_parent->b_class == plane_obj || Obj_Is_Control(obj) )
        Feature_Create_Scale_Constraints(obj->o_features + scale_feature,
                                obj->o_world_verts + obj->o_parent->b_ref_num);
    else
        Feature_Create_Uniform_Scale_Constraints(obj->o_features+scale_feature,
                                obj->o_world_verts + obj->o_parent->b_ref_num);

    obj->o_dynamic_func = Maintain_Generic_Dynamic;
    obj->o_static_func = Maintain_Generic_Static;
}


/*  ObjectInstancePtr
**  Copy_Object_Instance(ObjectInstancePtr orig)
**  Copies all of orig, giving the copy a new name.  Returns NULL on failure.
*/
ObjectInstancePtr
Copy_Object_Instance(ObjectInstancePtr orig)
{
    ObjectInstancePtr   res;
    int                 i;
    char                *label;

    /* Create the new label. */
    label = New(char, strlen(orig->o_label) + 10);
    sprintf(label, "%s_%d", orig->o_label,
            object_count[orig->o_parent->b_class]);
    res = Create_Instance(orig->o_parent, label);
    free(label);

    /* Copy the wireframe. If it's different (dense) then realloc vertices
    ** and normals.
    */
    if ( res->o_wireframe != orig->o_wireframe )
    {
        free(res->o_world_verts);
        free(res->o_main_verts);
        free(res->o_normals);
        res->o_wireframe = orig->o_wireframe;
        res->o_num_vertices = orig->o_num_vertices;
        res->o_num_real = orig->o_num_real;
        res->o_num_faces = orig->o_num_faces;
        res->o_world_verts = New(Vector, res->o_num_vertices);
        res->o_main_verts = New(Vertex, res->o_num_vertices);
        res->o_normals = New(Vector, res->o_num_faces);
    }

    /* Copy the transformations. */
    res->o_transform = orig->o_transform;
    res->o_inverse = orig->o_inverse;

    /* Set all the attributes to the parent's defaults. */
    Attribute_Copy(res->o_attribs, orig->o_attribs);

    /* Copy aliases. */
    if ( orig->o_aliases )
    {
        res->o_aliases = (void**)New(void*, LastTarget);
        for ( i = NoTarget ; i < LastTarget ; i++ )
            if ( orig->o_aliases[i] )
                res->o_aliases[i] = (void*)Strdup((char*)orig->o_aliases[i]);
            else
                res->o_aliases[i] = NULL;
    }

    if ( orig->o_lods )
    {
        res->o_lods = New(LODInfo, 1);
        res->o_lods->num_lods = orig->o_lods->num_lods;
        res->o_lods->lods = New(float, orig->o_lods->num_lods);
        for ( i = 0 ; i < res->o_lods->num_lods ; i++ )
            res->o_lods->lods[i] = orig->o_lods->lods[i];
    }

    /* Copy the world  vertices over. */
    for ( i = 0 ; i < res->o_num_vertices ; i++ )
    {
        res->o_world_verts[i] = orig->o_world_verts[i];
        res->o_main_verts[i] = orig->o_main_verts[i];
    }

    /* Copy the world normals over also. */
    for ( i = 0 ; i < res->o_num_faces ; i++ )
        res->o_normals[i] = orig->o_normals[i];

    /* Copy the feature information. */
    Instance_Copy_Features(orig, res);

    /* Copy the hook. */
    if ( Obj_Is_Torus(orig) )
        Torus_Copy_Hook(orig, res);
    else if ( Obj_Is_Triangle(orig) )
        Triangle_Copy_Hook(orig, res);
    else if ( Obj_Is_Bezier(orig) )
        Bezier_Copy_Hook(orig, res);

    if ( res->o_layer != orig->o_layer )
    {
        Layer_Remove_Instance(NULL, res->o_layer, res);
        res->o_layer = orig->o_layer;
        Layer_Add_Instance(NULL, res->o_layer, res);
    }

    return res;
}


static void
Instance_Add_Dependencies(ConstraintSpecPtr spec, ObjectInstancePtr obj,
                          void *ptr, void *ptr2, int dummy)
{
    ObjectInstancePtr   orig = (ObjectInstancePtr)ptr;

    if ( Spec_Is_Dependent(spec->spec_type) )
    {
        if ( spec_object(spec) == orig )
            spec->spec_data = (void*)obj;
        else
            Add_Dependency(spec_object(spec), obj);
    }
}


void
Copy_Constraint_Set(int src_num, int *dest_num, ConstraintPtr src_cons,
                    ConstraintPtr *dest_cons, ObjectInstancePtr src,
                    ObjectInstancePtr dest)
{
    int i, j;

    for ( i = 0 ; i < *dest_num ; i++ )
        free((*dest_cons)[i].c_specs);

    if ( src_num )
    {
        if ( *dest_num )
            *dest_cons = More(*dest_cons, ConstraintData, src_num);
        else
            *dest_cons = New(ConstraintData, src_num);
        *dest_num = src_num;
        for ( i = 0 ; i < src_num ; i++ )
        {
            (*dest_cons)[i] = src_cons[i];
            (*dest_cons)[i].c_label = Strdup(src_cons[i].c_label);
            (*dest_cons)[i].c_specs = New(ConstraintSpec,
                                          (*dest_cons)[i].c_num_specs);
            for ( j = 0 ; j < (*dest_cons)[i].c_num_specs ; j++ )
                (*dest_cons)[i].c_specs[j] = src_cons[i].c_specs[j];
            Constraint_Manipulate_Specs((*dest_cons) + i, dest, (void*)src,
                                        NULL, 0, Instance_Add_Dependencies);
        }
    }
    else
    {
        free(*dest_cons);
        *dest_cons = NULL;
    }
}

/*  void
**  Copy_Instance_Constraints(ObjectInstancePtr src, ObjectInstancePtr dest)
**  Copies all the constraint from an object.
*/
static void
Instance_Copy_Features(ObjectInstancePtr src, ObjectInstancePtr dest)
{
    int i;

    Remove_All_Object_Dependencies(dest);

    for ( i = 0 ; i < src->o_num_features ; i++ )
    {
        dest->o_features[i].base = src->o_features[i].base;
        dest->o_features[i].location = src->o_features[i].location;
        Copy_Constraint_Set(src->o_features[i].num_constraints,
                            &(dest->o_features[i].num_constraints),
                            src->o_features[i].constraints,
                            &(dest->o_features[i].constraints), src, dest);
        dest->o_features[i].flags = src->o_features[i].flags;
    }
}


/*  void
**  Rename_Instance(ObjectInstancePtr obj, char *label)
**  Changes the existing label on obj to label.
*/
void
Rename_Instance(ObjectInstancePtr obj, char *label)
{
    free(obj->o_label);
    obj->o_label = Strdup(label);

    changed_scene = TRUE;
}


/*  int
**  Transform_Instance(ObjectInstancePtr obj, Transformation *transform,
**                      Boolean replace)
**  Transforms object according to tranform.  If replace is True, overwrites
**  the existing transformation, otherwise multiplies onto it.
**  Returns 0 if the resulting transformation is singular, and leaves obj
**  unchanged.
*/
int
Transform_Instance(ObjectInstancePtr obj, Transformation *transform,
                    Boolean replace)
{
    Transformation  existing = obj->o_transform;
    Matrix          transp;
    int             i;


    if (replace)
        obj->o_transform = *transform;
    else
    {
        obj->o_transform.matrix =
            MMMul(&(transform->matrix), &(existing.matrix));
        VAdd(transform->displacement, existing.displacement,
            obj->o_transform.displacement);
    }

    /* Calculate the inverse transformation. */
    obj->o_inverse = MInvert(&(obj->o_transform.matrix));
    if ( MZero(obj->o_inverse) )
    {
        obj->o_transform = existing;
        return FALSE;
    }

    /* Transform all the vertices. */
    if ( Obj_Is_Torus(obj) )
    {
        Torus_Calculate_Vertices(obj->o_world_verts, obj->o_num_vertices,
                                 ((TorusPtr)obj->o_hook)->major_radius);
        Transform_Vertices(obj->o_transform, obj->o_world_verts,
                           obj->o_num_vertices)
    }
    else if ( Obj_Is_Control(obj) )
    {
        control_part(obj)->calc_verts(obj, obj->o_world_verts,
				      obj->o_num_real, obj->o_num_vertices);
        Transform_Vertices(obj->o_transform, obj->o_world_verts,
                           obj->o_num_vertices)
    }
    else
    {
        for ( i = 0 ; i < obj->o_num_vertices ; i++ )
            Transform_Vector(obj->o_transform, obj->o_wireframe->vertices[i],
                             obj->o_world_verts[i])
    }

    /* Calculate the face normals. */
    /* To transform a normal multiply it by the transpose of the inverse    */
    /* transformation matrix.                                               */
    if ( Obj_Is_Bezier(obj) )
        control_part(obj)->calc_face_norms(obj, obj->o_normals);
    else
    {
        if ( Obj_Is_Triangle(obj) )
            control_part(obj)->calc_face_norms(obj, obj->o_normals);
        MTrans(obj->o_inverse, transp);
        for ( i = 0 ; i < obj->o_num_faces ; i++ )
            MVMul(transp, obj->o_wireframe->faces[i].normal, obj->o_normals[i]);
    }

    return TRUE;

}


/*  void
**  Displace_Instance(ObjectInstancePtr obj, Vector displacement)
**  Displaces the instance by displacement.  Its an additive displacement.
*/
void
Displace_Instance(ObjectInstancePtr obj, Vector displacement)
{
    int     i;


    VAdd(obj->o_transform.displacement, displacement,
         obj->o_transform.displacement);

    /* Transform all the vertices. */
    for ( i = 0 ; i < obj->o_num_vertices ; i++ )
        VAdd(displacement, obj->o_world_verts[i], obj->o_world_verts[i]);

}

/*  void
**  Modify_Instance_Attributes(ObjectInstancePtr obj, Attributes *new, int flag)
**  Modifies the attributes of obj indicated by flags to match new.
*/
void
Modify_Instance_Attributes(ObjectInstancePtr obj, AttributePtr new, int flag)
{
    if ( ! obj->o_attribs )
        return;

    /* Always modify use_extension. */
    obj->o_attribs->defined = new->defined;
    obj->o_attribs->use_extension = new->use_extension;

    if ( flag & ModSimple )
    {
        obj->o_attribs->color = new->color;
        obj->o_attribs->diff_coef = new->diff_coef;
        obj->o_attribs->spec_coef = new->spec_coef;
        obj->o_attribs->spec_power =  new->spec_power;
        obj->o_attribs->reflect_coef = new->reflect_coef;
        obj->o_attribs->transparency = new->transparency;
        obj->o_attribs->refract_index = new->refract_index;
    }

    if ( flag & ModExtend )
    {
        if ( obj->o_attribs->extension[target_renderer] )
            free(obj->o_attribs->extension[target_renderer]);
        if ( new->extension[target_renderer] )
            obj->o_attribs->extension[target_renderer] =
                new->extension[target_renderer];
        else
            obj->o_attribs->extension[target_renderer] = NULL;
        obj->o_attribs->use_obj_trans = new->use_obj_trans;
        obj->o_attribs->open = new->open;
    }

    if ( flag & ModLight )
        obj->o_attribs->intensity = new->intensity;

    if ( flag & ModSpotlight )
    {
        obj->o_attribs->radius = new->radius;
        obj->o_attribs->tightness = new->tightness;
        obj->o_attribs->invert = new->invert;
    }

    if ( flag & ModArealight )
    {
        obj->o_attribs->samples = new->samples;
        obj->o_attribs->jitter = new->jitter;
    }

    changed_scene = TRUE;
}


/* Destroys all the features of an object. */
void
Instance_Destroy_Features(ObjectInstancePtr obj)
{
    int i, j;

    for ( i = 0 ; i < obj->o_num_features ; i++ )
    {
        for ( j = 0 ; j < obj->o_features[i].num_constraints ; j++ )
        {
            free(obj->o_features[i].constraints[j].c_label);
            free(obj->o_features[i].constraints[j].c_specs);
        }
        free(obj->o_features[i].constraints);
    }
    if ( obj->o_num_features )
        free(obj->o_features);
}


void
Destroy_Generic_Object(ObjectInstancePtr obj)
{
    Attribute_Destroy(obj->o_attribs);
    free(obj->o_attribs);
}

/*  void
**  Destroy_Instance(ObjectInstancePtr obj)
**  Removes obj from its parent and frees all the memory associated with obj.
*/
void
Destroy_Instance(ObjectInstancePtr obj)
{
    int i;

    Remove_Instance_From_Base(obj);

    if ( obj->o_aliases )
    {
        for ( i = NoTarget ; i < LastTarget ; i++ )
            if ( obj->o_aliases[i] )
                free(obj->o_aliases[i]);
        free(obj->o_aliases);
    }

    if ( obj->o_lods )
    {
        free(obj->o_lods->lods);
        free(obj->o_lods);
    }

    Remove_All_Object_Dependencies(obj);

    for ( i = obj->o_num_depend - 1 ; i >= 0 ; i-- )
        Constraint_Remove_References(obj->o_dependents[i].obj, obj);

    Layer_Remove_Instance(NULL, obj->o_layer, obj);

    free(obj->o_label);
    free(obj->o_world_verts);
    free(obj->o_main_verts);
    free(obj->o_normals);

    Instance_Destroy_Features(obj);

    obj->o_parent->b_destroy_func(obj);

    free(obj);
}


InstanceList
Instances_Build_Visible_List(InstanceList basis)
{
    InstanceList    result = NULL;
    InstanceList    elmt;

    for ( elmt = basis ; elmt ; elmt = elmt->next )
        if ( elmt->the_instance->o_flags & ObjVisible )
            Insert_Element(&result, elmt->the_instance);

    return result;
}
