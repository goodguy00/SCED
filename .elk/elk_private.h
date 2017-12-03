/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/elk/RCS/elk_private.h,v 1.0 1997/05/06 20:44:17 schenney Exp $
 *
 * $Log: elk_private.h,v $
 * Revision 1.0  1997/05/06 20:44:17  schenney
 * Initial revision
 *
 */
/* Private elk header file. */

#ifndef __ELK_P__
#define __ELK_P__

#include <sced.h>
#include <aggregate.h>
#include <base_objects.h>
#include <csg.h>
#include <instance_list.h>

/*
 * Here we need to redefine Object, True, and False
 * since these are defined both by ELK and by X11.
 */
#define Object EObject
#ifdef True
#undef True
#endif
#ifdef False
#undef False
#endif

/* Window stuff. */
extern void	Elk_Create_Window();

#define ELK_MAIN_WINDOW 1
#define ELK_CSG_WINDOW 2
#define ELK_AGG_WINDOW 3
extern int		elk_active;
extern WindowInfoPtr    elk_window;


typedef struct {
    ObjectInstancePtr object3;
} Elkobject3;

typedef struct {
    Viewport *viewport;
} Elkviewport;

typedef struct {
    CSGNodePtr  csg_node;
} Elkcsgnode;

#define ELKOBJECT3(OBJ) ((Elkobject3 *) POINTER(OBJ))
#define ELKVIEWPORT(OBJ) ((Elkviewport *) POINTER(OBJ))
#define ELKCSGNODE(OBJ) ((Elkcsgnode *) POINTER(OBJ))


#include <scheme.h>

extern int  T_Object3d;
extern int  T_Viewport;
extern int  T_CSGNode;

extern Object   Sym_Union;
extern Object   Sym_Intersection;
extern Object   Sym_Difference;

extern Object	Sym_Global;
extern Object	Sym_Local;

/* Evaluation function. */
extern char *Elk_Eval(char*);
extern void Elk_Echo_Output(char*);

/* Utility. */
extern InstanceList Elk_List_To_Instances(Object);


/* Viewport callbacks. */
extern int      Elk_Viewport_Print(Object, Object, int, int, int);
extern int      Elk_Viewport_Equal(Object, Object);
extern int      Elk_Viewport_Equiv(Object, Object);
extern Object   Elk_Viewport_Create();
extern Object   Elk_Viewport_Destroy(Object);
extern Object   Elk_Viewport_Lookat(Object, Object, Object, Object);
extern Object   Elk_Viewport_Position(Object, Object, Object, Object);
extern Object   Elk_Viewport_Upvector(Object, Object, Object, Object);
extern Object   Elk_Viewport_Distance(Object, Object);
extern Object   Elk_Viewport_Eye(Object, Object);
extern Object   Elk_Viewport_Setup(Object);
extern Object   Elk_Viewport_Zoom(Object, Object);
extern Object   Elk_Viewport_To_Camera(Object);

/* Object3d callbacks. */
extern int      Elk_Object3d_Print(Object, Object, int, int, int);
extern int      Elk_Object3d_Equal(Object, Object);
extern int      Elk_Object3d_Equiv(Object, Object);
extern Object   Elk_Object3d_Create(Object);
extern Object   Elk_Object3d_Position(Object, Object, Object, Object);
extern Object   Elk_Object3d_Displace(Object, Object, Object, Object);
extern Object   Elk_Object3d_Scale(Object, Object, Object, Object);
extern Object   Elk_Object3d_Rotate(Object, Object, Object, Object);
extern Object   Elk_Object3d_Set_Control(Object, Object, Object, Object, Object,
					 Object);
extern Object   Elk_Object3d_Destroy(Object);
extern Object   Elk_Object3d_Wireframe_Query(Object);
extern Object   Elk_Object3d_Wireframe_Level(Object, Object);
extern Object   Elk_Object3d_Attribs_Define(Object, Object);
extern Object   Elk_Object3d_Color(Object, Object, Object, Object);
extern Object   Elk_Object3d_Diffuse(Object, Object);
extern Object   Elk_Object3d_Specular(Object, Object, Object);
extern Object   Elk_Object3d_Reflect(Object, Object);
extern Object   Elk_Object3d_Transparency(Object, Object, Object);

/* CSG callbacks. */
extern int      Elk_CSG_Print(Object, Object, int, int, int);
extern int      Elk_CSG_Equal(Object, Object);
extern int      Elk_CSG_Equiv(Object, Object);
extern Object   Elk_CSG_Node(Object);
extern Object   Elk_CSG_Display(Object);
extern Object   Elk_CSG_Hide(Object);
extern Object   Elk_CSG_Attach(Object, Object, Object);
extern Object   Elk_CSG_Complete(Object, Object);

/* Aggregate callbacks. */
extern Object	Elk_Agg_Complete(Object, Object);



#endif /* __ELK_P__ */
