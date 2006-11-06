/*******************************************************************************
FILE : computed_field_sigmoidImageFilter.h

LAST MODIFIED : 31 May 2001

DESCRIPTION :
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
 *   Carey Stevens carey@zestgroup.com
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
#if !defined (COMPUTED_FIELD_SIGMOIDIMAGEFILTER_H)
#define COMPUTED_FIELD_SIGMOIDIMAGEFILTER_H

int Computed_field_register_types_sigmoidImageFilter(
	struct Computed_field_package *computed_field_package);
/*******************************************************************************
LAST MODIFIED : 18 October 2006

DESCRIPTION :
==============================================================================*/

int Computed_field_set_type_sigmoidImageFilter(struct Computed_field *field,
	struct Computed_field *source_field, float min, float max, float alpha, float beta);
/*******************************************************************************
LAST MODIFIED : 18 October 2006

DESCRIPTION :
Converts <field> to type COMPUTED_FIELD_SIGMOIDIMAGEFILTER, returning the value of
<sigmoidImageFilter> at the time/parameter value given by scalar <source_field>.
Sets number of components to same number as <sigmoidImageFilter>.
If function fails, field is guaranteed to be unchanged from its original state,
although its cache may be lost.
???RC In future may not need to pass computed_field_manager it all fields
maintain pointer to it. Only have it to invoke computed field manager messages
in response to changes in the sigmoidImageFilter from the control sigmoidImageFilter manager.
==============================================================================*/

int Computed_field_get_type_sigmoidImageFilter(struct Computed_field *field,
	struct Computed_field **source_field, float *min, float *max, float *alpha, float *beta);
/*******************************************************************************
LAST MODIFIED : 18 October 2006

DESCRIPTION :
If the field is of type COMPUTED_FIELD_SIGMOIDIMAGEFILTER, the source_field and sigmoidImageFilter
used by it are returned - otherwise an error is reported.
==============================================================================*/

#endif /* !defined (COMPUTED_FIELD_SIGMOIDIMAGEFILTER_H) */