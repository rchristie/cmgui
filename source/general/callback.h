/*******************************************************************************
FILE : callback.h

LAST MODIFIED : 23 March 2000

DESCRIPTION :
Macro definition for lists of callbacks between objects.
==============================================================================*/
#if !defined (CALLBACK_H)
#define CALLBACK_H
#include <Xm/Xm.h>
#include "general/list.h"
#include "general/object.h"

/*
Global Types
------------
*/

/*???DB.  Should have a return_code ? */
typedef void Callback_procedure(Widget,void *,void *);

struct Callback_data
/*******************************************************************************
LAST MODIFIED : 4 January 1995

DESCRIPTION :
Contains all information necessary for a callback.
==============================================================================*/
{
	Callback_procedure *procedure;
	void *data;
}; /* struct Callback_data */



#if defined (FULL_NAMES)
#define CALLBACK_FUNCTION( callback_type ) callback_function_ ## callback_type
#else
#define CALLBACK_FUNCTION( callback_type ) cbf_ ## callback_type
#endif

#define TYPEDEF_CALLBACK_FUNCTION( callback_type , object_type , \
	call_data_type ) \
typedef void CALLBACK_FUNCTION(callback_type)(object_type,call_data_type,void *)

#if defined (FULL_NAMES)
#define CALLBACK_ITEM( callback_type ) callback_item_ ## callback_type
#else
#define CALLBACK_ITEM( callback_type ) cbi_ ## callback_type
#endif

#define DECLARE_CALLBACK_TYPE( callback_type ) \
struct CALLBACK_ITEM(callback_type) \

/*
Global functions
----------------
*/

#if defined (FULL_NAMES)
#define CALLBACK_LIST_CALL( callback_type ) callback_list_call_ ## callback_type
#else
#define CALLBACK_LIST_CALL( callback_type ) cblc_ ## callback_type
#endif

#define PROTOTYPE_CALLBACK_LIST_CALL_FUNCTION( callback_type , object_type , \
	call_data_type ) \
int CALLBACK_LIST_CALL(callback_type)( \
	struct LIST(CALLBACK_ITEM(callback_type)) *callback_list, \
	object_type object,call_data_type call_data)
/*******************************************************************************
LAST MODIFIED : 20 March 2000

DESCRIPTION :
Calls every callback in <callback_list> with <object> and <call_data>.
==============================================================================*/

#if defined (FULL_NAMES)
#define CALLBACK_LIST_ADD_CALLBACK( callback_type ) \
	callback_list_add_callback_ ## callback_type
#else
#define CALLBACK_LIST_ADD_CALLBACK( callback_type ) cblac_ ## callback_type
#endif

#define PROTOTYPE_CALLBACK_LIST_ADD_CALLBACK_FUNCTION(callback_type ) \
int CALLBACK_LIST_ADD_CALLBACK(callback_type)( \
	struct LIST(CALLBACK_ITEM(callback_type)) *callback_list, \
	CALLBACK_FUNCTION(callback_type) *function,void *user_data)
/*******************************************************************************
LAST MODIFIED : 20 March 2000

DESCRIPTION :
Adds a callback = <function> + <user_data> to <callback_list>.
==============================================================================*/

#if defined (FULL_NAMES)
#define CALLBACK_LIST_REMOVE_CALLBACK( callback_type ) \
	callback_list_remove_callback_ ## callback_type
#else
#define CALLBACK_LIST_REMOVE_CALLBACK( callback_type ) cblrc_ ## callback_type
#endif

#define PROTOTYPE_CALLBACK_LIST_REMOVE_CALLBACK_FUNCTION(callback_type) \
int CALLBACK_LIST_REMOVE_CALLBACK(callback_type)( \
	struct LIST(CALLBACK_ITEM(callback_type)) *callback_list, \
	CALLBACK_FUNCTION(callback_type) *function,void *user_data)
/*******************************************************************************
LAST MODIFIED : 20 March 2000

DESCRIPTION :
Removes a callback = <function> + <user_data> from <callback_list>.
=============================================================================*/

#define DECLARE_CALLBACK_TYPES( callback_type , object_type , call_data_type ) \
TYPEDEF_CALLBACK_FUNCTION(callback_type,object_type,call_data_type); \
DECLARE_CALLBACK_TYPE(callback_type); \
DECLARE_LIST_TYPES(CALLBACK_ITEM(callback_type))

/* note that most modules will not need to prototype functions in headers as
	 will use the following in wrappers or module functions */
#define PROTOTYPE_CALLBACK_FUNCTIONS( callback_type , object_type , \
	call_data_type ) \
PROTOTYPE_LIST_FUNCTIONS(CALLBACK_ITEM(callback_type)); \
PROTOTYPE_CALLBACK_LIST_CALL_FUNCTION(callback_type,object_type, \
	call_data_type); \
PROTOTYPE_CALLBACK_LIST_ADD_CALLBACK_FUNCTION(callback_type); \
PROTOTYPE_CALLBACK_LIST_REMOVE_CALLBACK_FUNCTION(callback_type)

#endif /* !defined (CALLBACK_H) */
