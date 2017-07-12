/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/h/RCS/types.h,v 1.2 1997/06/11 21:12:52 schenney Exp $
 *
 * $Log: types.h,v $
 * Revision 1.2  1997/06/11 21:12:52  schenney
 * Added vertex normal calculation function for control pt objects.
 *
 * Revision 1.0  1997/05/06 20:42:49  schenney
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
**  types.h: Header file containing type declarations and definitions.
**
**  Created: 04/03/94
*/

#ifndef __SCED_TYPES__
#define __SCED_TYPES__



/* An linear transformation type, consisting of an affine   */
/*  transformation matrix (for scaling, rotating etc) and a */
/*  displacement vector for positioning in space.       */
typedef struct _Transformation {
    Matrix  matrix;
    Vector  displacement;
    } Transformation, *TransformPtr;



/*  A Viewport specification type.  */
typedef struct _Viewport {
    Vector          view_from;
    Vector          view_at;
    Vector          view_up;
    double          view_distance;
    double          eye_distance;
    Transformation  world_to_view;
    Transformation  view_to_world;
    Vector          eye_position;
    int             draw_mode;
    Dimension       scr_width;
    Dimension       scr_height;
    double          magnify;
    Boolean         is_default;
    } Viewport, *ViewportPtr;


/* A Camera structure.
*/
typedef struct _Camera {
    Boolean     defined;    /* Whether or not the camera has been defined. */
    Vector      location;   /* eyep, location or eye_position. */
    Vector      look_at;    /* lookp, look_at or lookat. */
    Vector      look_up;    /* up, sky or lookup. */
    float       horiz_fov;  /* fov in Rayshade. */
    float       vert_fov;   /* fov in Rayshade. */
    float       eye_dist;   /* focal_dist in Ray. */
    Dimension   scr_width;  /* mainly to allow for save/load. */
    Dimension   scr_height;
    } Camera;


/* A Cuboid is the 3D equivalent of a Rectangle.    */
typedef struct _Cuboid {
    Vector min;
    Vector max;
    } Cuboid;

/* Need a type for projection extents. */
typedef struct _Extent2D {
    XPoint min;
    XPoint max;
    } Extent2D;


/* Specifications for a point used in a constraint definition. */
typedef struct _ConstraintSpec {
    ConstraintSpecType  spec_type;      /* The type of spec point this is.*/
    void                *spec_data;     /* Extra info. Maybe an index or obj.*/
    Vector              spec_vector;    /* The vector describing the pt.  */
    } ConstraintSpec, *ConstraintSpecPtr;

/* Data for specifying constraints. */
typedef struct _ConstraintData {
    ConstraintType      c_type;     /* The nature of the feature eg plane.  */
    char                *c_label;   /* The label attached.                  */
    Boolean             c_status;   /* Whether or not it's active.          */
    Boolean             c_forced;   /* Whether or not it was forced.        */
    Vector              c_vector;   /* The defining vector. Normal for a
                                    ** plane, direction for a line. Undefined
                                    ** for a point.
                                    */
    Vector              c_point;    /* A specifying point. Any point on a
                                    ** plane or line, the point for a point.
                                    */
    double              c_value;    /* Special value. Norm.Point for a plane.*/
    double              c_radius;   /* Radius for spheres and circles. */
                                    /* The dist to the first pt for a ratio. */
    double              c_ratio;    /* The dist to the second pt for a ratio.*/
    int                 c_spec_flags;   /* Flags for specification. */
    int                 c_num_specs;/* The number of specifying points. */
    ConstraintSpecPtr   c_specs;    /* Specifications of the feature's
                                    ** defining points.
                                    */
    } ConstraintData, *ConstraintPtr;


/* A type for resultant constraints. */
typedef struct _Resultant {
    ConstraintData  feature_1;
    ConstraintData  feature_2;
    } Resultant, *ResultantPtr;


/* Forward type declarations. */
typedef struct _FeatureBase*    FeatureBasePtr;
typedef struct _Feature*        FeaturePtr;
typedef struct _EditInfo*       EditInfoPtr;
typedef struct _WindowInfo*     WindowInfoPtr;
typedef struct _ObjectInstance* ObjectInstancePtr;


/* Function type declarations for the feature type. */
typedef void    (*FeatureDrawFunction)(int, FeaturePtr, WindowInfoPtr,
                                       int, EditInfoPtr);
typedef void    (*FeatureCDrawFunction)(int, FeaturePtr, WindowInfoPtr, int,
                                        EditInfoPtr, Boolean);
typedef void    (*FeatureInitFunction)(FeatureBasePtr);
typedef Boolean (*FeatureSelectFunction)(int, int, int, EditInfoPtr);
typedef Boolean (*FeatureActiveFunction)(int, EditInfoPtr);
typedef void    (*FeatureDragFunction)(XEvent*, int, EditInfoPtr);
typedef void    (*FeatureSolveFunction)(ObjectInstancePtr, ConstraintPtr, int,
                                        ResultantPtr);
typedef Boolean (*FeatureSatisfyFunction)(EditInfoPtr, int, Boolean);

/* A base type for constrained features. This structure contains information
** about drawing the feature and constraint box information.
*/
typedef struct _FeatureBase {
    Boolean                 draw_initialised;
    FeatureInitFunction     draw_init_func; /* Drawing init function. */
    FeatureDrawFunction     draw_func;      /* Redraw procedure. */
    FeatureCDrawFunction    draw_con_func;  /* Redraw constraints function. */
    GC                      draw_gc1;       /* GC for one thing. */
    GC                      draw_gc2;       /* GC to draw another thing. */

    FeatureSelectFunction   select_func;    /* Selection function. */
    Cursor                  select_cursor;  /* Selected cursor. */
    int                     select_priority;/* Priority for selection. */
    FeatureActiveFunction   select_active;  /* Interaction allowed? */

    FeatureSelectFunction   covers_func;    /* Pointer over function.   */
    Widget                  add_shell;  /* Shell for popup add constraint.  */
    Widget                  change_shell;   /* Shell for popup change.  */

    FeatureDragFunction     drag_start_func;    /* Drag functions. */
    FeatureDragFunction     drag_func;
    FeatureDragFunction     drag_end_func;

    Boolean                 check_forced;   /* Whether a forced option exists*/
    FeatureSolveFunction    solve_function; /* Function to solve constr syst.*/
    FeatureSatisfyFunction  satisfy_function;   /* Force satisfaction. */

    int     num_defaults;           /* The number of default constraints. */

    } FeatureBase;

/* A type for constrained Features. */
typedef struct _Feature {
    FeatureBasePtr  base;
    Vector          location;
    int             num_constraints;
    ConstraintPtr   constraints;
    int             flags;
    } Feature;


/*  A Vertex has coordinates in view space and screen space.    */
typedef struct _Vertex {
    Vector  view;
    XPoint  screen;
    Boolean drawn;
    } Vertex, VertexPtr;

typedef struct _ColorVector {
    float   red;
    float   green;
    float   blue;
    } ColorVector, *ColorVectorPtr;

/* An attribute type.   */
typedef struct _Attributes {
    Boolean     defined;        /* Whether attributes have been explicitly set*/
    ColorVector color;          /* Obvious. */
    float       diff_coef;      /* Diffuse lighting coeficient. */
    float       spec_coef;      /* Specular lighting coeficient. */
    float       spec_power;     /* Specular power. */
    float       reflect_coef;   /* Coeficient of reflection. */
    float       transparency;   /* Degree of transparency. */
    float       refract_index;  /* Index of refraction squared. */
    Boolean     use_extension;  /* Whether to use the extension field. */
    char        *extension[LastTarget];     /* A string of additional info, one
                                            ** for each renderer.           */
    Boolean     open;           /* For cylinders and cones. */
    Boolean     use_obj_trans;  /* Whether to transform textures with the obj.*/
    ColorVector intensity;      /* Light color. */
    float       radius;         /* Spotlights. */
    float       tightness;      /* Spotlights. */
    short       samples;        /* Area lights. Stored as low and high 8 bits.*/
    Boolean     jitter;         /* Arealights. */
    Boolean     invert;         /* Spotlights. */
    } Attributes, *AttributePtr;

/* The wireframe format. */
/* It uses the classic structure with lists of vertices and lists of faces,
** each face having an ordered list of vertex indices.
** Face vertices are clockwise about the normal, in defiance of all convention.
*/
typedef struct _Face {
    int     	    *vertices;      /* Clockwise ordered vertex indices. */
    int     	    *normals;       /* Corresponding vertex normals. */
    int     	    num_vertices;
    Vector  	    normal;         /* A face normal. */
    AttributePtr    face_attribs;   /* A pointer to this face's attributes. */
    } Face, *FacePtr;

/* A type for edge lists. */
typedef struct _EdgeListElmt {
    int     endpt;
    void    *val;
    } Edge, *EdgePtr;

typedef struct _EdgeTableElmt {
    EdgePtr edges;
    int     num_edges;
    int     max_num_edges;
    } EdgeTableElmt, *EdgeTable;
    
typedef struct _Wireframe {
    Vector          *vertices;
    int             num_vertices;
    int             num_real_verts; /* The number of vertices that have edges.*/
    Vector          *normals;       /* Vertex normals */
    int             num_normals;
    EdgeTable       edges;          /* An edge table, if required. */
    FacePtr         faces;
    int             num_faces;
    AttributePtr    *attribs;   /* Face attributes, generally NULL. */
    int             num_attribs;
    } Wireframe, *WireframePtr;


typedef struct _LODInfo {
    int     num_lods;
    float   *lods;
    } LODInfo, *LODInfoPtr;

/* A type to hold dependency info. */
typedef struct _Dependent {
    struct _ObjectInstance  *obj;
    char                    count;
    } Dependent, *DependentList;

typedef void    (*InstanceFunction)(struct _ObjectInstance*);

typedef struct _CSGNode *CSGNodePtr;

/*  A BaseObject type.  Instances inherit from a base object.   */
typedef struct _BaseObject {
    String          b_label;        /* A name for the base class.           */
    GenericObject   b_class;        /* The type of object this is.          */
    int             b_ref_num;      /* The default reference for instances. */
    void            *b_struct;      /* A pointer to structural info.        */
    WireframePtr    *b_major_wires; /* Full triangulated wireframes.	    */
    WireframePtr    *b_wireframes;  /* The simplified version.              */
    short           b_max_density;  /* The number of dense_wireframes.      */
    Boolean         b_use_full;     /* Use the full wireframe, not csg.     */
    int             b_num_instances;/* The number of instances of this type.*/
    int             b_num_slots;    /* The number of spaces available.      */
    struct _ObjectInstance  **b_instances;  /* An array of pointers to      */
                                            /* instances.                   */
    InstanceFunction    b_create_func;  /* Function for creating the base   */
                                        /* dependent parts of an instance.  */
    InstanceFunction    b_destroy_func; /* Function for destroying same.    */
    } BaseObject, *BaseObjectPtr;


typedef void (*ConstrDynamicFunc)(struct _EditInfo*, int, VectorPtr,
                                  Transformation*);
typedef Boolean (*ConstrStaticFunc)(struct _ObjectInstance*);

/*  A structure for a particular  object instance.  */
typedef struct _ObjectInstance {
    String          o_label;        /* A name for the object.               */
    BaseObject      *o_parent;      /* The base type for this object.       */
    Wireframe       *o_wireframe;   /* Its wireframe.                       */
    Transformation  o_transform;    /* The transform from generic to world. */
    Matrix          o_inverse;      /* The transform from world to generic. */
    AttributePtr    o_attribs;      /* Visual properties.                   */
    void            **o_aliases;    /* A set of aliases for the object.     */
    LODInfoPtr      o_lods;         /* LOD distances for VRML.              */
    Extent2D        o_proj_extent;  /* The projection extent.               */
    int             o_layer;        /* Display layer number.                */
    int             o_num_vertices; /* The number of verts in its wireframe.*/
    int             o_num_real;     /* The real number of vertices. This is */
                                    /*  the number that belong to edges.    */
    Vector          *o_world_verts; /* The vertices in world.               */
    Vertex          *o_main_verts;  /* The array of wireframe main vertices.*/
    int             o_num_faces;    /* The number of faces on the object.   */
    Vector          *o_normals;     /* The normals for the faces.           */
    int             o_num_features; /* the number of constrained features.  */
    FeaturePtr      o_features;     /* The constrained features.            */
    ConstrDynamicFunc   o_dynamic_func; /* Function for dynamic maintenance */
    ConstrStaticFunc    o_static_func;  /* Function for static maintenance. */
    DependentList   o_dependents;   /* An array of objects with constraints */
                                    /* depending on this object. To allow   */
    short           o_num_depend;   /* constraint maintainence.             */
    void            *o_hook;        /* A hook for additional information.   */
    int             o_flags;        /* Bit flags for visibility, selection etc*/
    unsigned long   o_dfs_mark;     /* A value needed for depth-first-search*/
    } ObjectInstance;


/* A hook structure for Torus objects. */
typedef struct _TorusHook {
    double          major_radius;   /* The mean radiius. */
    } TorusHook, *TorusPtr;

/* A hook structure for triangle and patch objects, those defined by "control
** points of some type.
*/
typedef	void	(*ControlVertFunc)(ObjectInstancePtr, VectorPtr, int, int);
typedef	void	(*ControlVertNormFunc)(ObjectInstancePtr, VectorPtr, int);
typedef	void	(*ControlFaceNormFunc)(ObjectInstancePtr, VectorPtr);
typedef struct _ControlHook {
    Vector  	    	*control_verts;
    int     	    	num_control_verts;
    ControlVertFunc 	calc_verts;
    ControlFaceNormFunc	calc_face_norms;
    ControlVertNormFunc calc_vert_norms;
    } ControlHook, *ControlHookPtr;

/*  A structure for maintaining lists of object instances. */
typedef struct _InstanceListElmt {
    ObjectInstancePtr           the_instance;
    struct _InstanceListElmt    *next;
    struct _InstanceListElmt    *prev;
    } InstanceListElmt, *InstanceList;


/*  A structure for floating menus ie menus which get larger and smaller. */
typedef struct _MenuInfo {
    Widget  menu;
    Widget  *children;
    short   num_children;
    short   max_children;
    Widget  button;
    } MenuInfo, *MenuInfoPtr;


/* The various states a window can be in.
** Which events go where, and the reaction to certain events depends on this.
*/
typedef int StateType;


/* A structure for containing window information.  By window information
** I mean viewports, widgets, etc.
*/
typedef struct _WindowInfo {
    Widget      shell;          /* The window shell.                        */
    Widget      view_widget;    /* The view widget inside it.               */
    Viewport    viewport;       /* The viewport for the view widget.        */
    ObjectInstance  axes;       /* The axes for the window.                 */
    char        *text_string;   /* The string for inputing text.            */
    Widget      text_widget;    /* The text widget for the above string.    */
    Widget      text_label;     /* The label at the bottom.                 */
    Widget      apply_button;   /* The apply button for the text.           */
    MenuInfoPtr edit_menu;      /* The menu for edit.                       */
    InstanceList    all_instances;  /* All the instances for the window.    */
    InstanceList    selected_instances; /* Those selected.                  */
    InstanceList    edit_instances;     /* All those pending editing.       */
    StateType   current_state;  /* The current state for the window.        */
    Pixmap      off_screen;     /* Off screen bitmap for drawing into.      */
    Dimension   width;
    Dimension   height;         /* Current width and height.                */
    double      magnify;        /* Magnification value.                     */
    } WindowInfo;


/*  A structure for passing scene information to the export functions. */
typedef struct _SceneStruct {
    Renderer            target;
    Camera              camera;
    ObjectInstancePtr   light;
    ColorVector         ambient;
    InstanceList        instances;
    } SceneStruct, *ScenePtr;


/* A function type for functions that manipulate constraint specifiers. */
/* The first arg is always the constraint that needs manipulating.
** The second argument is always the object that owns the constraint.
** The rest are dependent on the function.
*/
typedef void (*SpecFunction)(ConstraintSpecPtr, ObjectInstancePtr,
                             void*, void*, int);

/* A function type for functions that test conditions on objects. */
/* The first arg is always the thing that needs testing, the second is
** arbitrary.
*/
typedef Boolean (*ObjectTestFunction)(ObjectInstancePtr, void*);

/* We need this for some function declarations. */
typedef struct _HashEntry **HashTable;


/* Application resources structure. */
typedef struct _ScedResources {
    Pixel   x_axis_color;
    Pixel   y_axis_color;
    Pixel   z_axis_color;
    int     axis_width;
    int     x_axis_length;
    int     y_axis_length;
    int     z_axis_length;
    int     axis_denom;
    XFontStruct *axis_font;
    Pixel   obj_x_axis_color;
    Pixel   obj_y_axis_color;
    Pixel   obj_z_axis_color;
    int     obj_axis_width;
    int     obj_x_axis_length;
    int     obj_y_axis_length;
    int     obj_z_axis_length;
    int     obj_axis_denom;
    int     edit_pt_rad;
    int     control_pt_rad;
    Pixel   scaling_color;
    Pixel   origin_color;
    Pixel   torus_color;
    Pixel   control_color;
    Pixel   ctext_color;
    XFontStruct *control_font;
    Pixel   object_color;
    Pixel   selected_color;
    int     selected_width;
    Pixel   light_color;
    int     light_pt_rad;
    Pixel   camera_color;
    Pixel   constraint_color;
    int     plane_con_length;
    int     line_con_length;
    int     incon_con_length;
    int     point_con_rad;
    int     origin_con_width;
    int     scale_con_width;
    int     torus_con_width;
    int     rotate_con_width;
    int     control_con_width;
    Pixel   referenced_color;
    Pixel   active_color;
    Pixel   selected_pt_color;
    int     select_pt_width;
    int     select_pt_line_width;
    Pixel   absolute_color;
    Pixel   offset_color;
    Pixel   reference_color;
    Pixel   vertex_color;
    Pixel   arcball_color;
    Pixel   construct_color;
    Pixel   aliased_color;
    XtOrientation   edit_orientation;
    int     edit_rows;
    int     edit_columns;
    } ScedResources;

/* Preferences grouped together. */
typedef struct _ScedPreferences {
    int         autosave_time;
    Boolean     compress_output;
    Boolean     save_simple_wires;
    Boolean     povray_v3;
    char        *scene_path;
    Attributes  default_attributes;
    } ScedPreferences, *ScedPreferencesPtr;

#endif
