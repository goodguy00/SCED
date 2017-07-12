/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/h/RCS/edit.h,v 1.0 1997/05/06 20:41:27 schenney Exp $
 *
 * $Log: edit.h,v $
 * Revision 1.0  1997/05/06 20:41:27  schenney
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
**  edit.h : header for common editing functions.
*/

#ifndef __SCED_EDIT__
#define __SCED_EDIT__


#define ARC_DIVISIONS 30

#define NO_DRAG 0
#define ORIGIN_DRAG 1
#define SCALE_DRAG 2
#define ROTATE_DRAG 3
#define RADIUS_DRAG 4
#define CONTROL_DRAG 5

#define MAJOR_AXIS 1
#define MINOR_AXIS 2

#define MAX_ADD_ITEMS 7 /* The max number of items in any add constraint menu.*/

#define MAX_NUM_SPECS 6 /* The max num of specifers ever required. */

#include <edit_types.h>

typedef enum _EditOpType {
    edit_drag_op,       /* A drag of any type. */
    edit_select_op,     /* A constraint selection op. */
    edit_deselect_op,   /* A constraint deselection op. */
    edit_add_op,        /* An add constraint op. */
    edit_remove_op,     /* A remove constraint op. */
    edit_modify_op,     /* A modify constrint op. */
    edit_scaling_op,    /* A scaling change op. */
    edit_origin_op,     /* An origin change op. */
    edit_axis_op,       /* An axis change op. */
    edit_radius_op      /* A radius pt change op. */
    } EditOpType, *EditOpTypePtr;

typedef void    (*Update_Spec_Func)(ConstraintSpecPtr, VectorPtr,
                                    FeaturePtr, VectorPtr, VectorPtr);
extern Update_Spec_Func update_spec_function[];


/*
**  edit.c
*/
extern void     Edit_Clear_Info(EditInfoPtr);
extern Boolean  Edit_Begin();
extern EditInfoPtr  Edit_Get_Info();
extern void     Edit_Draw_Selection_Points(EditInfoPtr);
extern void     Edit_Cleanup_Selection(EditInfoPtr, Boolean);
extern void     Edit_Transform_Vertices(Transformation*, EditInfoPtr);
extern void     Edit_Transform_Normals(Transformation*, EditInfoPtr);
extern void     Edit_Calculate_Cons_Points(EditInfoPtr, int);
extern void     Edit_Calculate_Axis_Cons_Points(EditInfoPtr, int);
extern Vector   Map_Point_Onto_Plane(XPoint*, ConstraintPtr, Viewport*, short,
                                     short, int);
extern Vector   Map_Point_Onto_Line(XPoint*, ConstraintPtr, Viewport*, short,
                                    short, int);
extern Vector   Map_Point_Onto_Circle(XPoint*, ConstraintPtr, ConstraintPtr,
                                      Viewport*, short, short, int);
extern Vector   Map_Point_Onto_Sphere(XPoint*, ConstraintPtr, XPoint *,
                                      Viewport*, int);
extern Boolean  Get_Closest_Screen_Pt(ResultantPtr, XPoint*, XPoint*, XPoint*,
                                      Vector*, int*);
extern void     Edit_Clear_Cursor(WindowInfoPtr);


/*
**  edit_shell.c
*/
extern void Edit_Complete();
extern void Edit_Initialize_Shell(EditInfoPtr);
extern void Edit_Match_Widths();
extern void Edit_Cancel_Modify(EditInfoPtr);
extern void Edit_Set_Drag_Label(int, Vector, double);


/*
**  constraint_list.c
*/
extern void Edit_Add_Constraint_Callback(Widget, XtPointer, XtPointer);
extern void Edit_Remove_Constraint(EditInfoPtr, int, int, Boolean);
extern void Edit_Modify_Constraint(EditInfoPtr, int, ConstraintSpecPtr,
                                   VectorPtr, int, char*, double, double,
                                   Boolean);

extern void Edit_Select_Constraint_Callback(Widget, XtPointer, XtPointer);
extern void Edit_Select_Constraint(EditInfoPtr, int, int,
                                   Boolean, Boolean, Boolean);
extern void Add_Spec(ConstraintSpec*, Vector, Vector, ConstraintSpecType,
                     ObjectInstancePtr, int);
extern void Add_Object_Constraint(ConstraintPtr, int, int, Boolean);

/*
**  add_constraint.c
*/
extern void Add_Modify_Constraint(EditInfoPtr, ConstraintPtr);

/*
**  dependency.c
*/
extern void Remove_Constraint(ObjectInstancePtr, ConstraintPtr*, int*, int);
extern void Add_Dependency(ObjectInstancePtr, ObjectInstancePtr);


/*
**  draw.c
*/
extern void Edit_Draw(WindowInfoPtr, int, EditInfoPtr, Boolean);
extern void Draw_Edit_Extras(WindowInfoPtr, int, EditInfoPtr, Boolean);
extern void Draw_All_Constraints(WindowInfoPtr, int, EditInfoPtr, Boolean);
extern void Draw_Point_Constraints(int, FeaturePtr, WindowInfoPtr, int,
                                    EditInfoPtr, Boolean);
extern void Draw_Axis_Constraints(int, FeaturePtr, WindowInfoPtr, int,
                                    EditInfoPtr, Boolean);


/*
**  events.c
*/
extern void Initiate_Object_Edit(WindowInfoPtr);
extern void Cancel_Object_Edit();


/*
**  placement.c
*/
extern void Edit_Extract_Resultant_Data(EditInfoPtr, int, ConstraintData*,
                                        XPoint*, XPoint*);
extern Vector   Edit_Obtain_New_Point(EditInfoPtr, int, XPoint, ConstraintData*,
                                      XPoint*, XPoint*);
extern void Edit_Start_Origin_Drag(XEvent*, int, EditInfoPtr);
extern void Edit_Continue_Origin_Drag(XEvent*, int, EditInfoPtr);
extern void Edit_Finish_Origin_Drag(XEvent*, int, EditInfoPtr);
extern Boolean  Edit_Force_Origin_Satisfaction(EditInfoPtr, int, Boolean);


/*
**  scale.c
*/
extern void Edit_Start_Scale_Drag(XEvent*, int, EditInfoPtr);
extern void Edit_Continue_Scale_Drag(XEvent*, int, EditInfoPtr);
extern void Edit_Finish_Scale_Drag(XEvent*, int, EditInfoPtr);
extern Boolean  Edit_Force_Scale_Satisfaction(EditInfoPtr, int, Boolean);
extern Boolean  Edit_Dynamic_Scale(EditInfoPtr, Vector);
extern void Scale_Calculate_Transform(Transformation*, Vector, Vector,
                                Vector, Vector, Matrix*, Matrix*, Boolean);
extern Boolean  Edit_Scale_Force_Constraints(EditInfoPtr, Boolean);


/*
**  radius.c
*/
extern void     Edit_Start_Radius_Drag(XEvent*, int, EditInfoPtr);
extern void     Edit_Continue_Radius_Drag(XEvent*, int, EditInfoPtr);
extern void     Edit_Finish_Radius_Drag(XEvent*, int, EditInfoPtr);
extern Boolean  Edit_Force_Radius_Satisfaction(EditInfoPtr, int, Boolean);
extern Boolean  Edit_Dynamic_Set_Radius(EditInfoPtr, Vector);
extern void     Radius_Modify_Radius(Vector, double, Vector, Vector,
                                     Transformation*, Matrix*, double*,
                                     VectorPtr, Matrix*, Boolean);

/*
**  rotate.c
*/
extern void Edit_Start_Rotate_Drag(XEvent*, int, EditInfoPtr);
extern void Edit_Continue_Rotate_Drag(XEvent*, int, EditInfoPtr);
extern void Edit_Finish_Rotate_Drag(XEvent*, int, EditInfoPtr);
extern void Edit_Align(EditInfoPtr, Matrix*);

/*
**  control_pt.c
*/
extern void Edit_Start_Control_Drag(XEvent*, int, EditInfoPtr);
extern void Edit_Continue_Control_Drag(XEvent*, int, EditInfoPtr);
extern void Edit_Finish_Control_Drag(XEvent*, int, EditInfoPtr);
extern Boolean  Edit_Dynamic_Control(EditInfoPtr, Vector, int);
extern Boolean  Edit_Dynamic_Control_Obj(EditInfoPtr, Vector, int);
extern Boolean  Edit_Force_Control_Satisfaction(EditInfoPtr, int, Boolean);
extern void     Control_Dynamic_Vertex(ObjectInstancePtr, Vector, int,Vector);

/*
**  maintain.c
*/
extern void     Edit_Maintain_Dynamic_Constraints(EditInfoPtr, int, VectorPtr,
                                                  Transformation*);
extern Boolean  Edit_Maintain_All_Constraints(ObjectInstancePtr, WindowInfoPtr,
                                              Boolean);
extern void     Edit_Maintain_Free_List();


/*
**  edit_extras.c
*/
extern void Edit_Reference_Callback(Widget, XtPointer, XtPointer);
extern void Edit_Origin_Callback(Widget, XtPointer, XtPointer);
extern void Edit_Change_Axis_1_Callback(Widget, XtPointer, XtPointer);
extern void Edit_Change_Axis_2_Callback(Widget, XtPointer, XtPointer);
extern void Edit_Radius_Callback(Widget, XtPointer, XtPointer);


/*
**  edit_undo.c
*/
extern void Edit_Undo(EditInfoPtr);
extern void Edit_Redo(Widget, XtPointer, XtPointer);
extern void Edit_Undo_Register_State(EditOpType, FeatureType, int);
extern void Edit_Undo_Clear(EditInfoPtr);


/*
**  align.c
*/
extern void Edit_Solve_Axis_System(ObjectInstancePtr, ConstraintPtr, int,
                                   ResultantPtr);
extern Boolean  Edit_Force_Alignment_Satisfaction(EditInfoPtr, int, Boolean);
extern Boolean  Edit_Dynamic_Align(EditInfoPtr, Transformation*, Vector);
extern Matrix   Major_Align_Matrix(Vector, Matrix*);
extern Matrix   Minor_Align_Matrix(Vector, Matrix*, Boolean);


/*
**  dfs.c
*/
extern Boolean  DFS(ObjectInstancePtr, ObjectInstancePtr *, int,
                    Boolean, InstanceList*, Boolean, Boolean);



/* Extern vars. */
extern Widget   edit_form;
extern Widget   feature_box[];
extern Widget   edit_command_menu;

extern Boolean  do_maintenance;
extern InstanceList topological_list;


#endif /* __SCED_EDIT__ */
