/*******************************************************************************
FILE : computed_field_finite_element.h

LAST MODIFIED : 25 May 2006

DESCRIPTION :
Implements computed fields which interface to finite element fields.
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
#if !defined (COMPUTED_FIELD_FINITE_ELEMENT_H)
#define COMPUTED_FIELD_FINITE_ELEMENT_H

#include "api/cmiss_field.h"

/*
Global types
------------
*/

struct Computed_field_finite_element_package;
/*******************************************************************************
LAST MODIFIED : 18 July 2000

DESCRIPTION :
Private package
==============================================================================*/

/* 
The Cmiss_computed_field which is Public is currently the same object as the 
cmgui internal Computed_field.  The Public interface is contained in 
api/cmiss_computed_field.h however most of the functions come directly from
this module.  So that these functions match the public declarations the
functions are given their public names.
*/

/* Convert the functions that have identical interfaces */
#define Computed_field_is_type_finite_element \
	Cmiss_field_is_type_finite_element
#define Computed_field_finite_element_define_at_node \
	Cmiss_field_finite_element_define_at_node
#define Computed_field_finite_element_set_string_at_node \
	Cmiss_field_finite_element_set_string_at_node

/*
Global functions
----------------
*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct Computed_field_finite_element_package *
	Computed_field_register_types_finite_element(
	struct Computed_field_package *computed_field_package);
/*******************************************************************************
LAST MODIFIED : 24 July 2008

DESCRIPTION :
This function registers the finite_element related types of Computed_fields.
==============================================================================*/

int Computed_field_is_type_finite_element(struct Computed_field *field);
/*******************************************************************************
LAST MODIFIED : 18 July 2000

DESCRIPTION :
==============================================================================*/

int Computed_field_is_type_finite_element_iterator(
	struct Computed_field *field, void *dummy_void);
/*******************************************************************************
LAST MODIFIED : 16 March 2007

DESCRIPTION :
Iterator/conditional function returning true if <field> is read only and a
wrapper for an FE_field.
==============================================================================*/

int Computed_field_finite_element_set_string_at_node(
	struct Computed_field *field, int component_number, struct FE_node *node, 
	double time, const char *string);
/*******************************************************************************
LAST MODIFIED : 24 May 2006

DESCRIPTION :
Special function for Computed_field_finite_element fields only.
Allows the setting of a string if that is the type of field represented.
==============================================================================*/

int Computed_field_finite_element_define_at_node(
	struct Computed_field *field, struct FE_node *node,
	struct FE_time_sequence *fe_time_sequence,
	struct FE_node_field_creator *node_field_creator);
/*******************************************************************************
LAST MODIFIED : 25 May 2006

DESCRIPTION :
Special function for Computed_field_finite_element fields only.
Defines the field at the specified node.
<fe_time_sequence> optionally defines multiple times for the <field>.  If it is
NULL then the field will be defined as constant for all times.
<node_field_creator> optionally defines different versions and/or derivative types.
If it is NULL then a single nodal value for each component will be defined.
==============================================================================*/

/*****************************************************************************//**
 * Creates a computed field wrapper for <fe_field>.
 * Makes the number of components the same as in the <fe_field>.
 * 
 * @param field_factory  Specifies owning region and other generic arguments.
 * @param fe_field  FE_field to be wrapped
 * @return Newly created field
 */
struct Computed_field *Computed_field_create_finite_element(
	struct Cmiss_field_factory *field_factory, struct FE_field *fe_field);

int Computed_field_get_type_finite_element(struct Computed_field *field,
	struct FE_field **fe_field);
/*******************************************************************************
LAST MODIFIED : 17 July 2000

DESCRIPTION :
If the field is of type COMPUTED_FIELD_FINITE_ELEMENT, the FE_field being
"wrapped" by it is returned - otherwise an error is reported.
==============================================================================*/

int Computed_field_is_read_only_with_fe_field(
	struct Computed_field *field, void *fe_field_void);
/*******************************************************************************
LAST MODIFIED : 22 January 2003

DESCRIPTION :
Iterator/conditional function returning true if <field> is read only and a
wrapper for <fe_field>.
???RC This looks ready for an overhaul since other fields in this module can
contain FE_fields.
==============================================================================*/

int Computed_field_contains_changed_FE_field(
	struct Computed_field *field, void *fe_field_change_log_void);
/*******************************************************************************
LAST MODIFIED : 22 January 2003

DESCRIPTION :
Returns true if <field> directly contains an FE_field and it is listed as
changed, added or removed in <fe_field_change_log>.
<fe_field_change_log_void> must point at a struct CHANGE_LOG<FE_field>.
==============================================================================*/

struct LIST(FE_field)
	*Computed_field_get_defining_FE_field_list(struct Computed_field *field);
/*******************************************************************************
LAST MODIFIED : 2 April 2003

DESCRIPTION :
Returns the list of FE_fields that <field> depends on.
==============================================================================*/

struct LIST(FE_field)
	*Computed_field_array_get_defining_FE_field_list(
		int number_of_fields, struct Computed_field **field_array);
/*******************************************************************************
LAST MODIFIED : 5 April 2006

DESCRIPTION :
Returns the compiled list of FE_fields that are required by any of
the <number_of_fields> fields in <field_array>.
==============================================================================*/

int Computed_field_is_type_cmiss_number(struct Computed_field *field);
/*******************************************************************************
LAST MODIFIED : 18 July 2000

DESCRIPTION :
==============================================================================*/

/*****************************************************************************//**
 * Creates a cmiss_number type fields whose value is the number of the element
 * or node location. 
 * 
 * @param field_factory  Specifies owning region and other generic arguments.
 * @return Newly created field
 */
struct Computed_field *Computed_field_create_cmiss_number(
	struct Cmiss_field_factory *field_factory);

int Computed_field_has_coordinate_fe_field(struct Computed_field *field,
	void *dummy);
/*******************************************************************************
LAST MODIFIED : 4 December 2001

DESCRIPTION :
Iterator/conditional function returning true if <field> is a wrapper for a
coordinate type fe_field.
==============================================================================*/

int Computed_field_has_element_xi_fe_field(struct Computed_field *field,
	void *dummy);
/*******************************************************************************
LAST MODIFIED : 2 June 2006

DESCRIPTION :
Iterator/conditional function returning true if <field> is a wrapper for an
element_xi type fe_field.
==============================================================================*/

int Computed_field_is_scalar_integer(struct Computed_field *field,
	void *dummy_void);
/*******************************************************************************
LAST MODIFIED : 20 June 2000

DESCRIPTION :
Returns true if <field> is a 1 integer component FINITE_ELEMENT wrapper.
==============================================================================*/

int Computed_field_is_scalar_integer_grid_in_element(
	struct Computed_field *field,void *element_void);
/*******************************************************************************
LAST MODIFIED : 26 May 2000

DESCRIPTION :
Returns true if <field> is a 1 integer component FINITE_ELEMENT wrapper which
is defined in <element> AND is grid-based.
Used for choosing field suitable for identifying grid points.
==============================================================================*/

int Computed_field_has_string_value_type(struct Computed_field *field,
	void *dummy_void);
/*******************************************************************************
LAST MODIFIED : 4 July 2002

DESCRIPTION :
Returns true if <field> is of string value type.
Currently only FE_fields can return string value type, hence this function is
restricted to this module.
Eventually, other computed fields will be of string type and this function will
belong elsewhere.
Note: not returning possible true result for embedded field as evaluate
at node function does not allow for string value either.
==============================================================================*/

int Computed_field_is_type_embedded(struct Computed_field *field, void *dummy);
/*******************************************************************************
LAST MODIFIED : 20 July 2000

DESCRIPTION :
==============================================================================*/

int Computed_field_depends_on_embedded_field(struct Computed_field *field);
/*******************************************************************************
LAST MODIFIED : 28 April 1999

DESCRIPTION :
Returns true if the field is of an embedded type or depends on any computed
fields which are or an embedded type.
==============================================================================*/

int Computed_field_manager_destroy_FE_field(
	struct MANAGER(Computed_field) *computed_field_manager,
	struct FE_field *fe_field);
/*******************************************************************************
LAST MODIFIED : 13 May 2003

DESCRIPTION :
Cleans up <fe_field> and its Computed_field wrapper if each are not in use.
==============================================================================*/

/*****************************************************************************//**
 * Creates a field returning the values for the given <nodal_value_type> and
 * <version_number> of <fe_field> at a node.
 * Makes the number of components the same as in the <fe_field>.
 * Field automatically takes the coordinate system of the source fe_field.
 * 
 * @param field_factory  Specifies owning region and other generic arguments.
 * @param fe_field  FE_field whose nodal parameters will be extracted
 * @param nodal_value_type  Parameter value type to extract
 * @param version_number  Version of parameter value to extract.
 * @return Newly created field
 */
struct Computed_field *Computed_field_create_node_value(
	struct Cmiss_field_factory *field_factory,
	struct FE_field *fe_field,enum FE_nodal_value_type nodal_value_type,
	int version_number);

int Computed_field_is_type_xi_coordinates(struct Computed_field *field,
	void *dummy_void);
/*******************************************************************************
LAST MODIFIED : 15 January 2002

DESCRIPTION :
==============================================================================*/

/*****************************************************************************//**
 * Creates a field whose return values are the xi coordinate location.
 * Currently fixed at 3 components, padding with zeros for lower dimension
 * elements.
 * 
 * @param field_factory  Specifies owning region and other generic arguments.
 * @return Newly created field
 */
struct Computed_field *Computed_field_create_xi_coordinates(
	struct Cmiss_field_factory *field_factory);

int Computed_field_is_type_node_value(struct Computed_field *field);
/*******************************************************************************
LAST MODIFIED : 19 July 2000

DESCRIPTION :
==============================================================================*/

int Computed_field_get_FE_field_time_array_index_at_FE_value_time(
	 struct Computed_field *field,FE_value time, FE_value *the_time_high,
	 FE_value *the_time_low, int *the_array_index,int *the_index_high,
	 int *the_index_low);
/*******************************************************************************
LAST MODIFIED : 9 Oct 2007

DESCRIPTION :
==============================================================================*/

struct FE_time_sequence *Computed_field_get_FE_node_field_FE_time_sequence(
	 struct Computed_field *computed_field, struct FE_node *node);
/*******************************************************************************
LAST MODIFIED : 9 Oct 2007

DESCRIPTION :
==============================================================================*/

struct FE_region_changes;

/***************************************************************************//**
 * Callback for changes to FE_region attached to a cmiss_region.
 * Updates definitions of Computed_field wrappers for changed FE_fields in the
 * region.
 * Also ensures region has cmiss_number and xi fields, at the appropriate time.
 * @private  Should only be called from cmiss_region.cpp!
 */
void Cmiss_region_FE_region_change(struct FE_region *fe_region,
	struct FE_region_changes *changes, void *cmiss_region_void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* !defined (COMPUTED_FIELD_FINITE_ELEMENT_H) */
