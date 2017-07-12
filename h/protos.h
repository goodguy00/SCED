/*
 * $Header$
 *
 * $Log$
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
**  protos.h: Function prototypes.
**
**  Created: 05/03/94
**
*/

#ifndef __SCED_PROTO__
#define __SCED_PROTO__


/*
**  Functions from viewport.c
*/
extern Boolean  Build_Viewport_Transformation(ViewportPtr);
extern void     Viewport_Init(ViewportPtr);
extern void     Create_View_Menu(Widget, WindowInfoPtr);
extern void     Create_Window_Menu(Widget, WindowInfoPtr);
extern void     Window_Set_Draw_Mode(WindowInfoPtr, int);
extern void     Window_Draw_Mode_Callback(Widget, XtPointer, XtPointer);

/*
**  Functions from conversions.c.
*/
extern void Convert_World_To_View(Vector*, Vertex*, int, Viewport*);
extern void Convert_View_To_Screen(Vertex*, int, Viewport*, short,short,double);
extern void Convert_Plane_World_To_View(ConstraintPtr, Viewport*,ConstraintPtr);
extern void Convert_Line_World_To_View(ConstraintPtr, Viewport*, ConstraintPtr);


/*
**  Functions for wireframe manipulation.
*/
extern WireframePtr         Wireframe_Copy(WireframePtr);
extern WireframePtr         Object_To_Wireframe(ObjectInstancePtr, Boolean);
extern void                 Wireframe_Destroy(WireframePtr);
extern struct _CSGWireframe *Wireframe_To_CSG(WireframePtr, Boolean);
extern WireframePtr         CSG_To_Wireframe(struct _CSGWireframe*);
extern WireframePtr         Wireframe_Simplify(WireframePtr, Boolean);
extern WireframePtr         Face_Triangulate(WireframePtr, FacePtr);
extern Boolean              Wireframe_Has_Attributes(WireframePtr);
extern int                  Wireframe_Count_Edges(WireframePtr);
extern WireframePtr         Wireframe_Triangulate(WireframePtr);
extern void                 Wireframe_Append(WireframePtr, WireframePtr,
                                             ObjectInstancePtr, HashTable);
extern void                 Wireframe_Assign_Vertex_Normals(WireframePtr,float);



/*
**  Functions from utils.c.
*/
extern void         Remove_Temporaries();
extern void         Sced_Exit(int);
extern void         Sced_Abort(Boolean);
extern char         *EMalloc(unsigned int);
extern char         *WMalloc(unsigned int);
extern char         *ERealloc(char*, unsigned int);
extern char         *WRealloc(char*, unsigned int);
extern void         Popup_Error(char*, Widget, char*);
extern Dimension    Match_Widths(Widget*, int);
extern void         Destroy_World(Boolean);
extern Boolean      Check_Rectangle_Intersection(XPoint, XPoint, XPoint,XPoint);
extern void         Set_Prompt(WindowInfoPtr, String);
extern double       Round_To_Snap(double, double);
extern Boolean      Points_Colinear_f(Vector, Vector, Vector);
extern Boolean      Points_Colinear(Vector, Vector, Vector);
extern void         Set_WindowInfo(WindowInfo*);
extern Vector       Extract_Euler_Angles(Matrix);
extern void         Save_Temp_Filename(char*);
extern void         Vector_To_Rotation_Matrix(Vector*, Matrix*);
extern void         Position_Shell(Widget, Boolean);
extern void         Window_Axes_Initialize(WindowInfoPtr);
#if ! HAVE_STRERROR
extern char         *StrError(int);
#endif

/*
**  copy.c
*/
extern void Copy_Objects_Callback(Widget, XtPointer, XtPointer);
extern void Copy_Action_Func(Widget, XEvent*, String*, Cardinal*);
extern void Copy_Object_System(WindowInfoPtr, InstanceList, Boolean);

/*
**  Functions from base_objects.c.
*/
extern Boolean          Initialize_Base_Objects();
extern BaseObjectPtr    Get_Base_Object_From_Label(String);
extern Boolean          Add_Instance_To_Base(ObjectInstancePtr, BaseObjectPtr);
extern void             Remove_Instance_From_Base(ObjectInstancePtr);
extern void             Destroy_All_Base_Objects();
extern BaseObjectPtr    Add_CSG_Base_Object(CSGNodePtr, char*, int,
                                    WireframePtr*, WireframePtr*, int, Boolean);
extern void             CSG_Destroy_Base_Object(Widget, BaseObjectPtr, Boolean);
extern void             CSG_Copy_Base_Object(Widget, BaseObjectPtr);
extern BaseObjectPtr    Add_Wireframe_Base_Object(char*, WireframePtr,
                                                  WireframePtr, int);
extern void             Wireframe_Destroy_Base_Object(Widget, BaseObjectPtr);
extern BaseObjectPtr    Add_Agg_Base_Object(InstanceList, char*, WireframePtr,
                                            WireframePtr, int);
extern void             Agg_Destroy_Base_Object(Widget, BaseObjectPtr);
extern void             Base_Change(ObjectInstancePtr, BaseObjectPtr, Boolean,
                                    WindowInfoPtr);
extern void             Base_Change_List(WindowInfoPtr, InstanceList,
                                         BaseObjectPtr);
extern void             Base_Change_Callback(Widget, XtPointer, XtPointer);
extern void             Base_Change_Select_Callback(Widget, BaseObjectPtr);
/* Located in csg_reference.c. */
extern int              Select_Base_Reference(WireframePtr*, CSGNodePtr,
                                              Boolean*);


/*
**  Functions from instances.c.
*/
extern ObjectInstancePtr    Create_Instance(BaseObjectPtr, String);
extern void     Create_Generic_Object(ObjectInstancePtr);
extern ObjectInstancePtr    Copy_Object_Instance(ObjectInstancePtr);
extern void     Rename_Instance(ObjectInstancePtr, char*);
extern int      Transform_Instance(ObjectInstancePtr, Transformation*, Boolean);
extern void     Displace_Instance(ObjectInstancePtr, Vector);
extern void     Modify_Instance_Attributes(ObjectInstancePtr, Attributes*, int);
extern void     Destroy_Generic_Object(ObjectInstancePtr);
extern void     Destroy_Instance(ObjectInstancePtr);
extern void     Instance_Destroy_Features(ObjectInstancePtr);
extern void     Copy_Constraint_Set(int, int*, ConstraintPtr, ConstraintPtr*,
                                    ObjectInstancePtr, ObjectInstancePtr);
extern InstanceList Instances_Build_Visible_List(InstanceList);


/*
**  Functions from features.h
*/
extern void             Feature_Create_Bases();
extern FeatureBasePtr   Feature_Base(int);
extern int              Feature_Base_Offset(FeatureBasePtr);
extern void Feature_Create_Origin_Constraints(FeaturePtr);
extern void Feature_Create_Major_Constraints(FeaturePtr, int, int, int);
extern void Feature_Create_Minor_Constraints(FeaturePtr, int, int, int);
extern void Feature_Create_Scale_Constraints(FeaturePtr, Vector*);
extern void Feature_Create_Cone_Scale_Constraints(FeaturePtr,
                                                  ObjectInstancePtr, Vector*);
extern void Feature_Create_Cyl_Scale_Constraints(FeaturePtr,
                                                 ObjectInstancePtr, Vector*);
extern void Feature_Create_Uniform_Scale_Constraints(FeaturePtr, Vector*);
extern void Feature_Create_Camera_Scale_Constraints(FeaturePtr,
                                                ObjectInstancePtr, Vector*);
extern void Feature_Create_Radius_Constraints(FeaturePtr, ObjectInstancePtr);
extern void Feature_Create_Control_Constraints(FeaturePtr, ObjectInstancePtr,
                                               int);
extern void Feature_Create_Dummy_Constraints(FeaturePtr);
extern void Feature_Calculate_Point_Display(int, FeatureBasePtr, EditInfoPtr);
extern void Feature_Calculate_Major_Display(int, FeatureBasePtr, EditInfoPtr);
extern void Feature_Calculate_Minor_Display(int, FeatureBasePtr, EditInfoPtr);


/*
**  Functions from bounds.c
*/
extern Cuboid   Calculate_Bounds(Vector*, int);
extern Cuboid   Calculate_Bounds_f(Vector*, int);
extern Extent2D Calculate_Projection_Extents(Vertex*, int);
extern void     Update_Projection_Extents(InstanceList);
extern Cuboid   Transform_Bound(Cuboid*, Transformation*);


/*
**  Functions from draw.c
*/
extern void View_Update(WindowInfoPtr, InstanceList, int);
extern void Draw_Initialize();
extern void Draw_Edges(Display*, Drawable, GC, GC, WireframePtr,
                       Vertex*, Vector*, Vector*, Viewport*);
extern void Draw_Visible_Edges(Display*, Drawable, GC, WireframePtr,
                        Vertex*, Vector*, Vector*, Viewport*);
extern void Draw_All_Edges(Display*, Drawable, GC, WireframePtr,
                           Vertex*, ViewportPtr);


/*
**  Functions from new_object.c
*/
extern void New_Object_Popup_Callback(Widget, XtPointer, XtPointer);
extern void Select_Object_Popup(WindowInfoPtr, int);
extern void Add_Object_To_World(ObjectInstancePtr, Boolean);
extern ObjectInstancePtr    Create_New_Object_From_Base(WindowInfoPtr,
                                                        BaseObjectPtr, Boolean);


/*
**  From dense_wireframe.c, wireframe changing functions.
*/
extern int  Wireframe_Density_Level(ObjectInstancePtr);
extern void Wireframe_Denser_Callback(Widget, XtPointer, XtPointer);
extern void Wireframe_Thinner_Callback(Widget, XtPointer, XtPointer);
extern WireframePtr Dense_Wireframe(BaseObjectPtr, int, Boolean);
extern void Object_Change_Wire_Level(ObjectInstancePtr, int level);
extern void Object_Change_Wire(ObjectInstancePtr);


/*
**  Functions for managing lights.
*/
extern void     Create_Light_Callback(Widget, XtPointer, XtPointer);
extern void     Create_Spotlight_Callback(Widget, XtPointer, XtPointer);
extern void     Create_Arealight_Callback(Widget, XtPointer, XtPointer);
extern void     Create_Dirlight_Callback(Widget, XtPointer, XtPointer);
extern void     Ambient_Light_Callback(Widget, XtPointer, XtPointer);
extern Boolean  Light_In_Instances(InstanceList);
extern void     Ambient_Action_Func(Widget, XEvent*, String*, Cardinal*);
extern void     Light_Action_Func(Widget, XEvent*, String*, Cardinal*);
extern void     Set_Light_Attributes(InstanceList, Boolean, Boolean);
extern void     Set_Spotlight_Attributes(InstanceList, Boolean);
extern void     Set_Arealight_Attributes(InstanceList);
extern void     Pointlight_Set_Defaults_Callback(Widget, XtPointer, XtPointer);
extern void     Spotlight_Set_Defaults_Callback(Widget, XtPointer, XtPointer);
extern void     Arealight_Set_Defaults_Callback(Widget, XtPointer, XtPointer);

/*
**  Functions from main_view.c
*/
extern void Sensitize_Main_Buttons(Boolean, int);


/*  Menu functions. */
extern void Create_Edit_Menu(WindowInfoPtr);
extern void Edit_Objects_Function(Widget, XtPointer, XtPointer);
extern void Add_Instance_To_Edit(WindowInfoPtr, InstanceList, Boolean);
extern void Delete_Edit_Instance(WindowInfoPtr, InstanceList);


/*
**  Edit functions.
*/
extern void Edit_Instance(WindowInfoPtr, InstanceList);
extern void Edit_Window_Delete_Message();
extern Boolean  Edit_Obj_On_Stack(ObjectInstancePtr);
extern void Edit_Set_Cursor_Action(Widget, XEvent*, String*, Cardinal*);
extern void Edit_Start_Drag(Widget, XEvent*, String*, Cardinal*);
extern void Edit_Continue_Drag(Widget, XEvent*, String*, Cardinal*);
extern void Edit_Finish_Drag(Widget, XEvent*, String*, Cardinal*);
extern void Edit_Sensitize_Buttons(Boolean, int);
extern void Maintain_Toggle_Callback(Widget, XtPointer, XtPointer);


/*
**  Dependency functions.
*/
extern void Add_Dependency(ObjectInstancePtr, ObjectInstancePtr);
extern void Edit_Remove_Dependencies(ConstraintPtr, ObjectInstancePtr);
extern void Constraint_Change_References(ObjectInstancePtr,
                                         ObjectTestFunction, void*);
extern void Constraint_Remove_References(ObjectInstancePtr, ObjectInstancePtr);
extern void Dependencies_Remove_Object(ConstraintSpecPtr, ObjectInstancePtr,
                                       void*, void*, int);
extern void Remove_All_Object_Dependencies(ObjectInstancePtr);
extern void Constraint_Manipulate_Constraints(ObjectInstancePtr, void*, void*,
                                              SpecFunction);

/*
**  Constraint functions.
*/
extern void Constraint_Manipulate_Specs(ConstraintPtr, ObjectInstancePtr, void*,
                                        void*, int, SpecFunction);

/* Maintenance functions. */
extern void Maintain_Generic_Dynamic(struct _EditInfo*, int, VectorPtr,
                                     Transformation*);
extern void Maintain_Torus_Dynamic(struct _EditInfo*, int, VectorPtr,
                                   Transformation*);
extern void Maintain_Control_Obj_Dynamic(struct _EditInfo*, int, VectorPtr,
                                         Transformation*);
extern void Maintain_Point_Dynamic(struct _EditInfo*, int, VectorPtr,
                                   Transformation*);
extern void Maintain_Dir_Dynamic(struct _EditInfo*, int, VectorPtr,
                                 Transformation*);
extern Boolean  Maintain_Generic_Static(ObjectInstancePtr);
extern Boolean  Maintain_Torus_Static(ObjectInstancePtr);
extern Boolean  Maintain_Control_Obj_Static(ObjectInstancePtr);
extern Boolean  Maintain_Point_Static(ObjectInstancePtr);
extern Boolean  Maintain_Dir_Static(ObjectInstancePtr);

/*
**  Viewport changing functions.
*/
extern void Initiate_Viewfrom_Change(Widget, XtPointer, XtPointer);
extern void Initiate_Pan_Change(Widget, XtPointer, XtPointer);
extern void Initiate_Distance_Change(WindowInfoPtr, Boolean);
extern void Start_Newview_Rotation(Widget, XEvent*, String*, Cardinal*);
extern void Newview_Rotation(Widget, XEvent*, String*, Cardinal*);
extern void Stop_Newview_Rotation(Widget, XEvent*, String*, Cardinal*);
extern void Start_Distance_Change(Widget, XEvent*, String*, Cardinal*);
extern void Distance_Change(Widget, XEvent*, String*, Cardinal*);
extern void Stop_Distance_Change(Widget, XEvent*, String*, Cardinal*);
extern int  Change_Lookat_Point_Callback(ConstraintSpecPtr);
extern int  Change_Lookup_Point_Callback(ConstraintSpecPtr);

/*
**  Viewport saving and recalling.
*/
extern void View_Save_Current_Callback(Widget, XtPointer, XtPointer);
extern void View_Recall_Callback(Widget, XtPointer, XtPointer);
extern void View_Name_Action_Func(Widget, XEvent*, String*, Cardinal*);
extern void View_Save(Viewport*, char*);
extern void View_Delete_Callback(Widget, XtPointer, XtPointer);
extern void View_Reset();

/*  from events.c */
extern void Redraw_Main_View(Widget, XtPointer, XtPointer);
extern void Cancel_Viewport_Change(WindowInfoPtr);
extern void Change_Lookat_Callback(Widget, XtPointer, XtPointer);
extern void Change_Lookup_Callback(Widget, XtPointer, XtPointer);
extern void Cancel_Change_Look_Event(WindowInfoPtr);
extern void Apply_Viewfrom_Text(WindowInfoPtr);


/*
**  Camera related functions.
*/
extern void Camera_To_Viewport(Camera*, ViewportPtr);
extern void Camera_To_Window(WindowInfoPtr);
extern void Viewport_To_Camera(ViewportPtr, Widget, Camera*, Boolean);
extern void Camera_Create_Object();
extern void Camera_Set_Object_From_Camera(Boolean);
extern void Camera_Set_Camera_From_Object(Camera*);


/*
**  Object selection functions.
*/
extern void Start_Selection_Drag(Widget, XEvent*, String*, Cardinal*);
extern void Continue_Selection_Drag(Widget, XEvent*, String*, Cardinal*);
extern void Finish_Selection_Drag(Widget, XEvent*, String*, Cardinal*);


/*
**  Load functions.
*/
extern void Load_Dialog_Func(Widget, XtPointer, XtPointer);
extern void Merge_Dialog_Func(Widget, XtPointer, XtPointer);
extern void Load_Action_Func(Widget, XEvent*, String*, Cardinal*);
extern FILE *Open_Load_File_Name(char**);
extern void Sced_Load_File(FILE*, Boolean);

/*
**  Save callback function.
*/
extern void Save_Dialog_Func(Widget, XtPointer, XtPointer);
extern void Save_Action_Func(Widget, XEvent*, String*, Cardinal*);
extern void Save_Func(FILE*);
extern void Save_Auto_Callback(XtPointer, XtIntervalId*);
extern void Save_Auto_Cancel();
extern void Save_Auto_Startup();
extern int  Save_String(FILE*, char*);
extern void Save_Base(BaseObjectPtr, char*);


/*
**  Export functions.
*/
extern int  Export_File(FILE*, char*, ScenePtr, Boolean);
extern void Export_Callback(Widget, XtPointer, XtPointer);
extern void Export_Action_Func(Widget, XEvent*, String*, Cardinal*);

/*  Zoom action. */
extern void Zoom_Dialog_Func(Widget, XtPointer, XtPointer);
extern void Zoom_Action_Func(Widget, XEvent*, String*, Cardinal*);
extern void Magnify_Step_Action_Func(Widget, XEvent*, String*, Cardinal*);

/*
**  Quit function.
*/
extern void Quit_Dialog_Func(Widget, XtPointer, XtPointer);


/*
**  Delete function
*/
extern void Delete_Objects_Callback(Widget, XtPointer, XtPointer);


/*
**  misc functions.
*/
extern void Reset_Dialog_Func(Widget, XtPointer, XtPointer);
extern void Clear_Dialog_Func(Widget, XtPointer, XtPointer);
extern void Image_Size_Callback(Widget, XtPointer, XtPointer);

/*
**  Callback for rename button in rename dialog.
*/
extern void Rename_Action_Func(Widget, XEvent*, String*, Cardinal*);
extern void Rename_Callback(Widget, XtPointer, XtPointer);

/*
**  Layers callbacks and actions.
*/
extern void New_Layer_Action_Function(Widget, XEvent*, String*, Cardinal*);
extern void Merge_Layer_Action_Function(Widget, XEvent*, String*, Cardinal*);

/*
**  Functions to invoke the apply button at the bottom of the screen.
*/
extern void Apply_Button_Callback(Widget, XtPointer, XtPointer);
extern void Apply_Button_Action(Widget, XEvent*, String*, Cardinal*);

/*
**  Functions for creating/editing CSG objects.
*/
extern void Sensitize_CSG_Buttons(Boolean, int);
extern void Set_CSG_Related_Sensitivity(Boolean);
extern void CSG_Window_Popup(Widget, XtPointer, XtPointer);
extern void CSG_Window_Delete_Message();
extern void New_CSG_Instance(ObjectInstancePtr);
extern void CSG_Tree_Notify_Func(Widget, XEvent*, String*, Cardinal*);
extern void CSG_Menu_Button_Up_Func(Widget, XEvent*, String*, Cardinal*);
extern void CSG_Tree_Motion_Func(Widget, XEvent*, String*, Cardinal*);
extern void CSG_Complete_Action_Func(Widget, XEvent*, String*, Cardinal*);
extern void CSG_Reset();
extern char *Complete_Get_Name(char*);


/*
**  Functions for creating/editing aggregate objects.
*/
extern void Sensitize_Agg_Buttons(Boolean, int);
extern void Set_Agg_Related_Sensitivity(Boolean);
extern void Agg_Window_Popup(Widget, XtPointer, XtPointer);
extern void Agg_Window_Delete_Message();
extern void Agg_Edit_Object(ObjectInstancePtr);
extern void Agg_Reset();
extern void Agg_Complete_Action_Func(Widget, XEvent*, String*, Cardinal*);


/* Wireframe functions. For importing, exporting wireframes. */
extern void Wireframe_Load_Callback(Widget, XtPointer, XtPointer);
extern void Wireframe_Delete_Callback(Widget, XtPointer, XtPointer);
extern void Wireframe_Select_Popup(int);
extern void Wireframe_Add_Select_Option(BaseObjectPtr);
extern void Wireframe_Select_Destroy_Widget(int);
extern void Set_Wireframe_Related_Sensitivity(Boolean);

/* OFF export function. */
extern void OFF_Export_Base(BaseObjectPtr, char*);


/* Attributes functions. */
extern void Attributes_Change_String(InstanceList, char*, Boolean, Boolean);
extern void Set_Attributes_Callback(Widget, XtPointer, XtPointer);
extern void Attributes_Set_Defaults_Callback(Widget, XtPointer, XtPointer);
extern int  Save_Declarations(FILE*);
extern void Add_Declarations(char*);
extern void Clear_Declarations();
extern void Specific_Attributes_Callback();

/* Preferences. */
extern void Preferences_Callback(Widget, XtPointer, XtPointer);

/* Defaults. */
extern void Load_Defaults_File(char*);
extern void Defaults_Save_Preferences(ScedPreferencesPtr);
extern void Defaults_Pathname_Action(Widget, XEvent*, String*, Cardinal*);

/* Alias functions. */
extern void Alias_Object_Alias_Callback(Widget, XtPointer, XtPointer);

/*
**  Preview functions.
*/
extern void Preview_Callback(Widget, XtPointer, XtPointer);
extern void Perform_Preview(WindowInfoPtr, Renderer, InstanceList, int, int,
                            Boolean);
extern void Preview_Sensitize(Boolean);


/*
**  Action for renderman dialog.
*/
extern void Renderman_Action_Func(Widget, XEvent*, String*, Cardinal*);

/*
** File functions.
*/
extern FILE *File_Open(char*, char*);
extern FILE *File_Select_Open(char*, char*, char*, char**);
extern char *File_Select(char*, char*);
extern void File_Action_Function(Widget, XEvent*, String*, Cardinal*);

/*
** OpenGL stuff.
*/
extern void OpenGL_Export_Base(BaseObjectPtr, char*);

/* VRML base export. */
extern void VRML_Export_Base(BaseObjectPtr, char*);


/*
**  Elk functions, if required.
*/
#if ELK_SUPPORT
extern int  Elk_Sced_Init(int, char**);
extern void Elk_Eval_Action(Widget, XEvent*, String*, Cardinal*);
extern void Elk_Window_Popup(Widget, XtPointer, XtPointer);
extern void Elk_Window_Delete_Message();
#endif /* ELK_SUPPORT */

#endif /* __SCED_PROTO__ */
