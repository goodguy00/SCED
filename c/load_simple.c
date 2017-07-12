/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/load_simple.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: load_simple.c,v $
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
**  load_simple.c: Functions for loading simple format files. These files
**                  are designed to be written by humans or other programs.
*/

#include <math.h>
#include <sced.h>
#include <aggregate.h>
#include <attributes.h>
#include <base_objects.h>
#include <csg.h>
#include <edge_table.h>
#include <instance_list.h>
#include <layers.h>
#include <load.h>
#include <View.h>
#include <Vector4.h>

#define Load_Float(f) \
    { \
        if ((token = yylex()) == INT_TOKEN) \
            f = (double)lex_int; \
        else if (token == FLOAT_TOKEN) \
            f = lex_float; \
        else \
        { \
            fprintf(stderr, "Malformed input file line %d: Float expected\n", \
                    line_num); \
            return ERROR_TOKEN; \
        } \
    }

#define Load_Vector(v) \
    { Load_Float((v).x); Load_Float((v).y); Load_Float((v).z); }

#define Load_Vector4(v) \
    { Load_Float((v).x); Load_Float((v).y); Load_Float((v).z); \
      Load_Float((v).w) }
#define Load_Matrix4(m) \
    { Load_Vector4((m).x); Load_Vector4((m).y); Load_Vector4((m).z); \
      Load_Vector4((m).w) }

static int  Load_Ambient(Boolean);
static int  Load_Simple_Instance(ObjectInstancePtr*, Boolean);
static int  Load_Simple_Object(ObjectInstancePtr);
static int  Load_Simple_CSG();
static int  Load_Simple_CSG_Tree(int, CSGNodePtr, CSGNodePtr*);
static int  Load_Simple_Wireframe();
static int  Load_Simple_Aggregate();


void
Load_Simple_File(FILE *file, int merge, int token)
{
    Viewport    camera_viewport;
    ObjectInstancePtr   dummy;

    if ( token == VERS_TOKEN )
    {
        if ((token = yylex()) == INT_TOKEN)
            version = (double)lex_int;
        else if (token == FLOAT_TOKEN)
            version = lex_float;
        else
        {
            fprintf(stderr, "Malformed input file line %d\n", line_num);
            return;
        }
        token = yylex();
    }

    while ( token != EOF_TOKEN && token != ERROR_TOKEN )
    {
        switch (token)
        {
            case VIEWPORT_TOKEN:
                if ( merge )
                    token = Load_View(NULL, NULL);
                else
                {
                    token = Load_View(&(main_window.viewport), NULL);
                    if ( main_window.view_widget &&
                         main_window.viewport.scr_width )
                        XtVaSetValues(main_window.view_widget,
                            XtNwidth, main_window.viewport.scr_width,
                            XtNdesiredWidth, main_window.viewport.scr_width,
                            XtNheight, main_window.viewport.scr_height,
                            XtNdesiredHeight, main_window.viewport.scr_height,
                            NULL);
                    main_window.magnify = main_window.viewport.magnify;
                }
                break;

            case CAMERA_TOKEN:
                token = Load_View(&camera_viewport, NULL);
                if ( ! merge )
                    Viewport_To_Camera(&camera_viewport,
                                       main_window.view_widget,
                                       &camera, FALSE);
                break;

            case TARGET_TOKEN:
                if ( ( token = yylex() ) == POVRAY_TOKEN )
                    target_renderer = POVray;
                else if ( token == RAYSHADE_TOKEN )
                    target_renderer = Rayshade;
                else if ( token == RADIANCE_TOKEN )
                    target_renderer = Radiance;
                else if ( token == RENDERMAN_TOKEN )
                    target_renderer = Renderman;
                else if ( token == LOAD_VRML_TOKEN )
                    target_renderer = VRML;
                else if ( token == HEADER_TOKEN )
                    fprintf(stderr,
                            "Header file output is no longer supported\n");
                else
                    fprintf(stderr, "Invalid target line %d\n", line_num);
                token = yylex();
                break;

            case DECLARE_TOKEN:
                if ( target_renderer == NoTarget )
                    fprintf(stderr, "Input file error: A Target must be active"
                            " before a Declare statement\n");
                Load_Declaration(target_renderer, merge);
                token = yylex();
                break;

            case OBJECT_TOKEN:
                token = Load_Simple_Instance(&dummy, FALSE);
                break;

            case CSG_TOKEN:
                token = Load_Simple_CSG();
                break;

            case WIREFRAME_TOKEN:
                token = Load_Simple_Wireframe();
                break;

            case AGG_TOKEN:
                token = Load_Simple_Aggregate();
                break;

            case AMBIENT_TOKEN:
                token = Load_Ambient(!merge);
                break;

            default:
                if ( merge )
                    fprintf(stderr, "Error token %d in file %s line %d\n",
                            token, merge_filename, line_num);
                else
                    fprintf(stderr, "Error token %d in file %s line %d\n",
                            token, io_file_name, line_num);
                token = yylex();
        }
    }

    View_Update(&main_window, main_window.all_instances, CalcView);
    Update_Projection_Extents(main_window.all_instances);
}


static int
Load_Colour(ColorVectorPtr col)
{
    int     token;
    double  r, g, b;

    Load_Float(r);
    Load_Float(g);
    Load_Float(b);

    col->red = r;
    col->green = g;
    col->blue = b;

    return COLOUR_TOKEN;
}

static int
Load_Ambient(Boolean set)
{
    ColorVector colour;

    if ( Load_Colour(&colour) == ERROR_TOKEN )
        return ERROR_TOKEN;

    if ( set )
        ambient_light = colour;

    return yylex();
}

static int
Load_Simple_Instance(ObjectInstancePtr *obj, Boolean csg)
{
    char            *obj_name;
    BaseObjectPtr   base;
    ConstraintPtr   f;
    int             token;
    int             i, j;

    /* Load the name. */
    if ( ( token = yylex() ) != STRING_TOKEN )
    {
        fprintf(stderr, "Malformed input file line %d: Expected object name\n",
                line_num);
        return ERROR_TOKEN;
    }
    obj_name = lex_string;

    /* Load the base type. */
    if ( ( token = yylex() ) != STRING_TOKEN )
    {
        fprintf(stderr, "Malformed input file line %d: Expected object type\n",
                line_num);
        return ERROR_TOKEN;
    }
    base = Get_Base_Object_From_Label(lex_string);
    if ( ! base )
    {
        fprintf(stderr, "Unable to find base \"%s\", line %d\n",
                lex_string, line_num);
        return ERROR_TOKEN;
    }
    free(lex_string);

    /* Create the object. */
    (*obj) = Create_Instance(base, obj_name);
    free((*obj)->o_label);
    (*obj)->o_label = obj_name;
    if ( ! csg )
    {
        Append_Element(&(main_window.all_instances), *obj);
        if ( Layer_Is_Visible((*obj)->o_layer) )
            (*obj)->o_flags |= ObjVisible;
    }

    if ( Obj_Is_Torus((*obj)) )
    {
        Load_Float(((TorusPtr)(*obj)->o_hook)->major_radius);
        VNew(((TorusPtr)(*obj)->o_hook)->major_radius, 0, 0,
             (*obj)->o_features[radius_feature].location);
        (*obj)->o_features[radius_feature].constraints[0].c_vector =
            (*obj)->o_features[radius_feature].location;
    }

    if ( Obj_Is_Control((*obj)) )
        for ( i = 0 ; i < control_part(*obj)->num_control_verts ; i++ )
        {
            Load_Vector(control_part(*obj)->control_verts[i]);
            (*obj)->o_features[pt0_feature + i].location =
                control_part(*obj)->control_verts[i];
            for ( j = 0 ; j < 3 ; j++ )
            {
                f = (*obj)->o_features[pt0_feature + i].constraints + j;
                f->c_point = control_part(*obj)->control_verts[i];
                f->c_value = VDot(f->c_vector, f->c_point);
            }
        }

    return Load_Simple_Object(*obj);
}


static int
Load_Simple_Object(ObjectInstancePtr obj)
{
    Transformation  obj_trans;
    Matrix4     transform;
    Matrix4     new_matrix;
    Matrix      temp_m;
    Vector      new_vector;
    Attributes  attributes = sced_preferences.default_attributes;
    Boolean     done = FALSE;
    Renderer    rend;
    int         s1, s2;
    int         token;

    NewIdentityMatrix4(transform);
    attributes.defined = FALSE;

    while ( ! done )
    {
        switch ( token = yylex() )
        {
            case MATRIX_TOKEN:
                Load_Matrix4(new_matrix);
                transform = M4M4Mul(&new_matrix, &transform);
                break;

            case SCALE_TOKEN:
                Load_Vector(new_vector);
                NewIdentityMatrix4(new_matrix);
                new_matrix.x.x = new_vector.x;
                new_matrix.y.y = new_vector.y;
                new_matrix.z.z = new_vector.z;
                transform = M4M4Mul(&new_matrix, &transform);
                break;

            case ROTATE_TOKEN:
                Load_Vector(new_vector);
                Vector_To_Rotation_Matrix(&new_vector, &temp_m);
                NewIdentityMatrix4(new_matrix);
                MToM4(temp_m, new_matrix);
                transform = M4M4Mul(&new_matrix, &transform);
                break;

            case POSITION_TOKEN:
                Load_Vector(new_vector);
                VAdd(new_vector, transform.w, transform.w);
                break;

            case DENSE_TOKEN:
                if ( ( token = yylex() ) != INT_TOKEN )
                {
                    fprintf(stderr,
                            "Malformed input file line %d: Expected integer\n",
                            line_num);
                    return ERROR_TOKEN;
                }
                Object_Change_Wire_Level(obj, (int)lex_int);
                break;

            case SPECULAR_TOKEN:
                attributes.defined = TRUE;
                Load_Float(attributes.spec_coef);
                Load_Float(attributes.spec_power);
                break;

            case DIFFUSE_TOKEN:
                attributes.defined = TRUE;
                Load_Float(attributes.diff_coef);
                break;

            case REFLECT_TOKEN:
                attributes.defined = TRUE;
                Load_Float(attributes.reflect_coef);
                break;

            case REFRACT_TOKEN:
                attributes.defined = TRUE;
                Load_Float(attributes.refract_index);
                break;

            case TRANSPARENCY_TOKEN:
                attributes.defined = TRUE;
                Load_Float(attributes.transparency);
                break;

            case COLOUR_TOKEN:
                attributes.defined = TRUE;
                if ( Load_Colour(&(attributes.color)) == ERROR_TOKEN )
                    return ERROR_TOKEN;
                break;

            case INTENSITY_TOKEN:
                attributes.defined = TRUE;
                if ( Load_Colour(&(attributes.intensity)) == ERROR_TOKEN )
                    return ERROR_TOKEN;
                break;

            case SAMPLE_TOKEN:
                attributes.defined = TRUE;
                if ( ( token = yylex() ) != INT_TOKEN )
                    return ERROR_TOKEN;
                s1 = lex_int;
                if ( ( token = yylex() ) != INT_TOKEN )
                    return ERROR_TOKEN;
                s2 = lex_int;
                attributes.samples = ( s1 & 0xFF ) | ( s2 << 8 );
                break;

            case RADIUS_TOKEN:
                attributes.defined = TRUE;
                Load_Float(attributes.radius);
                break;

            case TIGHTNESS_TOKEN:
                attributes.defined = TRUE;
                Load_Float(attributes.tightness);
                break;

            case JITTER_TOKEN:
                attributes.defined = TRUE;
                attributes.jitter = TRUE;
                break;

            case INVERT_TOKEN:
                attributes.defined = TRUE;
                attributes.invert = TRUE;
                break;

            case EXTEND_TOKEN:
                if ( Obj_Is_Light(obj) )
                    break;
                if ( ( token = yylex() ) != STRING_TOKEN )
                {
                    fprintf(stderr,
                            "Malformed input file line %d: Expected string\n",
                            line_num);
                    return ERROR_TOKEN;
                }
                attributes.defined = TRUE;
                attributes.use_extension = TRUE;
                attributes.extension[target_renderer] = lex_string;
                break;

            default: done = TRUE;
        }
    }

    M4ToM(transform, obj_trans.matrix);
    VNew(transform.w.x, transform.w.y, transform.w.z, obj_trans.displacement);
    Transform_Instance(obj, &obj_trans, TRUE);
    if ( attributes.defined )
        *(obj->o_attribs) = attributes;
    for ( rend = NoTarget ; rend < LastTarget ; rend++ )
        obj->o_attribs->extension[rend] = NULL;
    obj->o_attribs->extension[target_renderer] =
        attributes.extension[target_renderer];

    return token;
}


static int
Load_Simple_CSG_Tree(int token, CSGNodePtr parent, CSGNodePtr *ret_tree)
{
    *ret_tree = New(CSGNode, 1);
    (*ret_tree)->csg_parent = parent;
    (*ret_tree)->csg_widget = NULL;

    switch ( token )
    {
        case UNION_TOKEN:
            (*ret_tree)->csg_op = csg_union_op;
            return Load_Simple_CSG_Tree(
                        Load_Simple_CSG_Tree(yylex(), *ret_tree,
                                             &((*ret_tree)->csg_left_child)),
                                             *ret_tree,
                                             &((*ret_tree)->csg_right_child));

        case INTERSECTION_TOKEN:
            (*ret_tree)->csg_op = csg_intersection_op;
            return Load_Simple_CSG_Tree(
                        Load_Simple_CSG_Tree(yylex(), *ret_tree,
                                             &((*ret_tree)->csg_left_child)),
                                             *ret_tree,
                                             &((*ret_tree)->csg_right_child));

        case DIFFERENCE_TOKEN:
            (*ret_tree)->csg_op = csg_difference_op;
            return Load_Simple_CSG_Tree(
                        Load_Simple_CSG_Tree(yylex(), *ret_tree,
                                             &((*ret_tree)->csg_left_child)),
                                             *ret_tree,
                                             &((*ret_tree)->csg_right_child));

        case OBJECT_TOKEN:
            (*ret_tree)->csg_op = csg_leaf_op;
            return Load_Simple_Instance(&((*ret_tree)->csg_instance), TRUE);

        default:
            fprintf(stderr,
                    "Malformed input file line %d: Expected CSG tree element\n",
                    line_num);
            return ERROR_TOKEN;
    }
}

static int
Load_Simple_CSG()
{
    char            *csg_name;
    CSGNodePtr      tree;
    BaseObjectPtr   new_base;
    int             token;
    WireframePtr    *wire = New(WireframePtr, 1);

    if ( ( token = yylex() ) != STRING_TOKEN )
    {
        fprintf(stderr, "Malformed input file line %d: Expected string\n",
                line_num);
        return EOF_TOKEN;
    }
    csg_name = lex_string;

    if ( ( token = Load_Simple_CSG_Tree(yylex(), NULL, &tree) ) == ERROR_TOKEN )
        return token;

    wire[0] = CSG_Generate_Wireframe(tree, 0, FALSE);
    new_base = Add_CSG_Base_Object(tree, csg_name, 0, wire, NULL, 0, FALSE);
    free(csg_name);
    Base_Add_Select_Option(new_base);

    return token;
}


static int
Load_Simple_Aggregate()
{
    char                *agg_name;
    InstanceList        agg_insts = NULL;
    ObjectInstancePtr   obj;
    WireframePtr        wire;
    int                 token;

    if ( ( token = yylex() ) != STRING_TOKEN )
    {
        fprintf(stderr, "Malformed input file line %d: Expected string\n",
                line_num);
        return EOF_TOKEN;
    }
    agg_name = lex_string;

    token = yylex();
    while ( token == OBJECT_TOKEN )
    {
        token = Load_Simple_Instance(&obj, TRUE);
        if ( token == ERROR_TOKEN )
            return EOF_TOKEN;
        Append_Element(&agg_insts, obj);
        if ( Layer_Is_Visible(obj->o_layer) )
            obj->o_flags |= ObjVisible;
    }

    if ( token != AGG_TOKEN )
    {
        fprintf(stderr, "Malformed input file line %d: Expected Aggregate\n",
                line_num);
        return EOF_TOKEN;
    }

    wire = Agg_Generate_Wireframe(agg_insts, 0, AGG_ALL);
    Edge_Table_Build(wire);
    Base_Add_Select_Option(
	Add_Agg_Base_Object(agg_insts, agg_name, wire, NULL, 0));

    return yylex();
}


static int
Load_Simple_Wireframe()
{
    char            *wire_name;
    WireframePtr    wireframe = New(Wireframe, 1);
    BaseObjectPtr   res;
    int             token;
    int             i, j;
    Vector          v1, v2;
    double          temp_d;

    if ( ( token = yylex() ) != STRING_TOKEN )
    {
        fprintf(stderr, "Malformed input file line %d: Expected string\n",
                line_num);
        return EOF_TOKEN;
    }
    wire_name = lex_string;

    /* Get number of vertices. */
    if ( ( token = yylex() ) != INT_TOKEN )
    {
        fprintf(stderr, "Malformed input file line %d: Expected integer\n",
                line_num);
        return EOF_TOKEN;
    }
    wireframe->num_real_verts = (int)lex_int;
    wireframe->num_vertices = (int)lex_int + 1;

    if ( version >= 1.0 )
    {
        if ( ( token = yylex() ) != INT_TOKEN )
        {
            fprintf(stderr, "Malformed input file line %d: "
                    "Expected integer number of normals\n",
                    line_num);
            return EOF_TOKEN;
        }
        wireframe->num_normals = (int)lex_int;
    }
    else
        wireframe->num_normals = 0;


    /* Get number of faces. */
    if ( ( token = yylex() ) != INT_TOKEN )
    {
        fprintf(stderr, "Malformed input file line %d: Expected integer\n",
                line_num);
        return EOF_TOKEN;
    }
    wireframe->num_faces = (int)lex_int;

    wireframe->vertices = New(Vector, wireframe->num_vertices);
    for ( i = 0 ; i < wireframe->num_real_verts ; i++ )
        Load_Vector(wireframe->vertices[i])
    VNew(0, 0, 0, wireframe->vertices[i]);

    if ( wireframe->num_normals > 0 )
    {
        wireframe->normals = New(Vector, wireframe->num_normals);
        for ( i = 0 ; i < wireframe->num_normals ; i++ )
            Load_Vector(wireframe->normals[i])
    }
    else
        wireframe->normals = NULL;

    wireframe->faces = New(Face, wireframe->num_faces);
    for ( i = 0 ; i < wireframe->num_faces ; i++ )
    {
        if ( ( token = yylex() ) != INT_TOKEN )
        {
            fprintf(stderr, "Malformed input file line %d: Expected integer\n",
                    line_num);
            return EOF_TOKEN;
        }
        wireframe->faces[i].num_vertices = (int)lex_int;
        wireframe->faces[i].vertices =
            New(int, wireframe->faces[i].num_vertices);
        wireframe->faces[i].normals =
            New(int, wireframe->faces[i].num_vertices);
        for ( j = 0 ; j < wireframe->faces[i].num_vertices ; j++ )
        {
            if ( ( token = yylex() ) != INT_TOKEN )
            {
                fprintf(stderr,
                        "Malformed input file line %d: "
                        "Expected integer vertex %d\n",
                        line_num, j);
                return EOF_TOKEN;
            }
            wireframe->faces[i].vertices[j] = (int)lex_int;
            if ( wireframe->num_normals )
            {
                if ( ( token = yylex() ) != INT_TOKEN )
                {
                    fprintf(stderr,
                            "Malformed input file line %d: "
                            "Expected integer normal %d\n",
                            line_num, j);
                    return EOF_TOKEN;
                }
                wireframe->faces[i].normals[j] = (int)lex_int;
            }
        }

        if ( wireframe->faces[i].num_vertices > 2 )
        {
            VSub(wireframe->vertices[wireframe->faces[i].vertices[2]],
                 wireframe->vertices[wireframe->faces[i].vertices[0]], v1);
            VSub(wireframe->vertices[wireframe->faces[i].vertices[1]],
                 wireframe->vertices[wireframe->faces[i].vertices[0]], v2);
            VCross(v1, v2, wireframe->faces[i].normal);
            VUnit(wireframe->faces[i].normal, temp_d,
                  wireframe->faces[i].normal);
        }

        wireframe->faces[i].face_attribs = NULL;
    }

    if ( wireframe->num_normals == 0 )
        Wireframe_Assign_Vertex_Normals(wireframe, 45.0 * M_PI / 180.0);

    wireframe->attribs = NULL;
    wireframe->num_attribs = 0;
    wireframe->edges = NULL;

    res = Add_Wireframe_Base_Object(wire_name, wireframe, NULL, 0);
    Base_Add_Select_Option(res);

    return yylex();
}


