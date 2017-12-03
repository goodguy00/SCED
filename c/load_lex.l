/*
**  Sced: A Constraint Based Object Scene Editor
**
**  load_lex.l : The lex file that does lexical analysis for load.c
**
*/

%p 4000

%{

#define PATCHLEVEL 0

#include <sced.h>
#include <load.h>
#if ( HAVE_STRING_H )
#include <string.h>
#elif ( HAVE_STRINGS_H )
#include <strings.h>
#endif

void comment(void);
void read_string(void);

long   lex_int;
double lex_float;
char   *lex_string;

int     line_num = 1;

%}


SPACE       ("\t"|" ")
LETTER      [A-Z,a-z]
DIGIT       [0-9]
DOT         "."
IDENTIFIER  ({LETTER}|{DIGIT})({LETTER}|{DIGIT}|"_")*
INTEGER     "-"?{DIGIT}+
FRACTION    {DOT}{INTEGER}
EXPONENT    ([eE][+-]?{INTEGER})
REAL        {INTEGER}({FRACTION}{EXPONENT}?|{EXPONENT})|{FRACTION}{EXPONENT}?
NEWLINE     "\n"

%%


{SPACE}             ;
"#"                 comment();
"\""                {
                        read_string();
                        return STRING_TOKEN;
                    };
{REAL}              {
                        lex_float=atof(yytext);
                        return(FLOAT_TOKEN);
                    };
{INTEGER}           {
                        lex_int=atol(yytext);
                        return(INT_TOKEN);
                    };
"MainViewport"      return MAINVIEW_TOKEN;
"CSGViewport"       return CSGVIEW_TOKEN;
"Viewport"          return VIEWPORT_TOKEN;
"LookFrom"          return LOOKFROM_TOKEN;
"LookAt"            return LOOKAT_TOKEN;
"LookUp"            return LOOKUP_TOKEN;
"ViewDist"          return VIEWDIST_TOKEN;
"EyeDist"           return EYEDIST_TOKEN;
"Mode"              return MODE_TOKEN;
"BaseObjects"       return BASEOBJECTS_TOKEN;
"Instances"         return INSTANCES_TOKEN;
"Matrix"            return MATRIX_TOKEN;
"Scale"             return SCALE_TOKEN;
"Rotate"            return ROTATE_TOKEN;
"Union"             return UNION_TOKEN;
"Intersection"      return INTERSECTION_TOKEN;
"Difference"        return DIFFERENCE_TOKEN;
"Wireframe"         return WIREFRAME_TOKEN;
"Attributes"        return ATTRIBUTES_TOKEN;
"Color"             return COLOUR_TOKEN;
"Colour"            return COLOUR_TOKEN;
"Diffuse"           return DIFFUSE_TOKEN;
"Specular"          return SPECULAR_TOKEN;
"Reflect"           return REFLECT_TOKEN;
"Refract"           return REFRACT_TOKEN;
"Transparency"      return TRANSPARENCY_TOKEN;
"Camera"            return CAMERA_TOKEN;
"None"              return NONE_TOKEN;
"Rayshade"          return RAYSHADE_TOKEN;
"POVray"            return POVRAY_TOKEN;
"Genray"            return GENRAY_TOKEN;
"Genscan"           return GENSCAN_TOKEN;
"Radiance"          return RADIANCE_TOKEN;
"Renderman"         return RENDERMAN_TOKEN;
"HFOV"              return HFOV_TOKEN;
"VFOV"              return VFOV_TOKEN;
"Up"                return UP_TOKEN;
"Right"             return RIGHT_TOKEN;
"Screen"            return SCREEN_TOKEN;
"Magnify"           return MAGNIFY_TOKEN;
"Light"             return LIGHT_TOKEN;
"Ambient"           return AMBIENT_TOKEN;
"Position"          return POSITION_TOKEN;
"Reference"         return REFERENCE_TOKEN;
"Dependents"        return DEPENDENTS_TOKEN;
"Constraints"       return CONSTRAINTS_TOKEN;
"Plane"             return PLANE_TOKEN;
"Line"              return LINE_TOKEN;
"Point"             return POINT_TOKEN;
"Active"            return ACTIVE_TOKEN;
"Axes"              return AXES_TOKEN;
"Origin"            return ORIGIN_TOKEN;
"Allign"            return ALLIGN_TOKEN;
"Layer"             return LAYER_TOKEN;
"Directory"         return SCENEDIR_TOKEN;
"Default"           return DEFAULT_TOKEN;
"Mid"               return MID_TOKEN;
"Major"             return MAJOR_TOKEN;
"Minor"             return MINOR_TOKEN;
"Dense"             return DENSE_TOKEN;
"Full"              return FULL_TOKEN;
"CSG"               return CSG_TOKEN;
"Target"            return TARGET_TOKEN;
"Include"           return INCLUDES_TOKEN;
"Compress"          return COMPRESS_TOKEN;
"Declare"           return DECLARE_TOKEN;
"Extend"            return EXTEND_TOKEN;
"Open"              return OPEN_TOKEN;
"Version"           return VERS_TOKEN;
"Normal"            return NORMAL_TOKEN;
"Internal"          return INTERNAL_TOKEN;
"Object"            return OBJECT_TOKEN;
"\n"                line_num++;
{IDENTIFIER}        {
                        lex_string = yytext;
                        return ERROR_TOKEN;
                    }

%%

void comment()
{
    int ch;

    while (((ch=input()) != '\n') && (ch != 0));
    line_num++;
}

void read_string()
{
    int ch;
    int max, i;
    int last_ch = '\0';

    max = 24;
    lex_string = malloc(max);
    i = 0;

    while ((((ch=input()) != '"') || (last_ch == '\\')) && (ch != 0))
    {
        if (i == max)
        {
            max += 24;
            lex_string = realloc(lex_string, max);
        }

        if ( ch == '\n' ) line_num++;

        if ( ( last_ch == '\\' ) && ( ch != '"' ) )
            lex_string[i++] = last_ch;

        if ( ch != '\\' )
            lex_string[i++] = ch;

        last_ch = ch;
    }
    lex_string[i] = '\0';
}

