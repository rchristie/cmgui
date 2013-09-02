


/***************************************************************************//**
 * graphics_filter.hpp
 *
 * Declaration of scene graphic filter classes and functions.
 */
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
 * Portions created by the Initial Developer are Copyright (C) 2010
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

#ifndef GRAPHICS_FILTER_APP_HPP
#define GRAPHICS_FILTER_APP_HPP

#include "zinc/scene.h"
#include "general/list.h"
#include "general/manager.h"
#include "general/mystring.h"
#include "general/object.h"

struct cmzn_rendition;
struct cmzn_graphic;

DECLARE_LIST_TYPES(cmzn_graphics_filter);

PROTOTYPE_OBJECT_FUNCTIONS(cmzn_graphics_filter);
PROTOTYPE_GET_OBJECT_NAME_FUNCTION(cmzn_graphics_filter);

PROTOTYPE_LIST_FUNCTIONS(cmzn_graphics_filter);
PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(cmzn_graphics_filter,name,const char *);

PROTOTYPE_MANAGER_FUNCTIONS(cmzn_graphics_filter);
PROTOTYPE_MANAGER_IDENTIFIER_FUNCTIONS(cmzn_graphics_filter,name,const char *);


int cmzn_graphics_filter_manager_set_owner_private(struct MANAGER(cmzn_graphics_filter) *manager,
	struct cmzn_graphics_module *graphics_module);

int gfx_define_graphics_filter(struct Parse_state *state, void *root_region_void,
	void *filter_module_void);

int gfx_list_graphics_filter(struct Parse_state *state, void *dummy_to_be_modified,
	void *filter_module_void);

int set_cmzn_graphics_filter(struct Parse_state *state,
	void *graphics_filter_address_void, void *filter_module_void);
#endif /* GRAPHICS_FILTER_HPP_ */
