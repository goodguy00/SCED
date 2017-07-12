/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/new_object.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: new_object.c,v $
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
**  new_object.c : Functions used in created a new object, including those
**                  related to the newObjectDialog widget.
**
**  Created: 20/03/94
*/

#include <sced.h>
#include <base_objects.h>
#include <csg.h>
#include <aggregate.h>
#include <instance_list.h>
#include <X11/Shell.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <SimpleWire.h>


/* A widget for each object that is available to choose from.  */
static Widget   cube_object_widget;
static Widget   cyl_object_widget;
static Widget   sphere_object_widget;
static Widget   cone_object_widget;
static Widget   torus_object_widget;
static Widget   plane_object_widget;
static Widget   triangle_object_widget;
static Widget   bezier_object_widget;

static Widget   available_objects_widget;   /* The form widget. */
static Widget   new_label_widget;           /* The label widget at the top. */
static Widget   cancel_widget;              /* The cancel button. */
static Widget   new_object_shell = NULL;
Widget  new_csg_button = NULL;              /* The CSG object button. */
Widget  new_wire_button = NULL;             /* The Wireframe object button. */
Widget  new_agg_button = NULL;              /* The Aggregate object button. */

static WindowInfoPtr    current_window;

static int  select_op;


/* Object count, for generating default names. */
int     object_count[last_obj] = { 0 };


/*
**  Function prototypes.
*/
static void Create_New_Object_Dialog();
static void Create_Available_Objects();
static Widget Add_SimpleWire_To_Form(String, BaseObjectPtr, Widget, Widget,
                                     Widget, int, int);
static void Create_CSG_Callback(Widget, XtPointer, XtPointer);
static void Create_Wire_Callback(Widget, XtPointer, XtPointer);
static void Create_Aggregate_Callback(Widget, XtPointer, XtPointer);

void Create_New_Object(Widget, XtPointer, XtPointer);
void Cancel_New_Object(Widget, XtPointer, XtPointer);



/*  void
**  New_Object_Popup(Widget w, XtPointer client_data, XtPointer call_data)
**  Callback procedure to pop up the new object dialog box.
**  Should only be called by the New Object command button.
**  First checks to see if the box has been created.  If not, creates it.
**  Then, just pops it up.
*/
void
New_Object_Popup_Callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    if (new_object_shell == NULL)
        Create_New_Object_Dialog();

    current_window = (WindowInfoPtr)client_data;

    select_op = select_new;

    if ( current_window == &csg_window )
    {
        XtVaSetValues(plane_object_widget, XtNsensitive, FALSE, NULL);
        XtVaSetValues(triangle_object_widget, XtNsensitive, FALSE, NULL);
        XtVaSetValues(bezier_object_widget, XtNsensitive, FALSE, NULL);
    }
    else
    {
        XtVaSetValues(plane_object_widget, XtNsensitive, TRUE, NULL);
        XtVaSetValues(triangle_object_widget, XtNsensitive, TRUE, NULL);
        XtVaSetValues(bezier_object_widget, XtNsensitive, TRUE, NULL);
    }

    Position_Shell(new_object_shell, FALSE);
    XtPopup(new_object_shell, XtGrabExclusive);
}

void
Select_Object_Popup(WindowInfoPtr window, int op)
{
    if (new_object_shell == NULL)
        Create_New_Object_Dialog();

    current_window = window;

    select_op = op;
    
    Position_Shell(new_object_shell, FALSE);
    XtPopup(new_object_shell, XtGrabExclusive);
}


/*  void
**  Create_New_Object_Dialog()
**  Creates the new object dialog box widget.
*/
static void
Create_New_Object_Dialog()
{
    Arg             args[10];
    int             n;

    n = 0;
    new_object_shell = XtCreatePopupShell("newObject",
                    transientShellWidgetClass, main_window.shell, args, n);


    /* Create the available objects widget. */
    Create_Available_Objects();

    /* Set sensitivity. */
    if ( ! num_csg_base_objects )
        Set_CSG_Related_Sensitivity(FALSE);
    if ( ! num_wire_base_objects )
        Set_Wireframe_Related_Sensitivity(FALSE);
    if ( ! num_agg_base_objects )
        Set_Agg_Related_Sensitivity(FALSE);

    XtRealizeWidget(new_object_shell);

}

/*  void
**  Create_Available_Objects()
**  Creates the form widget  that contains the list  of available objects.
*/
static void
Create_Available_Objects()
{
    String      shell_geometry;
    unsigned    shell_width, shell_height;
    int         junk;
    int         obj_width, obj_height;
    int         gap;
    Arg         args[15];
    int         n, m;

    /* Get the parents size. */
    n = 0;
    XtSetArg(args[n], XtNgeometry, &shell_geometry);    n++;
    XtGetValues(new_object_shell, args, n);
    XParseGeometry(shell_geometry, &junk, &junk, &shell_width, &shell_height);

    n = 0;
    available_objects_widget = XtCreateManagedWidget("newObjectsBox",
                                formWidgetClass, new_object_shell, args, n);

    /* Get the form's gap. */
    n = 0;
    XtSetArg(args[n], XtNdefaultDistance, &gap);    n++;
    XtGetValues(available_objects_widget, args, n);

    obj_width = ((int)shell_width - 5 * gap - 8 ) / 4;
    obj_height = ((int)shell_height - 5 * gap - 8 ) / 3;

    /* Create the label for the top of the box. */
    n = 0;
    XtSetArg(args[n], XtNlabel, "Select A New Object"); n++;
    XtSetArg(args[n], XtNleft, XtChainLeft);            n++;
    XtSetArg(args[n], XtNright, XtChainLeft);           n++;
    XtSetArg(args[n], XtNtop, XtChainTop);              n++;
    XtSetArg(args[n], XtNbottom, XtChainTop);           n++;
    XtSetArg(args[n], XtNborderWidth, 0);               n++;
    new_label_widget = XtCreateManagedWidget("newObjectLabel", labelWidgetClass,
                                available_objects_widget, args, n);

    /* Put the simple wireframes in the form. */
    cube_object_widget = Add_SimpleWire_To_Form("cubeNewObject",
            base_objects[cube_obj],
            available_objects_widget, NULL, new_label_widget,
            obj_width, obj_height);
    sphere_object_widget = Add_SimpleWire_To_Form("sphereNewObject",
            base_objects[sphere_obj],
            available_objects_widget, cube_object_widget, new_label_widget,
            obj_width, obj_height);
    cyl_object_widget = Add_SimpleWire_To_Form("cylinderNewObject",
            base_objects[cylinder_obj],
            available_objects_widget, sphere_object_widget, new_label_widget,
            obj_width, obj_height);
    cone_object_widget = Add_SimpleWire_To_Form("coneNewObject",
            base_objects[cone_obj],
            available_objects_widget, cyl_object_widget, new_label_widget,
            obj_width, obj_height);
    torus_object_widget = Add_SimpleWire_To_Form("torusNewObject",
            base_objects[torus_obj],
            available_objects_widget, NULL, cube_object_widget,
            obj_width, obj_height);
    plane_object_widget = Add_SimpleWire_To_Form("planeNewObject",
            base_objects[plane_obj],
            available_objects_widget, torus_object_widget,
            sphere_object_widget, obj_width, obj_height);
    triangle_object_widget = Add_SimpleWire_To_Form("triangleNewObject",
            base_objects[triangle_obj],
            available_objects_widget, plane_object_widget, cyl_object_widget,
            obj_width, obj_height);
    bezier_object_widget = Add_SimpleWire_To_Form("bezierNewObject",
            base_objects[bezier_obj],
            available_objects_widget, triangle_object_widget,
            cone_object_widget, obj_width, obj_height);

    m = 0;
    XtSetArg(args[m], XtNfromVert, torus_object_widget);    m++;
    XtSetArg(args[m], XtNleft, XtChainLeft);                m++;
    XtSetArg(args[m], XtNright, XtChainLeft);               m++;
    XtSetArg(args[m], XtNtop, XtChainBottom);               m++;
    XtSetArg(args[m], XtNbottom, XtChainBottom);            m++;

    /* Put the CSG button bottom right. */
    n = m;
    XtSetArg(args[n], XtNlabel, "CSG Object");              n++;
    XtSetArg(args[n], XtNfromHoriz, NULL);                  n++;
    new_csg_button = XtCreateManagedWidget("newObjectCSG", commandWidgetClass,
                                available_objects_widget, args, n);
    XtAddCallback(new_csg_button, XtNcallback, Create_CSG_Callback, NULL);

    /* Put the Wireframe button beside the CSG button. */
    n = m;
    XtSetArg(args[n], XtNlabel, "Wireframe Object");        n++;
    XtSetArg(args[n], XtNfromHoriz, new_csg_button);        n++;
    new_wire_button = XtCreateManagedWidget("newObjectWire", commandWidgetClass,
                                available_objects_widget, args, n);
    XtAddCallback(new_wire_button, XtNcallback, Create_Wire_Callback, NULL);

    /* Put the Aggregate button beside the Wireframe button. */
    n = m;
    XtSetArg(args[n], XtNlabel, "Aggregate Object");        n++;
    XtSetArg(args[n], XtNfromHoriz, new_wire_button);       n++;
    new_agg_button = XtCreateManagedWidget("newObjectAgg", commandWidgetClass,
                                available_objects_widget, args, n);
    XtAddCallback(new_agg_button, XtNcallback, Create_Aggregate_Callback, NULL);

    /* Put the cancel button down the bottom. */
    n = m;
    XtSetArg(args[n], XtNlabel, "Cancel");                  n++;
    XtSetArg(args[n], XtNfromHoriz, new_agg_button);        n++;
    cancel_widget = XtCreateManagedWidget("newObjectCancel", commandWidgetClass,
                                available_objects_widget, args, n);
    XtAddCallback(cancel_widget, XtNcallback, Cancel_New_Object, NULL);


    /* Add callbacks for the widgets. */
    XtAddCallback(cube_object_widget, XtNcallback, Create_New_Object, NULL);
    XtAddCallback(cyl_object_widget, XtNcallback, Create_New_Object, NULL);
    XtAddCallback(cone_object_widget, XtNcallback, Create_New_Object, NULL);
    XtAddCallback(sphere_object_widget, XtNcallback, Create_New_Object, NULL);
    XtAddCallback(torus_object_widget, XtNcallback, Create_New_Object, NULL);
    XtAddCallback(plane_object_widget, XtNcallback, Create_New_Object, NULL);
    XtAddCallback(triangle_object_widget, XtNcallback, Create_New_Object, NULL);
    XtAddCallback(bezier_object_widget, XtNcallback, Create_New_Object, NULL);

}


static Widget
Add_SimpleWire_To_Form(String name, BaseObjectPtr base, Widget form,
                        Widget to_left, Widget to_top, int width, int height)
{
    Arg             args[10];
    int             n;

    n = 0;
    XtSetArg(args[n], XtNbasePtr, base);        n++;
    XtSetArg(args[n], XtNwidth, width);         n++;
    XtSetArg(args[n], XtNheight, height);       n++;
    XtSetArg(args[n], XtNfromHoriz, to_left);   n++;
    XtSetArg(args[n], XtNfromVert, to_top);     n++;

    return XtCreateManagedWidget(name, simpleWireWidgetClass, form, args, n);

}


static char *bnames[] = { "Cube_%d", "Sphere_%d", "Cylinder_%d", "Cone_%d",
                          "Triangle_%d", "Plane_%d", "Light_%d", "SpotLight_%d",
                          "AreaLight_%d", "Torus_%d", "DirLight_%d",
                          "Bezier_%d" };

ObjectInstancePtr
Create_New_Object_From_Base(WindowInfoPtr window, BaseObjectPtr base_obj,
                            Boolean attach)
{
    ObjectInstancePtr   new_obj;
    InstanceList        new_list;
    char                new_name[128];

    if ( base_obj->b_class == csg_obj ||
         base_obj->b_class == wireframe_obj ||
         base_obj->b_class == aggregate_obj )
        sprintf(new_name, "%s_%d", base_obj->b_label,
                object_count[base_obj->b_class]);
    else
        sprintf(new_name, bnames[base_obj->b_class],
                object_count[base_obj->b_class]);

    new_obj = Create_Instance(base_obj, new_name);

    if ( window == &csg_window )
    {
        if ( attach )
            CSG_Attach_Instance(new_obj);
        else
            New_CSG_Instance(new_obj);
        return new_obj;
    }

    /* If main window, set attributes to be defined. */
    if ( window == &main_window )
        new_obj->o_attribs->defined = TRUE;

    new_list = New(InstanceListElmt, 1);
    new_list->the_instance = new_obj;
    new_list->next = new_list->prev = NULL;
    Add_Instance_To_Edit(window, new_list, FALSE);
    free(new_list);

    return new_obj;
}


void
Create_New_Object(Widget w, XtPointer client_data, XtPointer call_data)
{
    BaseObjectPtr   base_obj;
    int             count = (int)call_data;
    int             i;

    XtPopdown(new_object_shell);

    XtVaGetValues(w, XtNbasePtr, &base_obj, NULL);

    if ( select_op == select_change )
        Base_Change_Select_Callback(NULL, base_obj);
    else
        for ( i = 0 ; i < count ; i++ )
            Create_New_Object_From_Base(current_window, base_obj, FALSE);
}


/*
**  static void
**  Create_CSG_Callback(Widget, XtPointer, XtPointer);
**  Does little more than pop it down and call the CSG routines.
*/
static void
Create_CSG_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    XtPopdown(new_object_shell);

    Base_Select_Popup(csg_obj,
                select_op == select_new ? csg_select_new : csg_select_change);
}

static void
Create_Wire_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    XtPopdown(new_object_shell);

    Base_Select_Popup(wireframe_obj, WIRE_CREATE);
}

static void
Create_Aggregate_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    XtPopdown(new_object_shell);

    Base_Select_Popup(aggregate_obj, AGG_CREATE);
}

void
Cancel_New_Object(Widget w, XtPointer client_data, XtPointer call_data)
{
    XtPopdown(new_object_shell);
}


