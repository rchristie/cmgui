/*******************************************************************************
FILE : computed_field_connected_threshold_image_filter.c

LAST MODIFIED : 22 February 2007

DESCRIPTION :
Wraps itk::MeanImageFilter
==============================================================================*/
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is cmgui.
 *
 * The Initial Developer of the Original Code is
 * Auckland Uniservices Ltd, Auckland, New Zealand.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
extern "C" {
#include "computed_field/computed_field.h"
}
#include "computed_field/computed_field_private.hpp"
#include "image_processing/computed_field_ImageFilter.hpp"
extern "C" {
#include "computed_field/computed_field_set.h"
#include "general/debug.h"
#include "general/mystring.h"
#include "user_interface/message.h"
#include "image_processing/computed_field_connected_threshold_image_filter.h"
}

#include "itkImage.h"
#include "itkVector.h"
#include "itkConnectedThresholdImageFilter.h"

using namespace CMISS;

namespace {

char computed_field_connected_threshold_image_filter_type_string[] = "connected_threshold_image_filter";

class Computed_field_connected_threshold_image_filter : public Computed_field_ImageFilter
{

public:
	float lower_threshold;
	float upper_threshold;
  float replace_value;
	int x_seed;
	int y_seed;

	// Need something to represent index

	Computed_field_connected_threshold_image_filter(Computed_field *field,
  	float lower_threshold, float upper_threshold, float replace_value, int x_seed, int y_seed);

	~Computed_field_connected_threshold_image_filter()
	{
	}

private:
	Computed_field_core *copy(Computed_field* new_parent)
	{
		return new Computed_field_connected_threshold_image_filter(new_parent,
 		  lower_threshold, upper_threshold, replace_value, x_seed, y_seed);
	}

	char *get_type_string()
	{
		return(computed_field_connected_threshold_image_filter_type_string);
	}

	int compare(Computed_field_core* other_field);

	int list();

	char* get_command_string();
};

int Computed_field_connected_threshold_image_filter::compare(Computed_field_core *other_core)
/*******************************************************************************
LAST MODIFIED : 22 February 2007

DESCRIPTION :
Compare the type specific data.
==============================================================================*/
{
	Computed_field_connected_threshold_image_filter* other;
	int return_code;

	ENTER(Computed_field_connected_threshold_image_filter::compare);
	if (field && (other = dynamic_cast<Computed_field_connected_threshold_image_filter*>(other_core)))
	{
		if ((dimension == other->dimension)
			&& (lower_threshold == other->lower_threshold)
			&& (upper_threshold == other->upper_threshold)
			&& (x_seed == other->x_seed)
			&& (y_seed == other->y_seed)
			&& (replace_value == other->replace_value))
		{
			return_code = 1;
		}
		else
		{
			return_code = 0;
		}
	}
	else
	{
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_connected_threshold_image_filter::compare */

template < class ImageType >
class Computed_field_connected_threshold_image_filter_Functor :
	public Computed_field_ImageFilter_FunctorTmpl< ImageType >
/*******************************************************************************
LAST MODIFIED : 22 February 2007

DESCRIPTION :
This class actually does the work of processing images with the filter.
It is instantiated for each of the chosen ImageTypes.
==============================================================================*/
{
	Computed_field_connected_threshold_image_filter *connected_threshold_image_filter;

public:

	Computed_field_connected_threshold_image_filter_Functor(
		Computed_field_connected_threshold_image_filter *connected_threshold_image_filter) :
		Computed_field_ImageFilter_FunctorTmpl< ImageType >(connected_threshold_image_filter),
		connected_threshold_image_filter(connected_threshold_image_filter)
	{
	}

	int set_filter(Field_location* location)
/*******************************************************************************
LAST MODIFIED : 12 September 2006

DESCRIPTION :
Create a filter of the correct type, set the filter specific parameters
and generate the outputImage.
==============================================================================*/
	{
		int return_code;


		typedef itk::ConnectedThresholdImageFilter< ImageType , ImageType > FilterType;
		
		typename FilterType::Pointer filter = FilterType::New();
		
		filter->SetLower( connected_threshold_image_filter->lower_threshold );
		filter->SetUpper( connected_threshold_image_filter->upper_threshold );
		filter->SetReplaceValue( connected_threshold_image_filter->replace_value );

		typename ImageType::IndexType seedIndex;
		seedIndex[0]=connected_threshold_image_filter->x_seed;
		seedIndex[1]=connected_threshold_image_filter->y_seed;

		filter->SetSeed(seedIndex);
		
		return_code = connected_threshold_image_filter->update_output_image
			< ImageType, FilterType >
			(location, filter, this->outputImage);
		
		return (return_code);
	} /* set_filter */

}; /* template < class ImageType > class Computed_field_connected_threshold_image_filter_Functor */

Computed_field_connected_threshold_image_filter::Computed_field_connected_threshold_image_filter(
	Computed_field *field,
	float lower_threshold, float upper_threshold, float replace_value, int x_seed, int y_seed) :
	Computed_field_ImageFilter(field),
	lower_threshold(lower_threshold), upper_threshold(upper_threshold), replace_value(replace_value), x_seed(x_seed), y_seed(y_seed)
/*******************************************************************************
LAST MODIFIED : 22 February 2007

DESCRIPTION :
Create the computed_field representation of the connected threshold image filter.
==============================================================================*/
{
#if defined DONOTUSE_TEMPLATETEMPLATES
	create_filters_singlecomponent_multidimensions(
		Computed_field_connected_threshold_image_filter_Functor, this);
#else
	create_filters_singlecomponent_multidimensions
		< Computed_field_connected_threshold_image_filter_Functor,
		Computed_field_connected_threshold_image_filter >
		(this);
#endif
}

int Computed_field_connected_threshold_image_filter::list()
/*******************************************************************************
LAST MODIFIED : 22 February 2007

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(List_Computed_field_connected_threshold_image_filter);
	if (field)
	{
		display_message(INFORMATION_MESSAGE,
			"    source field : %s\n",field->source_fields[0]->name);
		display_message(INFORMATION_MESSAGE,
			"    lower_threshold : %g\n", lower_threshold);
		display_message(INFORMATION_MESSAGE,
			"    upper_threshold : %g\n", upper_threshold);
		display_message(INFORMATION_MESSAGE,
			"    replace_value : %g\n", replace_value);
		display_message(INFORMATION_MESSAGE,
			"    x_seed : %d\n", x_seed);
		display_message(INFORMATION_MESSAGE,
			"    y_seed : %d\n", y_seed);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"list_Computed_field_connected_threshold_image_filter.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* list_Computed_field_connected_threshold_image_filter */

char *Computed_field_connected_threshold_image_filter::get_command_string()
/*******************************************************************************
LAST MODIFIED : 22 February 2007

DESCRIPTION :
Returns allocated command string for reproducing field. Includes type.
==============================================================================*/
{
	char *command_string, *field_name, temp_string[40];
	int error;

	ENTER(Computed_field_connected_threshold_image_filter::get_command_string);
	command_string = (char *)NULL;
	if (field)
	{
		error = 0;
		append_string(&command_string, get_type_string(), &error);
		append_string(&command_string, " field ", &error);
		if (GET_NAME(Computed_field)(field->source_fields[0], &field_name))
		{
			make_valid_token(&field_name);
			append_string(&command_string, field_name, &error);
			DEALLOCATE(field_name);
		}
		sprintf(temp_string, " lower_threshold %g", lower_threshold);
		append_string(&command_string, temp_string, &error);		
		sprintf(temp_string, " upper_threshold %g", upper_threshold);	
		append_string(&command_string, temp_string, &error);		
		sprintf(temp_string, " replace_value %g", replace_value);	
		append_string(&command_string, temp_string, &error);		
		sprintf(temp_string, " x_seed %d", x_seed);	
		append_string(&command_string, temp_string, &error);		
		sprintf(temp_string, " y_seed %d", y_seed);	
		append_string(&command_string, temp_string, &error);		
}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_connected_threshold_image_filter::get_command_string.  Invalid field");
	}
	LEAVE;

	return (command_string);
} /* Computed_field_connected_threshold_image_filter::get_command_string */

} //namespace

int Computed_field_set_type_connected_threshold_image_filter(struct Computed_field *field,
  struct Computed_field *source_field, float lower_threshold, float upper_threshold, 
  float replace_value, int x_seed, int y_seed)
/*******************************************************************************
LAST MODIFIED : 22 February 2007

DESCRIPTION :
Converts <field> to type COMPUTED_FIELD_CONNECTED_THRESHOLD_IMAGE_FILTER.
The <lower_threshold> and <upper_threshold> specify at what value thresholding
occurs.
==============================================================================*/
{
	int number_of_source_fields, return_code;
	struct Computed_field **source_fields;

	ENTER(Computed_field_set_type_connected_threshold_image_filter);
	if (field && source_field &&
		Computed_field_is_scalar(source_field, (void *)NULL))
	{
		return_code = 1;
		/* 1. make dynamic allocations for any new type-specific data */
		number_of_source_fields = 1;
		if (ALLOCATE(source_fields, struct Computed_field *,
			number_of_source_fields))
		{
			/* 2. free current type-specific data */
			Computed_field_clear_type(field);
			/* 3. establish the new type */
			field->number_of_components = source_field->number_of_components;
			source_fields[0] = ACCESS(Computed_field)(source_field);
			field->source_fields = source_fields;
			field->number_of_source_fields = number_of_source_fields;			
			field->core = new Computed_field_connected_threshold_image_filter(field,lower_threshold, 
				upper_threshold, replace_value, x_seed, y_seed);
		}
		else
		{
			DEALLOCATE(source_fields);
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_set_type_connected_threshold_image_filter.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_set_type_connected_threshold_image_filter */

int Computed_field_get_type_connected_threshold_image_filter(struct Computed_field *field,
  struct Computed_field **source_field, float *lower_threshold, float *upper_threshold, 
  float *replace_value, int *x_seed, int *y_seed)
/*******************************************************************************
LAST MODIFIED : 22 February 2007

DESCRIPTION :
If the field is of type COMPUTED_FIELD_CONNECTED_THRESHOLD_IMAGE_FILTER, 
the source_field and thresholds used by it are returned - 
otherwise an error is reported.
==============================================================================*/
{
	Computed_field_connected_threshold_image_filter* core;
	int return_code;

	ENTER(Computed_field_get_type_connected_threshold_image_filter);
	if (field && (core = dynamic_cast<Computed_field_connected_threshold_image_filter*>(field->core))
		&& source_field)
	{
		*source_field = field->source_fields[0];
		*lower_threshold = core->lower_threshold;
		*upper_threshold = core->upper_threshold;
		*replace_value = core->replace_value;
		*x_seed = core->x_seed;
		*y_seed = core->y_seed;
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_get_type_connected_threshold_image_filter.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_get_type_connected_threshold_image_filter */

int define_Computed_field_type_connected_threshold_image_filter(struct Parse_state *state,
	void *field_void, void *computed_field_simple_package_void)
/*******************************************************************************
LAST MODIFIED : 30 August 2006

DESCRIPTION :
Converts <field> into type COMPUTED_FIELD_CONNECTED_THRESHOLD_IMAGE_FILTER (if it is not 
already) and allows its contents to be modified.
==============================================================================*/
{
	float lower_threshold, upper_threshold, replace_value;
  int x_seed, y_seed;
	int return_code;
	struct Computed_field *field, *source_field;
	struct Computed_field_simple_package *computed_field_simple_package;
	struct Option_table *option_table;
	struct Set_Computed_field_conditional_data set_source_field_data;

	ENTER(define_Computed_field_type_connected_threshold_image_filter);
	if (state && (field = (struct Computed_field *)field_void) &&
		(computed_field_simple_package = (Computed_field_simple_package*)computed_field_simple_package_void))
	{
		return_code = 1;
		/* get valid parameters for projection field */
		source_field = (struct Computed_field *)NULL;
		lower_threshold = 0.0;
		upper_threshold = 1.0;
		replace_value = 255;
		x_seed = 1;
		y_seed = 1;

		if (computed_field_connected_threshold_image_filter_type_string ==
			Computed_field_get_type_string(field))
		{
			return_code =
				Computed_field_get_type_connected_threshold_image_filter(field, &source_field,
				  &lower_threshold, &upper_threshold, &replace_value, &x_seed, &y_seed);
		}
		if (return_code)
		{
			/* must access objects for set functions */
			if (source_field)
			{
				ACCESS(Computed_field)(source_field);
			}

			option_table = CREATE(Option_table)();
			/* field */
			set_source_field_data.computed_field_manager =
				computed_field_simple_package->get_computed_field_manager();
			set_source_field_data.conditional_function = Computed_field_is_scalar;
			set_source_field_data.conditional_function_user_data = (void *)NULL;
			Option_table_add_entry(option_table, "field", &source_field,
				&set_source_field_data, set_Computed_field_conditional);
			/* lower_threshold */
			Option_table_add_float_entry(option_table, "lower_threshold",
				&lower_threshold);
			/* upper_threshold */
			Option_table_add_float_entry(option_table, "upper_threshold",
				&upper_threshold);
			/* replace_value */
			Option_table_add_float_entry(option_table, "replace_value",
				&replace_value);
			Option_table_add_int_positive_entry(option_table, "x_seed",
				&x_seed);
			Option_table_add_int_positive_entry(option_table, "y_seed",
				&y_seed);

			return_code = Option_table_multi_parse(option_table, state);
			DESTROY(Option_table)(&option_table);

			/* no errors,not asking for help */
			if (return_code)
			{
				if (!source_field)
				{
					display_message(ERROR_MESSAGE,
						"define_Computed_field_type_connected_threshold_image_filter.  "
						"Missing source field");
					return_code = 0;
				}
			}
			if (return_code)
			{
				return_code = Computed_field_set_type_connected_threshold_image_filter(
				  field, source_field, lower_threshold, upper_threshold, replace_value, x_seed, y_seed);				
			}
			
			if (!return_code)
			{
				if ((!state->current_token) ||
					(strcmp(PARSER_HELP_STRING, state->current_token)&&
						strcmp(PARSER_RECURSIVE_HELP_STRING, state->current_token)))
				{
					/* error */
					display_message(ERROR_MESSAGE,
						"define_Computed_field_type_connected_threshold_image_filter.  Failed");
				}
			}
			if (source_field)
			{
				DEACCESS(Computed_field)(&source_field);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"define_Computed_field_type_connected_threshold_image_filter.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* define_Computed_field_type_connected_threshold_image_filter */

int Computed_field_register_types_connected_threshold_image_filter(
	struct Computed_field_package *computed_field_package)
/*******************************************************************************
LAST MODIFIED : 30 August 2006

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	ENTER(Computed_field_register_types_connected_threshold_image_filter);
	if (computed_field_package)
	{
		return_code = Computed_field_package_add_type(computed_field_package,
			computed_field_connected_threshold_image_filter_type_string, 
			define_Computed_field_type_connected_threshold_image_filter,
			Computed_field_package_get_simple_package(computed_field_package));
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_register_types_connected_threshold_image_filter.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_register_types_connected_threshold_image_filter */