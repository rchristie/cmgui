/*******************************************************************************
FILE : value.c

LAST MODIFIED : 17 September 1999

DESCRIPTION :
A value type that knows what its type is.  So that can have a single function
for setting values and a single function for getting values and still have type
checking.
==============================================================================*/
#include "general/debug.h"
#include "command/parser.h"
#include "general/object.h"
#include "general/value.h"
#include "user_interface/message.h"

/*
Module types
------------
*/

/*
Module variables
----------------
*/

/*
Module functions
----------------
*/

/*
Global functions
----------------
*/

char *Value_type_string(enum Value_type value_type)
/*******************************************************************************
LAST MODIFIED : 17 September 1999

DESCRIPTION :
Returns a pointer to a static string describing the <value_type>, eg.
INT_VALUE == "integer". This string should match the command used
to create the edit object. The returned string must not be DEALLOCATEd!
==============================================================================*/
{
	char *return_string;

	ENTER(Value_type_string);
	switch (value_type)
	{
		case DOUBLE_ARRAY_VALUE:
		{
			return_string="double_array";
		} break;
		case DOUBLE_VALUE:
		{
			return_string="double";
		} break;
		case ELEMENT_XI_VALUE:
		{
			return_string="element_xi";
		} break;
		case FE_VALUE_ARRAY_VALUE:
		{
			return_string="real_array";
		} break;
		case FE_VALUE_VALUE:
		{
			return_string="real";
		} break;
		case FLT_ARRAY_VALUE:
		{
			return_string="float_array";
		} break;
		case FLT_VALUE:
		{
			return_string="float";
		} break;
		case INT_ARRAY_VALUE:
		{
			return_string="integer_array";
		} break;
		case INT_VALUE:
		{
			return_string="integer";
		} break;
		case SHORT_ARRAY_VALUE:
		{
			return_string="short_array";
		} break;
		case SHORT_VALUE:
		{
			return_string="short";
		} break;
		case STRING_VALUE:
		{
			return_string="string";
		} break;
		case UNSIGNED_ARRAY_VALUE:
		{
			return_string="unsigned_array";
		} break;
		case UNSIGNED_VALUE:
		{
			return_string="unsigned";
		} break;
		case URL_VALUE:
		{
			return_string="url";
		} break;
		default:
		{
			display_message(ERROR_MESSAGE,
				"Value_type_string.  Unknown value_type");
			return_string=(char *)NULL;
		} break;
	}
	LEAVE;

	return (return_string);
} /* Value_type_string */

enum Value_type Value_type_from_string(char *value_type_string)
/*******************************************************************************
LAST MODIFIED : 17 September 1999

DESCRIPTION :
Returns the value_type from the string, eg "integer" = INT_VALUE.
Eeturns UNKNOWN_VALUE without error if value_type_string not recognized.
==============================================================================*/
{
	char *compare_type_string;
	enum Value_type value_type;

	ENTER(Value_type_from_string);
	if (value_type_string)
	{
		value_type=VALUE_TYPE_BEFORE_FIRST;
		value_type++;
		while ((value_type<VALUE_TYPE_AFTER_LAST)&&
			(compare_type_string=Value_type_string(value_type))&&
			(!(fuzzy_string_compare_same_length(compare_type_string,
				value_type_string))))
		{
			value_type++;
		}
		if (!fuzzy_string_compare_same_length(compare_type_string,
			value_type_string))
		{
			value_type=UNKNOWN_VALUE;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Value_type_from_string.  Invalid argument(s)");
		value_type=UNKNOWN_VALUE;
	}
	LEAVE;

	return (value_type);
} /* Value_type_from_string */

char **Value_type_get_valid_strings_simple(int *number_of_valid_strings)
/*******************************************************************************
LAST MODIFIED : 10 May 2000

DESCRIPTION :
Returns and allocated array of pointers to all static strings for valid
Value_types - obtained from function Value_type_string.
Does not return any array types.
Up to calling function to deallocate returned array - but not the strings in it!
==============================================================================*/
{
	char **valid_strings;
	enum Value_type value_type;
	int i;

	ENTER(Value_type_get_valid_strings_simple);
	if (number_of_valid_strings)
	{
		*number_of_valid_strings=0;
		value_type=VALUE_TYPE_BEFORE_FIRST;
		value_type++;
		while (value_type<VALUE_TYPE_AFTER_LAST)
		{
			if (!Value_type_is_array(value_type))
			{
				(*number_of_valid_strings)++;
			}
			value_type++;
		}
		if (ALLOCATE(valid_strings,char *,*number_of_valid_strings))
		{
			value_type=VALUE_TYPE_BEFORE_FIRST;
			value_type++;
			i=0;
			while (value_type<VALUE_TYPE_AFTER_LAST)
			{
				if (!Value_type_is_array(value_type))
				{
					valid_strings[i]=Value_type_string(value_type);
					i++;
				}
				value_type++;
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"Value_type_get_valid_strings_simple.  Not enough memory");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Value_type_get_valid_strings_simple.  Invalid argument");
		valid_strings=(char **)NULL;
	}
	LEAVE;

	return (valid_strings);
} /* Value_type_get_valid_strings_simple */

int Value_type_is_array(enum Value_type value_type)
/*******************************************************************************
LAST MODIFIED : 16 September 1999

DESCRIPTION :
Returns true if the value_type is an array.
==============================================================================*/
{
	int return_code;

	ENTER(Value_type_is_array);
	return_code=
		(DOUBLE_ARRAY_VALUE==value_type)||
		(FE_VALUE_ARRAY_VALUE==value_type)||
		(FLT_ARRAY_VALUE==value_type)||
		(INT_ARRAY_VALUE==value_type)||
		(SHORT_ARRAY_VALUE==value_type)||
		(UNSIGNED_ARRAY_VALUE==value_type);
	LEAVE;

	return (return_code);
} /* Value_type_is_array */

enum Value_type Value_type_simple_to_array(enum Value_type value_type)
/*******************************************************************************
LAST MODIFIED : 16 September 1999

DESCRIPTION :
If the <value_type> is a non-array type with an array equivalent then the latter
is returned, otherwise an error is reported and the original type is returned.
==============================================================================*/
{
	enum Value_type return_value_type;

	ENTER(Value_type_simple_to_array);
	switch (value_type)
	{
		case DOUBLE_VALUE:
		{
			return_value_type=DOUBLE_ARRAY_VALUE;
		} break;
		case FE_VALUE_VALUE:
		{
			return_value_type=FE_VALUE_ARRAY_VALUE;
		} break;
		case FLT_VALUE:
		{
			return_value_type=FLT_ARRAY_VALUE;
		} break;
		case INT_VALUE:
		{
			return_value_type=INT_ARRAY_VALUE;
		} break;
		case SHORT_VALUE:
		{
			return_value_type=SHORT_ARRAY_VALUE;
		} break;
		case UNSIGNED_VALUE:
		{
			return_value_type=UNSIGNED_ARRAY_VALUE;
		} break;
		default:
		{
			display_message(ERROR_MESSAGE,
				"Value_type_simple_to_array.  No array equivalent for type %s",
				Value_type_string(value_type));
			return_value_type=value_type;
		} break;
	}
	LEAVE;

	return (return_value_type);
} /* Value_type_simple_to_array */

enum Value_type Value_type_array_to_simple(enum Value_type value_type)
/*******************************************************************************
LAST MODIFIED : 16 September 1999

DESCRIPTION :
If the <value_type> is an array type with a non-array equivalent then the latter
is returned, otherwise an error is reported and the original type is returned.
==============================================================================*/
{
	enum Value_type return_value_type;

	ENTER(Value_type_array_to_simple);
	switch (value_type)
	{
		case DOUBLE_ARRAY_VALUE:
		{
			return_value_type=DOUBLE_VALUE;
		} break;
		case FE_VALUE_ARRAY_VALUE:
		{
			return_value_type=FE_VALUE_VALUE;
		} break;
		case FLT_ARRAY_VALUE:
		{
			return_value_type=FLT_VALUE;
		} break;
		case INT_ARRAY_VALUE:
		{
			return_value_type=INT_VALUE;
		} break;
		case SHORT_ARRAY_VALUE:
		{
			return_value_type=SHORT_VALUE;
		} break;
		case UNSIGNED_ARRAY_VALUE:
		{
			return_value_type=UNSIGNED_VALUE;
		} break;
		default:
		{
			display_message(ERROR_MESSAGE,
				"Value_type_array_to_simple.  No non-array equivalent for type %s",
				Value_type_string(value_type));
			return_value_type=value_type;
		} break;
	}
	LEAVE;

	return (return_value_type);
} /* Value_type_array_to_simple */

