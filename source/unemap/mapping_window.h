/*******************************************************************************
FILE : mapping_window.h

LAST MODIFIED : 26 November 2001

DESCRIPTION :
==============================================================================*/
#if !defined (MAPPING_WINDOW_H)
#define MAPPING_WINDOW_H

#include <stddef.h>
#if defined (MOTIF)
#include <X11/Intrinsic.h>
#include <X11/Xlib.h>
#include <Xm/Xm.h>
#endif /* defined (MOTIF) */
#include "unemap/map_dialog.h"
#include "unemap/mapping.h"

/*
Global types
------------
*/
enum Mapping_associate
/*******************************************************************************
LAST MODIFIED : 28 May 1992

DESCRIPTION :
The work area that the mapping window is currently associated with.
==============================================================================*/
{
	ACQUISITION_ASSOCIATE,
	ANALYSIS_ASSOCIATE
}; /* enum Mapping_associate */

#if defined (MOTIF)
struct Mapping_file_menu
/*******************************************************************************
LAST MODIFIED : 23 July 2001

DESCRIPTION :
The menu associated with the file button.
==============================================================================*/
{
	Widget save_configuration_button;
	Widget read_configuration_button;
	Widget read_bard_electrode_button;
	Widget set_default_configuration_button;
	Widget save_electrode_values_button;
}; /* struct Mapping_file_menu */
#endif /* defined (MOTIF) */

#if defined (MOTIF)
struct Mapping_print_menu
/*******************************************************************************
LAST MODIFIED : 23 May 2001

DESCRIPTION :
The menu associated with the print button.
==============================================================================*/
{
	Widget postscript_button;
	Widget rgb_button;
	Widget tiff_button;
	Widget jpg_button;
	Widget animate_rgb_button;
	Widget animate_tiff_button;
	Widget animate_jpg_button;
}; /* struct Mapping_print_menu */
#endif /* defined (MOTIF) */

struct Mapping_window
/*******************************************************************************
LAST MODIFIED : 1 February 2000

DESCRIPTION :
The mapping window object.
==============================================================================*/
{
#if defined (MOTIF)
	struct Scene_viewer *scene_viewer;
	Widget activation,window;
	Widget menu;
	Widget map_button;
#endif /* defined (MOTIF) */
	struct Map_dialog *map_dialog;
#if defined (MOTIF)
	Widget animate_button;
	Widget setup_button;
#endif /* defined (MOTIF) */
	struct Setup_dialog *setup_dialog;
#if defined (MOTIF)
	Widget modify_button;
	Widget page_button;
	Widget file_button;
	struct Mapping_file_menu file_menu;
	Widget projection_choice; /* the menu*/
	Widget projection_cylinder; /* choice in the above menu */	
	Widget projection_hammer; /* choice in the above menu */	
	Widget projection_polar; /* choice in the above menu */	
	Widget projection_patch; /* choice in the above menu */	
	Widget projection_3d; /* choice in the above menu */
	Widget region_choice;
	Widget region_pull_down_menu;
#endif /* defined (MOTIF) */
	int number_of_regions;
#if defined (MOTIF)
	WidgetList regions;
	Widget current_region;
	Widget print_button;
	struct Mapping_print_menu print_menu;
	Widget close_button;
#endif /* defined (MOTIF) */
	struct Map *map;
#if defined (MOTIF)
	Widget map_drawing_area_2d;
	Widget mapping_area;
	Widget mapping_area_2d,mapping_area_3d;
#endif /* defined (MOTIF) */
	struct Drawing_2d *map_drawing;
#if defined (MOTIF)
	Widget colour_or_auxiliary_drawing_area;
#endif /* defined (MOTIF) */
	struct Drawing_2d *colour_or_auxiliary_drawing;
#if defined (MOTIF)
	Widget colour_or_auxiliary_scroll_bar;
#endif /* defined (MOTIF) */
	struct Mapping_window **address,**current_mapping_window_address;
	char *open;
	struct Time_object *potential_time_object;
	struct Time_editor_dialog_struct *time_editor_dialog;
#if defined (UNEMAP_USE_3D)
#if defined (MOTIF)
	Widget map3d_interactive_tool_form;	
	Widget interactive_toolbar_widget;
	Widget map3d_viewing_form;
#endif /* defined (MOTIF) */
	/* interaction */
	struct MANAGER(Interactive_tool) *interactive_tool_manager;
	/* Note: interactive_tool is NOT accessed by graphics_window; the chooser
		 will update it if the current one is destroyed */
	struct Interactive_tool *interactive_tool,*transform_tool;
	/* used for getting updates from autoranging spectrums or the spectrum editor */
	void *spectrum_manager_callback_id;
#endif /* defined (UNEMAP_USE_3D) */
	struct User_interface *user_interface;	
}; /* struct Mapping_window */

/*
Global variables
----------------
*/
#if defined (MAPPING_WINDOW)
#if defined (MOTIF)
Widget mapping_file_read_select_shel=(Widget)NULL,
	mapping_file_read_select=(Widget)NULL;
#endif /* defined (MOTIF) */
#else
#if defined (MOTIF)
/*??? do these all need to be visible ? */
extern Widget mapping_file_read_select_shel,mapping_file_read_select;
#endif /* defined (MOTIF) */
#endif

/*
Global functions
----------------
*/

int open_mapping_window(struct Mapping_window **mapping_address,
#if defined (MOTIF)
	Widget activation,Widget parent,Widget *shell,Widget *outer_form,
#endif /* defined (MOTIF) */
	struct Mapping_window **current_mapping_window_address,char *open,
	enum Mapping_associate *current_associate,enum Map_type *map_type,
	enum Colour_option colour_option,enum Contours_option contours_option,
	enum Electrodes_label_type electrodes_label_type,enum Fibres_option fibres_option,
	enum Landmarks_option landmarks_option,enum Extrema_option extrema_option,
	int maintain_aspect_ratio,int print_spectrum,
	enum Projection_type projection_type,enum Contour_thickness contour_thickness,
	struct Rig **rig_address,int *event_number_address,
	int *potential_time_address,int *datum_address,int *start_search_interval,
	int *end_search_interval,
#if defined (MOTIF)
	Pixel identifying_colour,
#endif /* defined (MOTIF) */
	enum Mapping_associate associate,
#if defined (MOTIF)
	XtPointer set_mapping_region,XtPointer select_map_drawing_area,
	XtPointer select_colour_or_auxiliary_draw,XtPointer work_area,
#endif /* defined (MOTIF) */
	int screen_width,int screen_height,
	char *configuration_file_extension,char *postscript_file_extension,
	struct Map_drawing_information *map_drawing_information,
	struct User_interface *user_interface,struct Unemap_package *unemap_package,
	struct Electrical_imaging_event **first_eimaging_event,
	enum Signal_analysis_mode *analysis_mode);
/*******************************************************************************
LAST MODIFIED : 4 July 2001

DESCRIPTION :
If the mapping window does not exist then it is created with the specified
properties.  Then the mapping window is opened.
==============================================================================*/

int update_mapping_drawing_area(struct Mapping_window *mapping,int recalculate);
/*******************************************************************************
LAST MODIFIED : 30 May 2000

DESCRIPTION :
Calls draw_map_3d or update_mapping_drawing_area_2d depending upon
<mapping> ->map->projection_type
==============================================================================*/

int update_mapping_colour_or_auxili(struct Mapping_window *mapping);
/*******************************************************************************
LAST MODIFIED : 10 June 1992

DESCRIPTION :
The callback for redrawing the colour bar or auxiliary devices drawing area.
==============================================================================*/

int update_mapping_window_menu(struct Mapping_window *mapping);
/*******************************************************************************
LAST MODIFIED : 7 August 1992

DESCRIPTION :
Updates the mapping region menu to be consistent with the current rig.
==============================================================================*/

int update_map_from_manual_time_update(struct Mapping_window *mapping);
/*******************************************************************************
LAST MODIFIED : 23 November 2001

DESCRIPTION :
Sets recalculate and map->interpolation_type from 
map->draw_map_on_manual_time_update, and update the map.
Reset map->interpolation_type to it's initial value.
==============================================================================*/

int mapping_window_set_animation_buttons(struct Mapping_window *mapping);
/*******************************************************************************
LAST MODIFIED : 23 November 2001

DESCRIPTION :
Sets the animation buttons of the mapping window based upon map information
==============================================================================*/

int highlight_electrode_or_auxiliar(struct Device *device,
#if defined (UNEMAP_USE_NODES)
	struct FE_node *device_node,
#endif
	int electrode_number,	int auxiliary_number,struct Map *map,
	struct Mapping_window *mapping);
/*******************************************************************************
LAST MODIFIED : 5 September 2000

DESCRIPTION :
Highlights/dehighlights an electrode or an auxiliary device in the <mapping>
window.

==============================================================================*/

int Mapping_window_set_potential_time_object(struct Mapping_window *mapping,
	struct Time_object *potential_time_object);
/*******************************************************************************
LAST MODIFIED : 15 October 1998
DESCRIPTION :
==============================================================================*/

int mapping_window_kill_time_keeper_editor(struct Mapping_window *mapping);
/*******************************************************************************
LAST MODIFIED : 14 December 2001

DESCRIPTION :
If the <mapping> has a time keeper, stop it and destroy it's editor widget.
==============================================================================*/

int mapping_window_stop_time_keeper(struct Mapping_window *mapping);
/*******************************************************************************
LAST MODIFIED : 14 December 2001

DESCRIPTION :
If the <mapping> has a time keeper, stop it .
==============================================================================*/

#endif /* !defined (MAPPING_WINDOW_H) */
