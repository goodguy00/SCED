# Generated automatically from Makefile.in by configure.
#
# $Header: /opt/sced/cvs/src/Makefile.in,v 1.1.1.1 1997/09/07 15:23:04 chipr Exp $
#
# $Log: Makefile.in,v $
# Revision 1.1.1.1  1997/09/07 15:23:04  chipr
# Original 1.0b2 sources
#
# Revision 1.0  1997/05/06 20:09:52  schenney
# Initial revision
#
#
SHELL=/bin/sh

.SUFFIXES:
.SUFFIXES: .c .o

srcdir=.
C_DIR=$(srcdir)/c
H_DIR=$(srcdir)/h

TARGET=sced

# Set CC and CPP
CC=gcc

#	Use the following defs for flex.
LEX=flex
LEXLIB=-lfl

# Library bulding functions.
AR=ar
RANLIB=ranlib

# Put the location of you X header files here, if necessary.
# eg:
# XINCLUDE=-I/usr/openwin/include
XINCLUDE= -I/usr/include/X11

# Add any extra include directories to the end of this line, using -Idir
INCLUDE=-I$(H_DIR) $(XINCLUDE)

# Add any extra C flags, such as -g for debug, here.
#OTHER_FLAGS=
OTHER_FLAGS=-ggdb

CFLAGS=$(OTHER_FLAGS) -O
ALL_CFLAGS=$(INCLUDE) $(CFLAGS)

# Add any extra libraries here.
LIBS=-lXaw -lXmu -lXext -lXt  -lSM -lICE -lX11  $(LEXLIB) -lm 

# Put the location of your X library files here, if necessary.
# eg:
# XLIBDIR=-L/usr/openwin/lib
XLIBDIR= -L/usr/lib64

LDFLAGS=$(OPTIMISE) $(XLIBDIR) 

# Various support specific flags.
POVRAY_OBJ=$(C_DIR)/povray.o
RADIANCE_OBJ=
RAYSHADE_OBJ=
RENDERMAN_OBJ=
VRML_OBJ=
PREV_OBJ=$(C_DIR)/load_old.o


OBJS=$(C_DIR)/ConstraintBox.o $(C_DIR)/ConstraintPt.o \
	 $(C_DIR)/SimpleWire.o $(C_DIR)/Vectorlib.o $(C_DIR)/View.o \
	 $(C_DIR)/add_constraint.o $(C_DIR)/add_dialogs.o \
	 $(C_DIR)/agg_view.o $(C_DIR)/aggregate.o $(C_DIR)/alias.o \
	 $(C_DIR)/align.o $(C_DIR)/attributes.o \
	 $(C_DIR)/base_objects.o $(C_DIR)/base_select.o \
	 $(C_DIR)/bezier.o \
	 $(C_DIR)/bounds.o $(C_DIR)/camera.o \
	 $(C_DIR)/constraint.o $(C_DIR)/constraint_list.o \
	 $(C_DIR)/constraint_pt.o $(C_DIR)/constr_update.o \
	 $(C_DIR)/control_pt.o \
	 $(C_DIR)/conversions.o $(C_DIR)/copy.o  \
	 $(C_DIR)/csg_combine.o $(C_DIR)/csg_edit.o \
	 $(C_DIR)/csg_events.o \
	 $(C_DIR)/csg_gen_wire.o $(C_DIR)/csg_intersect.o \
	 $(C_DIR)/csg_reference.o \
	 $(C_DIR)/csg_split.o $(C_DIR)/csg_tree.o $(C_DIR)/csg_view.o \
	 $(C_DIR)/csg_wire.o \
	 $(C_DIR)/defaults.o $(C_DIR)/delete.o $(C_DIR)/dense_wireframe.o \
	 $(C_DIR)/dependency.o \
	 $(C_DIR)/dfs.o $(C_DIR)/draw.o $(C_DIR)/edge_table.o \
	 $(C_DIR)/edit.o $(C_DIR)/edit_extras.o \
	 $(C_DIR)/edit_shell.o $(C_DIR)/edit_menu.o $(C_DIR)/edit_undo.o \
	 $(C_DIR)/events.o \
	 $(C_DIR)/export.o $(C_DIR)/features.o \
	 $(C_DIR)/files.o $(C_DIR)/gen_wireframe.o \
	 $(C_DIR)/hash.o \
	 $(C_DIR)/instance_list.o $(C_DIR)/instances.o \
	 $(C_DIR)/kd_tree.o \
	 $(C_DIR)/layers.o \
	 $(C_DIR)/light.o $(C_DIR)/load.o \
	 $(C_DIR)/load_internal.o $(C_DIR)/load_lex.o \
	 $(PREV_OBJ) \
	 $(C_DIR)/load_simple.o $(C_DIR)/main_view.o \
	 $(C_DIR)/maintain.o $(C_DIR)/misc.o $(C_DIR)/names.o \
	 $(C_DIR)/new_object.o $(C_DIR)/new_view.o \
	 $(C_DIR)/off.o \
	 $(C_DIR)/opengl_out.o \
	 $(C_DIR)/placement.o \
	 $(POVRAY_OBJ) \
	 $(C_DIR)/preferences.o \
	 $(C_DIR)/preview.o $(C_DIR)/quit.o \
	 $(RADIANCE_OBJ) \
	 $(C_DIR)/radius.o \
	 $(RAYSHADE_OBJ) $(RENDERMAN_OBJ) \
	 $(C_DIR)/rotate.o $(C_DIR)/save.o $(C_DIR)/sced.o \
	 $(C_DIR)/scale.o $(C_DIR)/select_point.o $(C_DIR)/selection.o \
	 $(C_DIR)/torus.o $(C_DIR)/triangle.o \
	 $(C_DIR)/utils.o \
	 $(VRML_OBJ) \
	 $(C_DIR)/view_recall.o $(C_DIR)/viewport.o \
	 $(C_DIR)/wireframe.o $(C_DIR)/zoom.o


all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

$(C_DIR)/load_lex.o: $(C_DIR)/load_lex.l
	$(LEX) -t $(C_DIR)/load_lex.l > $(C_DIR)/load_lex.c
	$(CC) $(ALL_CFLAGS) -c -o $(C_DIR)/load_lex.o $(C_DIR)/load_lex.c
	rm $(C_DIR)/load_lex.c

.c.o :
	$(CC) -c $(ALL_CFLAGS) -o $@ $<
	
clean:
	rm -f $(OBJS)
