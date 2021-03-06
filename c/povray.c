/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/povray.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: povray.c,v $
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
**  povray.c : export functions for povray.
**
**  External function (there's just one).
**
**  void
**  Export_POVray(FILE *outfile)
**  Exports all the relevant info into outfile.
*/

#include <math.h>
#include <ctype.h>
#include <sced.h>
#include <aggregate.h>
#include <attributes.h>
#include <base_objects.h>
#include <bezier.h>
#include <constraint.h> /* For Distance_Point_To_Plane. */
#include <csg.h>
#include <hash.h>
#include <time.h>

static int  Export_Camera(FILE *, Camera);
static int  Export_Light(FILE *, ObjectInstancePtr, TransformPtr, AttributePtr);
static int  Export_All_Transforms(FILE*, InstanceList, TransformPtr);
static int  Export_Basetypes(FILE*, BaseObjectPtr);
static int  Export_CSG_Tree(FILE*, CSGNodePtr);
static int  Export_Wireframe(FILE*, WireframePtr);
static int  Export_Instances(FILE *, InstanceList, TransformPtr, AttributePtr);
static void Export_Object_Type(FILE*, ObjectInstancePtr);
static void Export_Attributes(FILE*, AttributePtr);
static int  Export_Declarations(FILE*, ColorVector);


#define POVVPrint(v) fprintf(outfile, "< %1.15g, %1.15g, %1.15g >\n", (v).x, (v).y, (v).z);

/* Stuff to allow exporting of transformation. */
static char transform_name[16];
static HashTable    base_hash;
static HashTable    transform_hash;
static long         transform_number;
static Transformation   identity;

/*  int
**  Export_POVray(FILE *outfile, ScenePtr scene)
**  Exports all the relevant info into outfile.
*/
int
Export_POVray(FILE *outfile, ScenePtr scene)
{
    time_t          current_time;

    time(&current_time);
    Identity_Transform(identity);

    base_hash = Hash_New_Table();
    if ( ! sced_preferences.povray_v3 )
    {
        transform_hash = Hash_New_Table();
        transform_number = 0;
    }

    if ( fprintf(outfile, "// File generated by sced\n") < 0 ||
         fprintf(outfile, "// %s\n", ctime(&current_time)) < 0 ||
         Export_Declarations(outfile, scene->ambient) < 0 ||
         Export_Camera(outfile, scene->camera) < 0 ||
         Export_Light(outfile, scene->light, &identity, NULL) < 0 ||
         ( sced_preferences.povray_v3 ? 0 :
            Export_All_Transforms(outfile, scene->instances, &identity) < 0 ) ||
         Export_Instances(outfile, scene->instances, &identity, NULL) < 0 )
    {
        Popup_Error("Write Failed!", main_window.shell, "Error");
        return 0;
    }

    return 1;
}



static int
Export_Camera(FILE *outfile, Camera cam)
{
    if ( ! cam.defined ) return 1;

    /* For povray, the camera is a big structure definition. */
    fprintf(outfile, "// Camera definition\n");
    fprintf(outfile, "camera {\n");
    fprintf(outfile, "\tlocation ");    POVVPrint(cam.location);
    fprintf(outfile, "\tsky ");         POVVPrint(cam.look_up);
    fprintf(outfile, "\tdirection <0, 0, %g>\n", cam.eye_dist);
    fprintf(outfile, "\tright <-%g, 0, 0>\n",
            cam.eye_dist * 2 * tan(cam.horiz_fov * M_PI / 360));
    fprintf(outfile, "\tup <0, %g, 0>\n",
            cam.eye_dist * 2 * tan(cam.vert_fov * M_PI / 360));
    fprintf(outfile, "\tlook_at ");     POVVPrint(cam.look_at);

    return fprintf(outfile, "}\n\n");
}


static int
Export_Light(FILE *outfile, ObjectInstancePtr light, TransformPtr add_trans,
             AttributePtr def_attribs)
{
    double  radius;
    double  cos_rad;
    Vector  direction;
    Vector  vect1, vect2, temp_v;

    if ( ! light ) return 1;

    if ( light->o_attribs->defined || ! def_attribs )
        def_attribs = light->o_attribs;

    if ( light->o_parent->b_class == dirlight_obj )
    {
        fprintf(stderr, "Sced: Directional lights not supported by POV\n");
        fprintf(stderr, "Sced: Exporting as point light instead.\n");
    }

    fprintf(outfile, "light_source {\n");
    fprintf(outfile, "\t");
    Transform_Vector(*add_trans, light->o_transform.displacement, vect1);
    POVVPrint(vect1);
    fprintf(outfile, "\tcolor red %g green %g blue %g\n",
            def_attribs->intensity.red,
            def_attribs->intensity.green,
            def_attribs->intensity.blue);

    if ( light->o_parent->b_class == spotlight_obj )
    {
        /* Calculate the radius. */
        VSub(light->o_world_verts[0], light->o_world_verts[9], vect1);
        VSub(light->o_world_verts[8], light->o_world_verts[9], vect2);
        VUnit(vect1, radius, vect1);
        VUnit(vect2, radius, vect2);
        cos_rad = VDot(vect1, vect2);
        radius = acos(cos_rad) * 180 / M_PI;

        if ( def_attribs->invert )
        {
            /* Invert it. */
            /* vect2 still points toward direction. */
            VScalarMul(vect2, -1.0, vect2);
            VAdd(vect2, light->o_world_verts[9], direction);

            radius += 90.0;
        }
        else
            direction = light->o_world_verts[8];
        Transform_Vector(*add_trans, direction, direction);

        fprintf(outfile, "\tspotlight\n");
        fprintf(outfile, "\tpoint_at ");
        POVVPrint(direction);
        fprintf(outfile, "\tradius %g\n", radius);
        fprintf(outfile, "\tfalloff %g\n", radius *
                def_attribs->radius);
        fprintf(outfile, "\ttightness %g\n",
                def_attribs->tightness);
    }

    if ( light->o_parent->b_class == arealight_obj )
    {
        Matrix  transp, inverse;

        /* Calculate the edges. */
        VSub(light->o_world_verts[0], light->o_world_verts[1], vect1);
        VSub(light->o_world_verts[3], light->o_world_verts[0], vect2);
        inverse = MInvert(&(add_trans->matrix));
        MTrans(inverse, transp);
        MVMul(transp, vect1, temp_v);   vect1 = temp_v;
        MVMul(transp, vect2, temp_v);   vect2 = temp_v;

        fprintf(outfile, "\tarea_light\n");
        fprintf(outfile, "\t");
        POVVPrint(vect1);
        fprintf(outfile, "\t,");
        POVVPrint(vect2);
        fprintf(outfile, "\t, %d, %d\n\tadaptive 3\n",
                def_attribs->samples & 0xFF,
                (def_attribs->samples >> 8 ) & 0xFF);
        if ( def_attribs->jitter )
            fprintf(outfile, "\tjitter\n");
    }

    fprintf(outfile, "}\n");

    return fprintf(outfile, "\n");
}

static void
Export_Transform_V3(FILE *outfile, ObjectInstancePtr inst, TransformPtr add)
{
    Matrix  temp_m;
    Vector  displ;

    MVMul(add->matrix, inst->o_transform.displacement, displ);
    VAdd(displ, add->displacement, displ);
    temp_m = MMMul(&(add->matrix), &(inst->o_transform.matrix));

    fprintf(outfile, "\tmatrix <%1.15g, %1.15g, %1.15g, %1.15g,"
                     " %1.15g, %1.15g, %1.15g, %1.15g, %1.15g, "
                     " %1.15g, %1.15g, %1.15g>\n",
            temp_m.x.x, temp_m.y.x, temp_m.z.x, temp_m.x.y, temp_m.y.y,
            temp_m.z.y, temp_m.x.z, temp_m.y.z, temp_m.z.z, displ.x,
            displ.y, displ.z);
}

static void
Export_Transform(FILE *outfile, ObjectInstancePtr inst, TransformPtr add)
{
    Matrix  transp;

    if ( sced_preferences.povray_v3 )
    {
        Export_Transform_V3(outfile, inst, add);
        return;
    }

    Hash_Insert(transform_hash, (unsigned long)&(inst->o_transform),
                (void*)transform_number);
    sprintf(transform_name, "transform_%ld", (unsigned long)transform_number++);

    MTrans(inst->o_transform.matrix, transp);
    fprintf(outfile, "#declare %s = transform {\n", transform_name);
    fprintf(outfile, "\tmatrix\n");
    fprintf(outfile, "\t"); POVVPrint(transp.x);
    fprintf(outfile, "\t"); POVVPrint(transp.y);
    fprintf(outfile, "\t"); POVVPrint(transp.z);
    fprintf(outfile, "\ttranslate ");
    POVVPrint(inst->o_transform.displacement);
    fprintf(outfile, "\t};\n");

}

static int
Export_All_Transforms(FILE *outfile, InstanceList insts, TransformPtr add)
{       
    InstanceList    inst_elmt;

    fprintf(outfile, "// Transformations\n");

    for ( inst_elmt = insts ; inst_elmt ; inst_elmt = inst_elmt->next )
        if ( inst_elmt->the_instance->o_parent->b_class != plane_obj )
            Export_Transform(outfile, inst_elmt->the_instance, add);

    return fprintf(outfile, "\n");
}


static void
Export_CSG_Transforms(FILE *outfile, CSGNodePtr tree)
{
    if ( ! tree ) return;

    if ( tree->csg_op == csg_leaf_op )
        Export_Transform(outfile, tree->csg_instance, &identity);
    else
    {
        Export_CSG_Transforms(outfile, tree->csg_left_child);
        Export_CSG_Transforms(outfile, tree->csg_right_child);
    }
}


static void
Export_CSG_Bases(FILE *outfile, CSGNodePtr tree)
{
    if ( ! tree ) return;

    if ( tree->csg_op == csg_leaf_op )
    {
        if ( Obj_Is_Derived(tree->csg_instance) )
            Export_Basetypes(outfile, tree->csg_instance->o_parent);
    }
    else
    {
        Export_CSG_Bases(outfile, tree->csg_left_child);
        Export_CSG_Bases(outfile, tree->csg_right_child);
    }
}


static void
Export_Aggregate_Bases(FILE *outfile, InstanceList insts)
{
    InstanceList    inst_elmt;

    for ( inst_elmt = insts ; inst_elmt != NULL ; inst_elmt = inst_elmt->next )
	if ( Obj_Is_Derived(inst_elmt->the_instance) )
	    Export_Basetypes(outfile, inst_elmt->the_instance->o_parent);
}


static int
Export_Aggregate_List(FILE *outfile, InstanceList insts)
{
    fprintf(outfile, "union {\n");

    Export_Instances(outfile, insts, &identity, NULL);

    return fprintf(outfile, "}\n");
}


static int
Export_Basetypes(FILE *outfile, BaseObjectPtr base_obj)
{
    CSGNodePtr  new_tree;

    if ( Hash_Get_Value(base_hash, (unsigned long)base_obj) != (void*)-1 )
        return 1;

    if ( base_obj->b_class == csg_obj )
    {
        new_tree = CSG_Contract_Tree((CSGNodePtr)base_obj->b_struct);
        if ( ! sced_preferences.povray_v3 )
            Export_CSG_Transforms(outfile, new_tree);
        Export_CSG_Bases(outfile, new_tree);
    }
    else if ( base_obj->b_class == aggregate_obj )
	Export_Aggregate_Bases(outfile,
			       ((AggregatePtr)base_obj->b_struct)->children);

    /* Save information about non-default base objects. */
    fprintf(outfile, "// BaseObjects\n");

    fprintf(outfile, "#declare %s =\n", base_obj->b_label);
    if ( base_obj->b_class == csg_obj )
    {
        Export_CSG_Tree(outfile, new_tree);
        CSG_Delete_Tree(new_tree, FALSE);
    }
    else if ( base_obj->b_class == wireframe_obj )
        Export_Wireframe(outfile, base_obj->b_major_wires[0]);
    else if ( base_obj->b_class == aggregate_obj )
	Export_Aggregate_List(outfile,
			      ((AggregatePtr)base_obj->b_struct)->children);

    Hash_Insert(base_hash, (unsigned long)base_obj, (void*)base_obj);

    return fprintf(outfile, ";\n");
}


static int
Export_CSG_Tree(FILE *outfile, CSGNodePtr tree)
{
    if ( ! tree ) return 1;

    if ( tree->csg_op == csg_leaf_op )
    {
        fprintf(outfile, "// %s\n", tree->csg_instance->o_label);

        Export_Object_Type(outfile, tree->csg_instance);
        if ( tree->csg_instance->o_parent->b_class == plane_obj )
        {
            Vector  zero_v;
            Vector  normal;
            Matrix  inverse;
            double  temp_d;

            inverse = MInvert(&(tree->csg_instance->o_transform.matrix));
            VUnit(inverse.z, temp_d, normal);
            VNew(0, 0, 0, zero_v);
            fprintf(outfile, "\t");
            POVVPrint(normal);
            fprintf(outfile, "\t, %g\n",
                    -Distance_Point_To_Plane(&normal,
                                &tree->csg_instance->o_transform.displacement,
                                &zero_v));
        }

        if ( tree->csg_instance->o_attribs->use_obj_trans )
            Export_Attributes(outfile, tree->csg_instance->o_attribs);

        if ( tree->csg_instance->o_parent->b_class != plane_obj )
        {
            if ( sced_preferences.povray_v3 )
                Export_Transform(outfile, tree->csg_instance, &identity);
            else
            {
                fprintf(outfile, "\ttransform transform_%ld\n",
                (unsigned long)Hash_Get_Value(transform_hash,
                         (unsigned long)&(tree->csg_instance->o_transform)));
            }
        }

        if ( ! tree->csg_instance->o_attribs->use_obj_trans )
            Export_Attributes(outfile, tree->csg_instance->o_attribs);
    }
    else
    {
        switch ( tree->csg_op )
        {
            case csg_union_op: 
                fprintf(outfile, "union { \n");
                break;
            case csg_intersection_op: 
                fprintf(outfile, "intersection { \n");
                break;
            case csg_difference_op: 
                fprintf(outfile, "difference { \n");
                break;
            default:;
        }

        Export_CSG_Tree(outfile, tree->csg_left_child);
        Export_CSG_Tree(outfile, tree->csg_right_child);
    }

    return fprintf(outfile, "}\n");
}


static void
Export_Face(FILE *outfile, WireframePtr src, FacePtr face)
{
    fprintf(outfile, "smooth_triangle {\n\t");
    POVVPrint(src->vertices[face->vertices[0]]);
    fprintf(outfile, "\t,");
    POVVPrint(src->normals[face->vertices[0]]);
    fprintf(outfile, "\t,");
    POVVPrint(src->vertices[face->vertices[2]]);
    fprintf(outfile, "\t,");
    POVVPrint(src->normals[face->vertices[2]]);
    fprintf(outfile, "\t,");
    POVVPrint(src->vertices[face->vertices[1]]);
    fprintf(outfile, "\t,");
    POVVPrint(src->normals[face->vertices[1]]);

    Export_Attributes(outfile, face->face_attribs);
    fprintf(outfile, "}\n");
}


static int
Export_Wireframe(FILE *outfile, WireframePtr wire)
{
    int i;

    fprintf(outfile, "union {\n");

    for ( i = 0 ; i < wire->num_faces ; i++ )
        Export_Face(outfile, wire, wire->faces + i);

    return fprintf(outfile, "}\n");
}


static int
Export_Aliased_Object(FILE *outfile, ObjectInstancePtr obj, TransformPtr add,
                      AttributePtr def_attribs)
{
    char    *alias = (char*)obj->o_aliases[POVray];
    int     index;

    index = 0;
    while ( alias[index] != '\0' )
    {
        if ( alias[index] == '(' && alias[index + 1] == '*' )
        {
            if ( ! strncmp(alias + index, "(*transform*)", 13) )
            {
                if ( sced_preferences.povray_v3 )
                    Export_Transform(outfile, obj, add);
                else
                {
                    fprintf(outfile, "\ttransform transform_%ld\n",
                            (unsigned long)Hash_Get_Value(transform_hash,
                                        (unsigned long)&(obj->o_transform)));
                    fprintf(outfile, "\ttransform transform_%ld\n",
                            (unsigned long)Hash_Get_Value(transform_hash,
                                                          (unsigned long)add));
                }
                index += 13;
            }
            else if ( ! strncmp(alias + index, "(*attributes*)", 14) )
            {
                if ( obj->o_attribs->defined )
                    Export_Attributes(outfile, obj->o_attribs);
                else
                    Export_Attributes(outfile, def_attribs);
                index += 14;
            }
            else
                fputc((int)alias[index++], outfile);
        }
        else
            fputc((int)alias[index++], outfile);
    }

    return 1;
}



static int
Export_Instances(FILE *outfile, InstanceList insts, TransformPtr add_trans,
                 AttributePtr def_attribs)
{
    InstanceList        inst_elmt;
    ObjectInstancePtr   inst;

    for ( inst_elmt = insts ; inst_elmt != NULL ; inst_elmt = inst_elmt->next )
    {
        inst = inst_elmt->the_instance;

        if ( Obj_Is_Construction(inst) )
            continue;

        if ( inst->o_parent->b_class == light_obj ||
             inst->o_parent->b_class == spotlight_obj ||
             inst->o_parent->b_class == arealight_obj ||
             inst->o_parent->b_class == dirlight_obj )
            Export_Light(outfile, inst, add_trans, def_attribs);
        else if ( Obj_Is_Aliased(inst, POVray) )
            Export_Aliased_Object(outfile, inst, add_trans, def_attribs);
        else
        {
            if ( inst->o_parent->b_class == csg_obj ||
                 inst->o_parent->b_class == wireframe_obj ||
		 inst->o_parent->b_class == aggregate_obj )
                Export_Basetypes(outfile, inst->o_parent);

            fprintf(outfile, "// %s\n", inst->o_label);

            Export_Object_Type(outfile, inst);

            if ( inst->o_parent->b_class == plane_obj )
            {
                Vector  zero_v;
                Matrix  inverse;
                Vector  normal;
                double  temp_d;

                inverse = MInvert(&(inst->o_transform.matrix));
                VUnit(inverse.z, temp_d, normal);
                VNew(0, 0, 0, zero_v);
                fprintf(outfile, "\t");
                POVVPrint(normal);

                fprintf(outfile, "\t, %g\n",
                        -Distance_Point_To_Plane(&normal,
                                        &inst->o_transform.displacement,
                                        &zero_v));
            }

            if ( inst->o_attribs->use_obj_trans )
            {
                if ( inst->o_attribs->defined )
                    Export_Attributes(outfile, inst->o_attribs);
                else
                    Export_Attributes(outfile, def_attribs);
            }

            if ( inst->o_parent->b_class != plane_obj )
            {
                if ( sced_preferences.povray_v3 )
                    Export_Transform(outfile, inst, add_trans);
                else
                {
                    fprintf(outfile, "\ttransform transform_%ld\n",
                            (unsigned long)Hash_Get_Value(transform_hash,
                                        (unsigned long)&(inst->o_transform)));
                    fprintf(outfile, "\ttransform transform_%ld\n",
                            (unsigned long)Hash_Get_Value(transform_hash,
                                                    (unsigned long)add_trans));
                }
            }

            if ( ! inst->o_attribs->use_obj_trans )
            {
                if ( inst->o_attribs->defined )
                    Export_Attributes(outfile, inst->o_attribs);
                else
                    Export_Attributes(outfile, def_attribs);
            }

            if ( inst->o_attribs->open )
                fprintf(outfile, "\topen\n");

            /* Export bounds, if relevent. */
            if ( ! sced_preferences.povray_v3 &&
                 inst->o_parent->b_class == csg_obj )
            {
                Cuboid  bound;

                bound = Transform_Bound(
                         &(((CSGNodePtr)inst->o_parent->b_struct)->csg_bound),
                         &(inst->o_transform));
                fprintf(outfile, "\tbounded_by { box {\n");
                fprintf(outfile, "\t\t");
                POVVPrint(bound.min);
                fprintf(outfile, "\t\t,");
                POVVPrint(bound.max);
                fprintf(outfile, "\t\t}}\n");
            }

            fprintf(outfile, "}\n\n");
        }
    }

    return fprintf(outfile, "\n");
}


static void
Export_Object_Type(FILE *outfile, ObjectInstancePtr obj)
{
    int i;

    switch ( obj->o_parent->b_class )
    {
        case sphere_obj:
            fprintf(outfile, "sphere { <0,0,0>, 1.0\n");
            break;
        case cylinder_obj:
            fprintf(outfile, "cylinder { <0,0,-1>, <0,0,1>, 1.0\n");
            break;
        case cone_obj:
            fprintf(outfile, "cone { <0,0,-1>, 1, <0,0,1>, 0\n");
            break;
        case cube_obj:
            fprintf(outfile, "box { <-1,-1,-1>, <1,1,1>\n");
            break;
        case torus_obj:
            fprintf(outfile, "torus { %1.15g, 1.0\n\trotate <90, 0, 0>\n",
                    ((TorusPtr)obj->o_hook)->major_radius);
            break;
        case plane_obj:
            fprintf(outfile, "plane {\n");
            break;
        case triangle_obj:
            fprintf(outfile, "triangle {\n\t");
            POVVPrint(control_part(obj)->control_verts[0]);
            fprintf(outfile, "\t,");
            POVVPrint(control_part(obj)->control_verts[1]);
            fprintf(outfile, "\t,");
            POVVPrint(control_part(obj)->control_verts[2]);
            break;
        case bezier_obj:
            fprintf(outfile, "bicubic_patch {\n\ttype 1\n\tflatness 0\n\t");
            fprintf(outfile, "u_steps %d\n\tv_steps %d\n\t",
                    Wireframe_Density_Level(obj) + 2,
                    Wireframe_Density_Level(obj) + 2);
            for ( i = 0 ; i < 16 ; i++ )
            {
                POVVPrint(control_part(obj)->control_verts[bezier_map[i]]);
                if ( i < 15 ) 
                    fprintf(outfile, ",");
                fprintf(outfile, "\n\t");
            }
            break;
        case csg_obj:
        case wireframe_obj:
	case aggregate_obj:
            fprintf(outfile, "object { %s\n", obj->o_parent->b_label);
            break;

        default:;
    }
}



static void
Export_Attributes(FILE *outfile, AttributePtr attribs)
{
    if ( ! attribs || ! attribs->defined )
        return;

    if ( ! attribs->use_extension || ! attribs->extension[POVray] )
    {
        fprintf(outfile, "\tpigment\n");
        fprintf(outfile, "\t\t{ color red %g green %g blue %g filter %g}\n",
                attribs->color.red, attribs->color.green,
                attribs->color.blue, attribs->transparency);
        fprintf(outfile, "\tfinish {\n");
        fprintf(outfile, "\t\tdiffuse %g\n", attribs->diff_coef);
        fprintf(outfile, "\t\tambient ambient_light_level\n");
        fprintf(outfile, "\t\tspecular %g\n", attribs->spec_coef);
        fprintf(outfile, "\t\treflection %g\n", attribs->reflect_coef);
        fprintf(outfile, "\t\trefraction 1.0\n");
        fprintf(outfile, "\t\tior %g\n", attribs->refract_index);
        fprintf(outfile, "\t}\n");
    }
    else
        fprintf(outfile, "%s\n", attribs->extension[POVray]);
}



static int
Export_Declarations(FILE *outfile, ColorVector amb)
{
    double  ambient;

    /* Export ambient identifier. */
    ambient = ( amb.red + amb.green + amb.blue ) / 3.0;
    fprintf(outfile, "#declare ambient_light_level = %g;\n", ambient);

    /* This is simple at this stage. Just dump the string. */
    if ( declarations[POVray] )
        fprintf(outfile, "\n%s\n", declarations[POVray]);

    return fprintf(outfile, "\n");
}


