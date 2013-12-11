/* OpenCMISS-Cmgui Application
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

int set_FE_field_component_FE_region(struct Parse_state *state,
	void *fe_field_component_address_void, void *fe_region_void);
/*******************************************************************************
LAST MODIFIED : 6 March 2003

DESCRIPTION :
FE_region wrapper for set_FE_field_component.
==============================================================================*/


int set_FE_field_conditional_FE_region(struct Parse_state *state,
	void *fe_field_address_void, void *parse_field_data_void);
/*******************************************************************************
LAST MODIFIED : 3 December 2002

DESCRIPTION :
FE_region wrapper for set_FE_field_conditional. <parse_field_data_void> points
at a struct Set_FE_field_conditional_FE_region_data.
==============================================================================*/

int set_FE_fields_FE_region(struct Parse_state *state,
	void *fe_field_order_info_address_void, void *fe_region_void);
/*******************************************************************************
LAST MODIFIED : 7 March 2003

DESCRIPTION :
FE_region wrapper for set_FE_fields.
==============================================================================*/

int Option_table_add_set_FE_field_from_FE_region(
	struct Option_table *option_table, const char *entry_string,
	struct FE_field **fe_field_address, struct FE_region *fe_region);
/*******************************************************************************
LAST MODIFIED : 11 March 2003

DESCRIPTION :
Adds an entry for selecting an FE_field.
==============================================================================*/

/**
 * Command modifier to translate a node identifier into an accessed node
 * object from fe_nodeset.
 */
int set_FE_node_FE_nodeset(struct Parse_state *state, void *node_address_void,
	void *fe_nodeset_void);

int set_FE_element_top_level_FE_region(struct Parse_state *state,
	void *element_address_void, void *fe_region_void);
/*******************************************************************************
LAST MODIFIED : 26 February 2003

DESCRIPTION :
A modifier function for specifying a top level element, used, for example, to
set the seed element for a xi_texture_coordinate computed_field.
==============================================================================*/
