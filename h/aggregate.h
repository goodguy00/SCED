/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/h/RCS/aggregate.h,v 1.0 1997/05/06 20:40:51 schenney Exp $
 *
 * $Log: aggregate.h,v $
 * Revision 1.0  1997/05/06 20:40:51  schenney
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
**  Sced: A Constraint Based Object Scene Editor
**
**  aggregate.h: Header file for aggregate base objects.
*/

#ifndef _AGGREGATE_H_
#define _AGGREGATE_H_


#define AGG_CREATE 1
#define AGG_DELETE 2
#define AGG_COPY   3
#define AGG_EXPORT 4

#define AGG_MAJOR  1
#define AGG_ALL    2
#define AGG_OBJS   4
#define AGG_LIGHTS 8

typedef struct _Aggregate {
    InstanceList    children;
    Cuboid          agg_bound;
    } Aggregate, *AggregatePtr;


extern InstanceList Aggregate_Contains_Base(BaseObjectPtr, BaseObjectPtr,
                                            Boolean);
extern InstanceList Aggregate_Contains_Light(BaseObjectPtr, Boolean);
extern InstanceList Aggregate_Contains_Spotlight(BaseObjectPtr, Boolean);
extern InstanceList Aggregate_Contains_Arealight(BaseObjectPtr, Boolean);
extern InstanceList Aggregate_Contains_Pointlight(BaseObjectPtr, Boolean);
extern InstanceList Aggregate_Contains_Dirlight(BaseObjectPtr, Boolean);

extern void	    Aggregate_Destroy(AggregatePtr);

extern void         Agg_Complete_Callback(Widget, XtPointer, XtPointer);
extern void         Agg_Copy_Base_Object(Widget, BaseObjectPtr);
extern WireframePtr Agg_Generate_Wireframe(InstanceList, int, int);

extern void Agg_Update_List_References(InstanceList, InstanceList);


extern void Create_Agg_Display();

extern int  num_agg_base_objects;

#endif
