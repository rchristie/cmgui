/*******************************************************************************
FILE : interactive_tool.h

LAST MODIFIED : 11 May 2000

DESCRIPTION :
Active CMGUI tools should create a wrapper Interactive_tool that supplies a
consistent way of choosing which one is to receive input from a given input
device, and listing which ones are active.
Only certain such tools will be capable of receiving input events from scene
viewer/mouse input and other devices, but many more will follow and change the
content of the global selections and objects with text input.
==============================================================================*/
#if !defined (INTERACTIVE_TOOL_H)
#define INTERACTIVE_TOOL_H

#include "general/list.h"
#include "general/manager.h"
#include "general/object.h"
#include "interaction/interactive_event.h"

/*
Global types
------------
*/

typedef void Interactive_event_handler(void *device_id,
	struct Interactive_event *event,void *user_data);
typedef int Interactive_tool_bring_up_dialog_function(void *user_data);
typedef Widget Interactive_tool_make_button_function(void *user_data,
	Widget parent);

struct Interactive_tool;
/*******************************************************************************
LAST MODIFIED : 10 April 2000

DESCRIPTION :
Wrapper object for CMGUI tools giving them a name, a consistent interface for
sending input device events to, and can be put in a manager.
The contents of this object are private.
==============================================================================*/

DECLARE_LIST_TYPES(Interactive_tool);
DECLARE_MANAGER_TYPES(Interactive_tool);

/*
Global functions
----------------
*/

struct Interactive_tool *CREATE(Interactive_tool)(char *name,char *display_name,
	Interactive_event_handler *interactive_event_handler,
	Interactive_tool_make_button_function *make_button_function,
	Interactive_tool_bring_up_dialog_function *bring_up_dialog_function,
	void *tool_data);
/*******************************************************************************
LAST MODIFIED : 11 May 2000

DESCRIPTION :
Creates an Interactive_tool with the given <name> and <icon>. If an
<interactive_event_handler> is supplied it is called to pass on any input
events to the interactive_tool, with the <tool_data> passed as the third
parameter.
==============================================================================*/

int DESTROY(Interactive_tool)(
	struct Interactive_tool **interactive_tool_address);
/*******************************************************************************
LAST MODIFIED : 10 April 2000

DESCRIPTION :
Destroys the Interactive_tool.
==============================================================================*/

PROTOTYPE_OBJECT_FUNCTIONS(Interactive_tool);
PROTOTYPE_LIST_FUNCTIONS(Interactive_tool);
PROTOTYPE_FIND_BY_IDENTIFIER_IN_LIST_FUNCTION(Interactive_tool,name,char *);
PROTOTYPE_GET_OBJECT_NAME_FUNCTION(Interactive_tool);

PROTOTYPE_MANAGER_COPY_FUNCTIONS(Interactive_tool,name,char *);
PROTOTYPE_MANAGER_FUNCTIONS(Interactive_tool);
PROTOTYPE_MANAGER_IDENTIFIER_FUNCTIONS(Interactive_tool,name,char *);

char *Interactive_tool_get_display_name(
	struct Interactive_tool *interactive_tool);
/*******************************************************************************
LAST MODIFIED : 11 May 2000

DESCRIPTION :
Returns the display_name of the tool = what should be shown on widget/window
descriptions of the tool. The actual name generally uses underscore _ characters
instead of spaces for more terse commands.
Up to calling function to DEALLOCATE the returned copy of the display_name.
==============================================================================*/

int Interactive_tool_handle_interactive_event(
	struct Interactive_tool *interactive_tool,void *device_id,
	struct Interactive_event *interactive_event);
/*******************************************************************************
LAST MODIFIED : 10 April 2000

DESCRIPTION :
Passes the <interactive_event> from <device_id> to the tool wrapped by the
<interactive_tool> object.
==============================================================================*/

Widget Interactive_tool_make_button(struct Interactive_tool *interactive_tool,
	Widget parent);
/*******************************************************************************
LAST MODIFIED : 8 April 2000

DESCRIPTION :
Makes and returns a toggle_button widget representing <interactive_tool> as a
child of <parent>. <parent> is expected to be a RowColumn widget with an entry
callback receiving value changes from the toggle_button.
==============================================================================*/

#endif /* !defined (INTERACTIVE_TOOL_H) */
