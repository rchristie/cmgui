/*******************************************************************************
FILE : light.h

LAST MODIFIED : 8 October 2002

DESCRIPTION :
The data structures used for representing graphical lights.
==============================================================================*/
#if !defined (LIGHT_H)
#define LIGHT_H

#include "general/enumerator.h"
#include "general/list.h"
#include "general/manager.h"
#include "general/object.h"
#include "graphics/colour.h"
#include "graphics/graphics_library.h"

/*
Global types
------------
*/

enum Light_type
/*******************************************************************************
LAST MODIFIED : 24 November 1994

DESCRIPTION :
The light type.  A SPOT light as position and direction, a INFINITE light has
direction only (parallel light from a source at infinity) and a POINT light has
position only.
==============================================================================*/
{
	INFINITE_LIGHT,
	POINT_LIGHT,
	SPOT_LIGHT
}; /* enum Light_type */

struct Light;
/*******************************************************************************
LAST MODIFIED : 4 December 1997

DESCRIPTION :
The properties of a light. The members of this object are private.
==============================================================================*/

DECLARE_LIST_TYPES(Light);

DECLARE_MANAGER_TYPES(Light);

struct Modify_light_data
/*******************************************************************************
LAST MODIFIED : 17 February 1997

DESCRIPTION :
==============================================================================*/
{
	struct Light *default_light;
	struct MANAGER(Light) *light_manager;
}; /* struct Modify_light_data */

/*
Global functions
----------------
*/

PROTOTYPE_ENUMERATOR_FUNCTIONS(Light_type);

struct Light *CREATE(Light)(char *name);
/*******************************************************************************
LAST MODIFIED : 4 December 1995

DESCRIPTION :
Allocates memory and assigns fields for a light.
==============================================================================*/

int DESTROY(Light)(struct Light **light_address);
/*******************************************************************************
LAST MODIFIED : 25 September 1995

DESCRIPTION :
Frees the memory for the light and sets <*light_address> to NULL.
==============================================================================*/

PROTOTYPE_OBJECT_FUNCTIONS(Light);
PROTOTYPE_GET_OBJECT_NAME_FUNCTION(Light);

PROTOTYPE_LIST_FUNCTIONS(Light);
PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(Light,name,char *);

PROTOTYPE_MANAGER_COPY_FUNCTIONS(Light,name,char *);
PROTOTYPE_MANAGER_FUNCTIONS(Light);
PROTOTYPE_MANAGER_IDENTIFIER_FUNCTIONS(Light,name,char *);

int get_Light_attenuation(struct Light *light, float *constant_attenuation,
	float *linear_attenuation, float *quadratic_attenuation);
/*******************************************************************************
LAST MODIFIED : 8 October 2002

DESCRIPTION :
Returns the constant, linear and quadratic attentuation factors which control
the falloff in intensity of <light> according to 1/(c + l*d + q*d*d)
where d=distance, c=constant, l=linear, q=quadratic.
Values 1 0 0 (ie. only constant attenuation of 1) gives no attenuation.
Infinite/directional lights are not affected by these values.
==============================================================================*/

int set_Light_attenuation(struct Light *light, float constant_attenuation,
	float linear_attenuation, float quadratic_attenuation);
/*******************************************************************************
LAST MODIFIED : 8 October 2002

DESCRIPTION :
Sets the constant, linear and quadratic attentuation factors which control
the falloff in intensity of <light> according to 1/(c + l*d + q*d*d)
where d=distance, c=constant, l=linear, q=quadratic.
Values 1 0 0 (ie. only constant attenuation of 1) gives no attenuation.
Infinite/directional lights are not affected by these values.
==============================================================================*/

int get_Light_colour(struct Light *light,struct Colour *colour);
/*******************************************************************************
LAST MODIFIED : 4 December 1997

DESCRIPTION :
Returns the colour colour of the light.
==============================================================================*/

int get_Light_colour(struct Light *light,struct Colour *colour);
/*******************************************************************************
LAST MODIFIED : 4 December 1997

DESCRIPTION :
Sets the colour of the light.
==============================================================================*/

int get_Light_direction(struct Light *light,float direction[3]);
/*******************************************************************************
LAST MODIFIED : 4 December 1997

DESCRIPTION :
Returns the direction of the light, relevent for infinite and spot lights.
==============================================================================*/

int set_Light_direction(struct Light *light,float direction[3]);
/*******************************************************************************
LAST MODIFIED : 4 December 1997

DESCRIPTION :
Sets the direction of the light, relevent for infinite and spot lights.
==============================================================================*/

int get_Light_position(struct Light *light,float position[3]);
/*******************************************************************************
LAST MODIFIED : 4 December 1997

DESCRIPTION :
Returns the position of the light, relevent for point and spot lights.
==============================================================================*/

int set_Light_position(struct Light *light,float position[3]);
/*******************************************************************************
LAST MODIFIED : 4 December 1997

DESCRIPTION :
Sets the position of the light, relevent for point and spot lights.
==============================================================================*/

int get_Light_spot_cutoff(struct Light *light, float *spot_cutoff);
/*******************************************************************************
LAST MODIFIED : 8 October 2002

DESCRIPTION :
Returns the spotlight cutoff angle in degrees from 0 to 90.
==============================================================================*/

int set_Light_spot_cutoff(struct Light *light, float spot_cutoff);
/*******************************************************************************
LAST MODIFIED : 8 October 2002

DESCRIPTION :
Sets the spotlight cutoff angle in degrees from 0 to 90.
==============================================================================*/

int get_Light_spot_exponent(struct Light *light, float *spot_exponent);
/*******************************************************************************
LAST MODIFIED : 8 October 2002

DESCRIPTION :
Returns the spotlight exponent which controls how concentrated the spotlight
becomes as one approaches its axis. A value of 0.0 gives even illumination
throughout the cutoff angle.
==============================================================================*/

int set_Light_spot_exponent(struct Light *light, float spot_exponent);
/*******************************************************************************
LAST MODIFIED : 8 October 2002

DESCRIPTION :
Sets the spotlight exponent which controls how concentrated the spotlight
becomes as one approaches its axis. A value of 0.0 gives even illumination
throughout the cutoff angle.
==============================================================================*/

int get_Light_type(struct Light *light,enum Light_type *light_type);
/*******************************************************************************
LAST MODIFIED : 4 December 1997

DESCRIPTION :
Returns the light_type of the light (infinite/point/spot).
==============================================================================*/

int set_Light_type(struct Light *light,enum Light_type light_type);
/*******************************************************************************
LAST MODIFIED : 4 December 1997

DESCRIPTION :
Sets the light_type of the light (infinite/point/spot).
==============================================================================*/

int modify_Light(struct Parse_state *state,void *light_void,
	void *modify_light_data_void);
/*******************************************************************************
LAST MODIFIED : 17 February 1997

DESCRIPTION :
Modifies the properties of a light.
==============================================================================*/

int list_Light(struct Light *light,void *dummy);
/*******************************************************************************
LAST MODIFIED : 21 September 1998

DESCRIPTION :
Writes the properties of the <light> to the command window.
==============================================================================*/

int list_Light_name(struct Light *light,void *preceding_text_void);
/*******************************************************************************
LAST MODIFIED : 14 October 1998

DESCRIPTION :
Writes the name of the <light> to the command window, preceded on each line by
the optional <preceding_text> string.
==============================================================================*/

int list_Light_name_command(struct Light *light,void *preceding_text_void);
/*******************************************************************************
LAST MODIFIED : 22 January 2002

DESCRIPTION :
Writes the name of the <light> to the command window, preceded on each line by
the optional <preceding_text> string. Makes sure quotes are put around the
name of the light if it contains any special characters.
Follows the light name with semicolon and carriage return.
==============================================================================*/

int reset_Lights(void);
/*******************************************************************************
LAST MODIFIED : 4 December 1997

DESCRIPTION :
Must be called at start of rendering before lights are activate with
execute_Light. Ensures all lights are off at the start of the rendering loop,
and makes sure the lights that are subsequently defined start at GL_LIGHT0...
==============================================================================*/

int set_Light(struct Parse_state *state,
	void *light_address_void,void *light_manager_void);
/*******************************************************************************
LAST MODIFIED : 11 December 1997

DESCRIPTION :
Modifier function to set the light from a command.
==============================================================================*/

int compile_Light(struct Light *light,void *dummy_void);
/*******************************************************************************
LAST MODIFIED : 4 December 1997

DESCRIPTION :
Struct Light iterator function which should make a display list for the light.
==============================================================================*/

int execute_Light(struct Light *light,void *dummy_void);
/*******************************************************************************
LAST MODIFIED : 4 December 1997

DESCRIPTION :
Struct Light iterator function for activating the <light>.
Does not use display lists. See comments with compile_Light, above.
==============================================================================*/

int Light_to_list(struct Light *light,void *list_of_lights_void);
/*******************************************************************************
LAST MODIFIED : 9 December 1997

DESCRIPTION :
Light iterator function for duplicating lists of lights.
==============================================================================*/

int Light_is_in_list(struct Light *light, void *light_list_void);
/*******************************************************************************
LAST MODIFIED : 30 May 2001

DESCRIPTION :
Returns true if <light> is in <light_list>.
==============================================================================*/

#endif
