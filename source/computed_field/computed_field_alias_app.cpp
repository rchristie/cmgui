/* OpenCMISS-Cmgui Application
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "opencmiss/zinc/fieldalias.h"
#include "general/debug.h"
#include "general/message.h"
#include "command/parser.h"
#include "computed_field/computed_field_private.hpp"
#include "computed_field/computed_field_private_app.hpp"

const char computed_field_alias_type_string[] = "alias";

class Computed_field_alias_package : public Computed_field_type_package
{
public:
	cmzn_region *root_region;

	Computed_field_alias_package(cmzn_region *root_region)
	  : root_region(root_region)
	{
		ACCESS(cmzn_region)(root_region);
	}

	~Computed_field_alias_package()
	{
		DEACCESS(cmzn_region)(&root_region);
	}
};

/*****************************************************************************//**
 * Command modifier function for defining a field as type computed_field_alias
 * (if not already) and allowing its contents to be modified.
 *
 * @param state  The parse state containing the command tokens.
 * @param field_modify_void  Void pointer to Computed_field_modify_data containing
 *   the field and the region it will be added to.
 * @param computed_field_alias_package_void  Void pointer to
 *   Computed_field_alias_package containing root_region.
 * @return  1 on success, 0 on failure.
 */
int define_Computed_field_type_alias(Parse_state *state,
	void *field_modify_void, void *computed_field_alias_package_void)
{
	int return_code;
	Computed_field_alias_package *computed_field_alias_package;
	Computed_field_modify_data *field_modify;

	ENTER(define_Computed_field_type_alias);
	if (state&&(field_modify=(Computed_field_modify_data *)field_modify_void) &&
		(computed_field_alias_package =
			(Computed_field_alias_package *)computed_field_alias_package_void))
	{
		return_code = 1;
		char *original_field_path_and_name = NULL;

		Option_table *option_table = CREATE(Option_table)();
		Option_table_add_help(option_table,
			"Creates a field which is an alias for another field, which can be from another region. "
			"This is the main mechanism for reusing field definitions from other regions. "
			"The optional region path is an absolute path from the root region if preceded by / "
			"or relative to the current region if not.");
		Option_table_add_string_entry(option_table,"field",
			&original_field_path_and_name, " [[/]REGION_PATH/]FIELD_NAME");
		return_code = Option_table_multi_parse(option_table, state);
		DESTROY(Option_table)(&option_table);
		/* no errors,not asking for help */
		if (return_code)
		{
			cmzn_region *root_region = NULL;
			char *region_path = NULL;
			char *remainder = NULL;
			cmzn_region *region = NULL;
			Computed_field *original_field = (Computed_field *)NULL;

			if (original_field_path_and_name)
			{
				if (original_field_path_and_name[0] == CMZN_REGION_PATH_SEPARATOR_CHAR)
				{
					// absolute path
					root_region = computed_field_alias_package->root_region;
				}
				else
				{
					// relative path
					root_region = field_modify->get_region();
				}
				if (cmzn_region_get_partial_region_path(root_region,
					original_field_path_and_name, &region, &region_path, &remainder))
				{
					if (!(original_field = cmzn_region_find_field_by_name(region, remainder)))
					{
						display_message(ERROR_MESSAGE,
							"gfx define field alias:  Could not find field %s", original_field_path_and_name);
						display_parse_state_location(state);
						return_code = 0;
					}
				}
				else
				{
					return_code = 0;
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"gfx define field alias:  Must specify field to alias");
				display_parse_state_location(state);
				return_code = 0;
			}
			if (return_code)
			{
				return_code = field_modify->update_field_and_deaccess(
					cmzn_fieldmodule_create_field_alias(field_modify->get_field_module(),
						original_field));
			}
			if (original_field)
			{
				DEACCESS(Computed_field)(&original_field);
			}
			DEALLOCATE(region_path);
			DEALLOCATE(remainder);
		}
		if (!return_code)
		{
			if ((!state->current_token) ||
				(strcmp(PARSER_HELP_STRING, state->current_token)&&
					strcmp(PARSER_RECURSIVE_HELP_STRING, state->current_token)))
			{
				display_message(ERROR_MESSAGE, "gfx define field alias:  Failed");
			}
		}
		DEALLOCATE(original_field_path_and_name);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"define_Computed_field_type_alias.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* define_Computed_field_type_alias */

int Computed_field_register_type_alias(
	struct Computed_field_package *computed_field_package,
	struct cmzn_region *root_region)
{
	int return_code;
	Computed_field_alias_package *computed_field_alias_package =
		new Computed_field_alias_package(root_region);

	ENTER(Computed_field_register_type_alias);
	if (computed_field_package)
	{
		return_code = Computed_field_package_add_type(computed_field_package,
			computed_field_alias_type_string,
			define_Computed_field_type_alias,
			computed_field_alias_package);
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Computed_field_register_type_alias.  Invalid argument(s)");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* Computed_field_register_type_alias */
