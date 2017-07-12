/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/h/RCS/defs.h,v 1.0 1997/05/06 20:41:21 schenney Exp $
 *
 * $Log: defs.h,v $
 * Revision 1.0  1997/05/06 20:41:21  schenney
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
**  defs.h : global #defines
**
**  Created: 26/03/94
*/


#ifndef __SCED_DEFS__
#define __SCED_DEFS__

/* Flags for object things. Self explanatory. ;-) */
#define ObjVisible      (1)
#define ObjSelected     (1<<1)
#define ObjDepends      (1<<2)
#define ObjPending      (1<<3)
#define ObjAll          0xFFFF

/* Flags for features. */
#define FeaturePref1    (1)     /* Preferred constraint solution. */
#define FeaturePref2    (1<<1)
#define FeatureAll      0xFFFF


/* Flags for view updates. */
#define ViewNone        (0)     /* Just do the drawing, and nothing else.   */
#define CalcView        (1)     /* Recalculate the view coords.  This       */
                                /* implies CalcScreen.                      */
#define CalcScreen      (1<<1)  /* Recalculate the screen coords for the    */
                                /* objects.                                 */
#define RemoveHidden    (1<<2)  /* Draw with hidden lines removed for each  */
                                /* object.                                  */
#define JustExpose      (1<<3)  /* Called upon exposure exents. If the      */
                                /* off screen bitmap is for the right       */
                                /* window, just do a copy.                  */
#define SkipEdges       (1<<4)  /* Enable edge skipping, where a proportion */
                                /* of edges in large wireframes are not     */
                                /* drawn to make redraw faster.             */
                                /* This has not been implemented yet.       */
                                /* Any takers?                              */
#define NewSize         (1<<5)  /* The window has changed size, so          */
                                /* reallocate Pixmaps.                      */


/* Flags for the window state, which determines how mouse events are
** translated.
*/
#define NoneState           (0)
#define SelectionState      (1)
#define ViewpointState      (1<<1) 
#define PanState            (1<<2)
#define DistanceState       (1<<3) 
#define EyeState            (1<<4)
#define EditState           (1<<5)
#define LookState           (1<<6)
#define SelectPointState    (1<<7)

#define SelectionDragState  (1<<8)
#define ViewpointDragState  (1<<9)
#define DistanceDragState   (1<<10)
#define CtrlState           (1<<11)

#define AllState            0xFFFF


/* Flags for Modify_Instance_Attributes */
#define ModSimple       (1)
#define ModExtend       (1<<1)
#define ModLight        (1<<2)
#define ModSpotlight    (1<<3)
#define ModArealight    (1<<4)

/* Flags for base object selection. */
#define select_new      1
#define select_change   2


/* Flags for wireframe loading. */
#define WIRE_CREATE 1
#define WIRE_DELETE 2


/* Flags for saving. */
#define SAVE_ONLY 0
#define SAVE_QUIT 1
#define SAVE_LOAD 2
#define SAVE_RESET 3
#define SAVE_CLEAR 4
#define SAVE_NONAME 5

/* Window drawing modes. */
#define DRAW_ALL    1
#define DRAW_DASHED 2
#define DRAW_CULLED 3

/* Sensitization flags. */
#define SenNone         0
#define SenFile         1
#define SenCSG          (1<<1)
#define SenAgg          (1<<2)
#define SenWireframe    (1<<3)
#define SenObject       (1<<4)
#define SenLights       (1<<5)
#define SenView         (1<<6)
#define SenWindow       (1<<7)
#define SenLayers       (1<<8)
#define SenTarget       (1<<9)
#define SenCamera       (1<<10)
#define SenPreview      (1<<11)
#define SenClear        (1<<12)
#define SenEdit         (1<<13)
#define SenAll          0xFFFFFFFF  /* Actually more bits than all. */

#define SenCSGCSG       1
#define SenCSGObject    (1<<1)
#define SenCSGView      (1<<2)
#define SenCSGWindow    (1<<3)
#define SenCSGLayers    (1<<4)
#define SenCSGClear     (1<<5)
#define SenCSGEdit      (1<<6)
#define SenCSGTree      (1<<7)
#define SenCSGAll       0xFFFFFFFF  /* Actually more bits than all. */

#define SenAggCSG       1
#define SenAggComplete  (1<<2)
#define SenAggObject    (1<<2)
#define SenAggView      (1<<3)
#define SenAggWindow    (1<<4)
#define SenAggLayers    (1<<5)
#define SenAggClear     (1<<6)
#define SenAggEdit      (1<<7)
#define SenAggAll       0xFFFFFFFF  /* Actually more bits than all. */

#define SenEditFinish   1
#define SenEditSuspend  (1<<1)
#define SenEditUndo     (1<<2)
#define SenEditRedo     (1<<3)
#define SenEditMaintain (1<<4)
#define SenEditOrigin   (1<<5)
#define SenEditMajor    (1<<6)
#define SenEditMinor    (1<<7)
#define SenEditScaling  (1<<8)
#define SenEditRadius   (1<<9)
#define SenEditAll      0xFFFFFFFF  /* Actually more bits than all. */

#endif /* __SCED_DEFS__ */
