/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/elk/RCS/object3d.c,v 1.0 1997/05/06 20:44:40 schenney Exp $
 *
 * $Log: object3d.c,v $
 * Revision 1.0  1997/05/06 20:44:40  schenney
 * Initial revision
 *
 */
#include <math.h>
#include "elk_private.h"
#include <edit.h>   /* For constraint maintenance. */


Object  Sym_Union;
Object  Sym_Intersection;
Object  Sym_Difference;


/***********************************************************************
 *
 * Description: Elk_Object3d_print() is used internally by the scheme
 *      interpreter. This function is called whenever a user
 *      tries to print an object from scheme.
 *
 * Scheme example: (display (object3d-create "sphere")) => #[object3 0x1a4c00]
 *
 ***********************************************************************/
int
Elk_Object3d_Print(Object w, Object port, int raw, int depth, int len)
{
    ObjectInstancePtr object3 = (ObjectInstancePtr) ELKOBJECT3(w)->object3;
    
    Printf(port, "#[object3 0x%x]\n", object3);
    return 0;
}

/***********************************************************************
 *
 * Description: Elk_Object3d_equal() is used internally by the scheme
 *      interpreter. This function is called whenever two objects
 *      are compared in scheme using the "equal?" operator.
 *
 * Scheme example: (equal?
 *          (object3d-create "sphere")
 *          (object3d-create "sphere"))
 *
 ***********************************************************************/
int
Elk_Object3d_Equal(Object a, Object b)
{
    return (ELKOBJECT3(a)->object3 == ELKOBJECT3(b)->object3);
}

/***********************************************************************
 *
 * Description: Elk_Object3d_equiv() is used internally by the scheme
 *      interpreter. This function is called whenever two objects
 *      are compared in scheme using the "eqv?" operator.
 *
 * Scheme example: (eqv?
 *          (object3d-create "sphere")
 *          (object3d-create "sphere"))
 *
 ***********************************************************************/
int
Elk_Object3d_Equiv(Object a, Object b)
{
    return (ELKOBJECT3(a)->object3 == ELKOBJECT3(b)->object3);
}

/***********************************************************************
 *
 * Description: Elk_Object3d_create() is the C callback for the scheme
 *      function "object3d-create".
 *
 * Parameter:   strobj - string object specifying type of object to
 *      be created. This parameter sould match the name of one of the
 *      base objects.
 *
 * Scheme example: (object3d-create "sphere")
 *
 * Return value: Returns the newly created object. This object may be
 *       passed to other scheme functions that accept an object
 *       parameter.
 *
 ***********************************************************************/
Object
Elk_Object3d_Create(Object strobj)
{
    Object obj;
    char *objtype;
    ObjectInstancePtr new_obj;
    BaseObjectPtr base_obj;

    /*
     * First check to make sure that the type of strobj
     * is a string.
     */
    Check_Type(strobj, T_String);
    
    /*
     * Next we allocate an Elk object to save the real object in.
     */
    obj = Alloc_Object(sizeof(Elkobject3), T_Object3d, 0);
    objtype = STRING(strobj)->data;
    
    /*
     * Determine the type of base object to use.
     */
    if ( ! ( base_obj = Get_Base_Object_From_Label(objtype) ) )
    {
        Primitive_Error("Unknown base object: ~s", base_obj);
        return Void;    /* unknown object type */
    }

    /*
     * Create an instance of the base object.
     */
    new_obj = Create_New_Object_From_Base(elk_window, base_obj, FALSE);

    ELKOBJECT3(obj)->object3 = new_obj;

    return obj;
}


static void
Elk_Transform_Instance(ObjectInstancePtr obj, TransformPtr t, Boolean incr)
{
    InstanceList    e;

    Transform_Instance(obj, t, incr);

    if ( Edit_Maintain_All_Constraints(obj, elk_window, TRUE) )
    {
        Update_Projection_Extents(topological_list);
        changed_scene = TRUE;
    }
    else if ((e = Find_Object_In_Instances(obj, elk_window->all_instances)))
    {
        View_Update(elk_window, e, CalcView);
        Update_Projection_Extents(e);
        changed_scene = TRUE;
    }
    Edit_Maintain_Free_List();
}


/***********************************************************************
 *
 * Description: Elk_Object3d_position() is the C callback for the scheme
 *      function "object3d-position". This function is used to
 *      move an object to a new world coordinate.
 *
 * Parameter:   xobj - X coordinate to move to
 *      yobj - Y coordinate to move to
 *      zobj - Z coordinate to move to
 *      obj - object to move. This parameter should be of
 *          the same type as that returned by the scheme
 *          function "object3d-create".
 *
 * Scheme example: (object3d-position 1.0 1.0 1.0 (object3d-create "sphere"))
 *
 * Return value: Returns the object passed in thus allowing for succesive
 *       transformations to be performed. For example:
 *       (object3d-position 1.0 1.0 1.0
 *         (object3d-position 1.0 1.0 1.0 (object3d-create "sphere")))
 *
 ***********************************************************************/
Object
Elk_Object3d_Position(Object xobj, Object yobj, Object zobj, Object obj)
{
    Transformation *t;

    Check_Type(xobj, T_Flonum);
    Check_Type(yobj, T_Flonum);
    Check_Type(zobj, T_Flonum);
    Check_Type(obj, T_Object3d);
    t = &ELKOBJECT3(obj)->object3->o_transform;
    t->displacement.x = FLONUM(xobj)->val;
    t->displacement.y = FLONUM(yobj)->val;
    t->displacement.z = FLONUM(zobj)->val;

    Elk_Transform_Instance(ELKOBJECT3(obj)->object3, t, TRUE);

    return obj;
}


/***********************************************************************
 *
 * Description: Elk_Object3d_displace() is the C callback for the scheme
 *      function "object3d-displace". This function is used to
 *      move an object by a certain amount.
 *
 * Parameter:   xobj - X displacement
 *      yobj - Y displacement
 *      zobj - Z displacement
 *      obj - object to move. This parameter should be of
 *          the same type as that returned by the scheme
 *          function "object3d-create".
 *
 * Scheme example: (object3d-displace 1.0 1.0 1.0 (object3d-create "sphere"))
 *
 * Return value: Returns the object passed in thus allowing for succesive
 *       transformations to be performed. For example:
 *       (object3d-displace 1.0 1.0 1.0
 *         (object3d-displace 1.0 1.0 1.0 (object3d-create "sphere")))
 *
 ***********************************************************************/
Object
Elk_Object3d_Displace(Object xobj, Object yobj, Object zobj, Object obj)
{
    Transformation *t;

    Check_Type(xobj, T_Flonum);
    Check_Type(yobj, T_Flonum);
    Check_Type(zobj, T_Flonum);
    Check_Type(obj, T_Object3d);
    t = &ELKOBJECT3(obj)->object3->o_transform;
    t->displacement.x += FLONUM(xobj)->val;
    t->displacement.y += FLONUM(yobj)->val;
    t->displacement.z += FLONUM(zobj)->val;

    Elk_Transform_Instance(ELKOBJECT3(obj)->object3, t, TRUE);

    return obj;
}


/***********************************************************************
 *
 * Description: Elk_Object3d_scale() is the C callback for the scheme
 *      function "object3d-scale". This function is used to
 *      rescale an object.
 *
 * Parameter:   xobj - amount to scale in X.
 *      yobj - amount to scale in Y.
 *      zobj - amount to scale in Z.
 *      obj - object to scale. This parameter should be of
 *          the same type as that returned by the scheme
 *          function "object3d-create".
 *
 * Scheme example: (object3d-scale 2.0 2.0 2.0 (object3d-create "sphere"))
 *
 * Return value: Returns the object passed in thus allowing for succesive
 *       transformations to be performed. For example:
 *       (object3d-scale 2.0 2.0 2.0
 *         (object3d-scale 2.0 2.0 2.0 (object3d-create "sphere")))
 *
 ***********************************************************************/
Object
Elk_Object3d_Scale(Object xobj, Object yobj, Object zobj, Object obj)
{
    Transformation a;

    Check_Type(xobj, T_Flonum);
    Check_Type(yobj, T_Flonum);
    Check_Type(zobj, T_Flonum);
    Check_Type(obj, T_Object3d);

    a.matrix.x.x = FLONUM(xobj)->val;
    a.matrix.x.y = 0.0;
    a.matrix.x.z = 0.0;
    a.matrix.y.x = 0.0;
    a.matrix.y.y = FLONUM(yobj)->val;
    a.matrix.y.z = 0.0;
    a.matrix.z.x = 0.0;
    a.matrix.z.y = 0.0;
    a.matrix.z.z = FLONUM(zobj)->val;
    VNew(0, 0, 0, a.displacement);

    Elk_Transform_Instance(ELKOBJECT3(obj)->object3, &a, FALSE);

    return obj;
}

/***********************************************************************
 *
 * Description: Elk_Object3d_scale() is the C callback for the scheme
 *      function "object3d-scale". This function is used to
 *      rescale an object.
 *
 * Parameter:   xobj - amount to scale in X.
 *      yobj - amount to scale in Y.
 *      zobj - amount to scale in Z.
 *      obj - object to scale. This parameter should be of
 *          the same type as that returned by the scheme
 *          function "object3d-create".
 *
 * Scheme example: (object3d-scale 2.0 2.0 2.0 (object3d-create "sphere"))
 *
 * Return value: Returns the object passed in thus allowing for succesive
 *       transformations to be performed. For example:
 *       (object3d-scale 2.0 2.0 2.0
 *         (object3d-scale 2.0 2.0 2.0 (object3d-create "sphere")))
 **********************************************************************/
Object
Elk_Object3d_Rotate(Object xobj, Object yobj, Object zobj, Object obj)
{
    Transformation  t;
    Vector          vect;

    Check_Type(xobj, T_Flonum);
    Check_Type(yobj, T_Flonum);
    Check_Type(zobj, T_Flonum);
    Check_Type(obj, T_Object3d);

    vect.x = FLONUM(xobj)->val;
    vect.y = FLONUM(yobj)->val;
    vect.z = FLONUM(zobj)->val;
    Vector_To_Rotation_Matrix(&vect, &(t.matrix));

    VNew(0, 0, 0, t.displacement);

    Elk_Transform_Instance(ELKOBJECT3(obj)->object3, &t, FALSE);

    return obj;
}


Object
Elk_Object3d_Set_Control(Object x_obj, Object y_obj, Object z_obj, Object c_obj,
			 Object coord_obj, Object obj_obj)
{
    ObjectInstancePtr	obj;
    InstanceList	e;
    Vector          	new_posn;
    Vector		temp_v;

    Check_Type(x_obj, T_Flonum);
    Check_Type(y_obj, T_Flonum);
    Check_Type(z_obj, T_Flonum);
    Check_Integer(c_obj);
    Check_Type(coord_obj, T_Symbol);
    Check_Type(obj_obj, T_Object3d);

    obj = ELKOBJECT3(obj_obj)->object3;

    if ( ! Obj_Is_Control(obj) )
	return obj_obj;

    if ( Get_Integer(c_obj) >= control_part(obj)->num_control_verts )
	return obj_obj;

    VNew(FLONUM(x_obj)->val, FLONUM(y_obj)->val, FLONUM(z_obj)->val, new_posn);
    if ( EQ(coord_obj, Sym_Global) )
    {
	/* Need to get new location in local coordinates. Apply the inverse
	** transformation.
	*/
	VSub(new_posn, obj->o_transform.displacement, temp_v);
	MVMul(obj->o_inverse, temp_v, new_posn);
    }

    control_part(obj)->control_verts[Get_Integer(c_obj)] = new_posn;

    control_part(obj)->calc_verts(obj, obj->o_world_verts,
				  obj->o_num_real, obj->o_num_vertices);
    Transform_Vertices(obj->o_transform, obj->o_world_verts,
		       obj->o_num_vertices);
    control_part(obj)->calc_face_norms(obj, obj->o_normals);
    if ( Obj_Is_Triangle(obj) )
    {
	Matrix	transp;
	Vector	normal;
	double	temp_d;

	normal = obj->o_normals[0];
	MTrans(obj->o_inverse, transp);
	MVMul(transp, normal, obj->o_normals[0]);
	if ( ! VZero(obj->o_normals[0]) )
                VUnit(obj->o_normals[0], temp_d, obj->o_normals[0]);
    }

    if ( Edit_Maintain_All_Constraints(obj, elk_window, TRUE) )
    {
        Update_Projection_Extents(topological_list);
        changed_scene = TRUE;
    }
    else if ((e = Find_Object_In_Instances(obj, elk_window->all_instances)))
    {
        View_Update(elk_window, e, CalcView);
        Update_Projection_Extents(e);
        changed_scene = TRUE;
    }
    Edit_Maintain_Free_List();

    return obj_obj;
}


/***********************************************************************
 *
 * Description: Elk_Object3d_destroy() is the C callback for the scheme
 *      function "object3d-destroy". This function is used to
 *      destroy an object from scheme.
 *
 * Parameter:   obj - object to be destroyed. This parameter should be of
 *          the same type as that returned by the scheme
 *          function "object3d-create".
 *
 * Scheme example: (object3d-destroy (object3d-create "sphere"))
 *
 * Return value: Returns the Void object.
 *
 ***********************************************************************/
Object
Elk_Object3d_Destroy(Object obj)
{
    ObjectInstancePtr object3;

    Check_Type(obj, T_Object3d);
    object3 = ELKOBJECT3(obj)->object3;
    if (object3) {
        InstanceList victim;
        
        victim = Find_Object_In_Instances(object3,
                          elk_window->all_instances);
        if (elk_window->all_instances == victim)
            elk_window->all_instances = victim->next;
        Delete_Element(victim);
        free(victim);
        /*
         * Check for the victim in the edit lists.
         */
        if ((victim = Find_Object_In_Instances(object3,
                   elk_window->edit_instances)))
            Delete_Edit_Instance(elk_window, victim);
        Destroy_Instance(object3);
        View_Update(elk_window, elk_window->all_instances, ViewNone);
    }
    return Void;
}

/**********************************************************************
 *
 * Description: Elk_Object3d_wireframe_query() is the C callback for the scheme
 *              function "object3d-wireframe-query". This function is used to
 *              obtain the wireframe level for an object.
 *
 * Parameter: obj - The object to query.
 *
 * Scheme example: (object3d-wireframe-query (object3d-create "sphere"))
 *                 0
 *
 * Return value: Returns an INT type object containing the wireframe level.
 **********************************************************************/
Object
Elk_Object3d_Wireframe_Query(Object obj)
{
    ObjectInstancePtr object3;

    Check_Type(obj, T_Object3d);
    object3 = ELKOBJECT3(obj)->object3;
    if ( ! object3)
        return Void;

    return Make_Integer(Wireframe_Density_Level(object3));
}


/**********************************************************************
 *
 * Description: Elk_Object3d_wireframe_level() is the C callback for
 *              the scheme function "object3d-wireframe-level". This
 *              function is used to set the wireframe level for an object.
 *
 * Parameters: obj - the object to set.
 *             level_obj - the new level.
 *
 * Scheme example: (object3d-wireframe-level (object3d-create "sphere") 2)
 *
 * Return value: Returns the object obj.
 **********************************************************************/
Object
Elk_Object3d_Wireframe_Level(Object level_obj, Object obj)
{
    ObjectInstancePtr   object3;
    int                 level;
    InstanceList        e;

    Check_Type(obj, T_Object3d);
    Check_Integer(level_obj);
    object3 = ELKOBJECT3(obj)->object3;
    level = Get_Integer(level_obj);
    if ( ! object3)
        return Void;

    Object_Change_Wire_Level(object3, level);

    if ((e = Find_Object_In_Instances(object3, elk_window->all_instances)))
    {
        View_Update(elk_window, e, CalcView);
        Update_Projection_Extents(e);
        changed_scene = TRUE;
    }

    return obj;
}

Object
Elk_Object3d_Attribs_Define(Object val, Object obj)
{
    Check_Type(obj, T_Object3d);

    ((AttributePtr)ELKOBJECT3(obj)->object3->o_attribs)->defined =
        Truep(val);

    changed_scene = TRUE;

    return obj;
}

Object
Elk_Object3d_Color(Object robj, Object gobj, Object bobj, Object obj)
{
    Check_Type(robj, T_Flonum);
    Check_Type(gobj, T_Flonum);
    Check_Type(bobj, T_Flonum);
    Check_Type(obj, T_Object3d);

    ((AttributePtr)ELKOBJECT3(obj)->object3->o_attribs)->color.red =
        FLONUM(robj)->val;
    ((AttributePtr)ELKOBJECT3(obj)->object3->o_attribs)->color.green =
        FLONUM(gobj)->val;
    ((AttributePtr)ELKOBJECT3(obj)->object3->o_attribs)->color.blue =
        FLONUM(bobj)->val;

    changed_scene = TRUE;

    return obj;
}

Object
Elk_Object3d_Diffuse(Object difobj, Object obj)
{
    Check_Type(difobj, T_Flonum);
    Check_Type(obj, T_Object3d);

    ((AttributePtr)ELKOBJECT3(obj)->object3->o_attribs)->diff_coef =
        FLONUM(difobj)->val;

    changed_scene = TRUE;

    return obj;
}

Object
Elk_Object3d_Specular(Object specobj, Object powobj, Object obj)
{
    Check_Type(specobj, T_Flonum);
    Check_Type(powobj, T_Flonum);
    Check_Type(obj, T_Object3d);

    ((AttributePtr)ELKOBJECT3(obj)->object3->o_attribs)->spec_coef =
        FLONUM(specobj)->val;
    ((AttributePtr)ELKOBJECT3(obj)->object3->o_attribs)->spec_power =
        FLONUM(powobj)->val;

    changed_scene = TRUE;

    return obj;
}

Object
Elk_Object3d_Reflect(Object reflobj, Object obj)
{
    Check_Type(reflobj, T_Flonum);
    Check_Type(obj, T_Object3d);

    ((AttributePtr)ELKOBJECT3(obj)->object3->o_attribs)->reflect_coef =
        FLONUM(reflobj)->val;

    changed_scene = TRUE;

    return obj;
}

Object
Elk_Object3d_Transparency(Object transobj, Object refrobj, Object obj)
{
    Check_Type(transobj, T_Flonum);
    Check_Type(refrobj, T_Flonum);
    Check_Type(obj, T_Object3d);

    ((AttributePtr)ELKOBJECT3(obj)->object3->o_attribs)->transparency =
        FLONUM(transobj)->val;
    ((AttributePtr)ELKOBJECT3(obj)->object3->o_attribs)->refract_index =
        FLONUM(refrobj)->val;

    changed_scene = TRUE;

    return obj;
}

