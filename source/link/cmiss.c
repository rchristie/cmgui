/*******************************************************************************
FILE : cmiss.c

LAST MODIFIED : 23 January 2002

DESCRIPTION :
See cmiss.h for interface details.

Implementation details:

The 'reconstitution' works as a finite state machine.
We have a current
	node field info

The message structure forms a hierachy:

Node field information
	Number of fields
		Field name
		Field type
		Other info
		Number of components
			Component name
			Value index
			Number of derivatives
			Number of versions

Node
	Number
	Data


???GMH  Oi - how do node groups fit into this scheme?

NOTE
1 fsm == finite state machine
2 input and output for wormholes are from the point of view of the wormhole
==============================================================================*/
#include <stdlib.h>
#include <unistd.h>
#if defined (UNIX)
#include <sys/param.h>
#include <sys/utsname.h>
#endif /* defined (UNIX) */
#if defined (VMS)
#include <string.h>
#include <starlet.h>
#include <lib$routines.h>
#include <descrip.h>
#include <clidef.h>
#endif /* defined (VMS) */
#include <stdio.h>
#include "wormhole.h"
#include "link/cmiss.h"
#include "general/debug.h"
#include "user_interface/message.h"

/*???DB.  Enable data transfer */
#define NOT_TEMPORARY

/*
Module types
------------
*/
enum CMISS_connection_data_input_state
/*******************************************************************************
LAST MODIFIED : 3 September 2001

DESCRIPTION :
States occupied by the data stream (on input).
==============================================================================*/
{
	/* node, element etc */
	CMISS_DATA_INPUT_STATE_TYPE,
	/* about to receive element field info */
	CMISS_DATA_INPUT_STATE_ELEMENT_FIELD_INFO,
	/* about to receive element data */
	CMISS_DATA_INPUT_STATE_ELEMENT_DATA,
	/* about to receive deleted elements */
	CMISS_DATA_INPUT_STATE_ELEMENT_DELETE,
	/* about to receive element group number */
	CMISS_DATA_INPUT_STATE_ELEMENT_GROUP,
	/* about to receive element group data */
	CMISS_DATA_INPUT_STATE_ELEMENT_GROUP_DATA,
	/* about to receive deleted element groups */
	CMISS_DATA_INPUT_STATE_ELEMENT_GROUP_DELETE,
	/* about to receive node field info */
	CMISS_DATA_INPUT_STATE_NODE_FIELD_INFO,
	/* about to receive node data */
	CMISS_DATA_INPUT_STATE_NODE_DATA,
	/* about to receive deleted nodes */
	CMISS_DATA_INPUT_STATE_NODE_DELETE,
	/* about to receive node group number */
	CMISS_DATA_INPUT_STATE_NODE_GROUP,
	/* about to receive node group data */
	CMISS_DATA_INPUT_STATE_NODE_GROUP_DATA,
	/* about to receive deleted node groups */
	CMISS_DATA_INPUT_STATE_NODE_GROUP_DELETE
}; /* enum CMISS_connection_data_input_state */

struct CMISS_connection
/*******************************************************************************
LAST MODIFIED : 3 October 2001

DESCRIPTION :
Private structure representing the connection between cm and cmgui.
==============================================================================*/
{
	/* name of the machine this is running on */
	char *name;
	/* if this flag is set then cmgui does not wait for cm commands to complete,
		otherwise it does */
	char asynchronous_commands;
	/* if this flag is set then a cm command is in progress */
	char command_in_progress;
	/* if this flag is set then have received a cm quit message */
	char cm_quit;
	/* finite state information */
	enum CMISS_connection_data_input_state data_input_state;
	int data_type,template_node_number_of_fields;
	struct FE_element_field_info *current_element_field_info;
	struct FE_field **template_node_fields;
	struct FE_node *template_node;
	struct GROUP(FE_element) *new_element_group;
	struct GROUP(FE_node) *new_node_group;
	struct MANAGER(FE_element) *element_manager;
	struct MANAGER(FE_field) *fe_field_manager;
	struct FE_time *fe_time;
	struct MANAGER(FE_node) *data_manager;
	struct MANAGER(FE_node) *node_manager;
	struct MANAGER(GROUP(FE_element)) *element_group_manager;
	struct MANAGER(GROUP(FE_node)) *node_group_manager;
	struct MANAGER(GROUP(FE_node)) *data_group_manager;
	struct Prompt_window **prompt_window_address;
	struct User_interface *user_interface;
	struct Wh_input *command_input,*data_input,*prompt_input;
	struct Wh_output *command_output,*data_output,*prompt_output;
	void *data_manager_callback_id;
	void *element_manager_callback_id;
	void *node_manager_callback_id;
}; /* struct CMISS_connection */

enum CM_storage_array
/*******************************************************************************
LAST MODIFIED : 2 September 2001

DESCRIPTION :
The array that cm uses to store the array.
==============================================================================*/
{
	CM_ARRAY_UNKNOWN,
	CM_ARRAY_FIX,
	CM_ARRAY_LD,
	CM_ARRAY_WD,
	CM_ARRAY_XID,
	CM_ARRAY_XP,
	CM_ARRAY_YP,
	CM_ARRAY_ZD
}; /* enum CM_storage_array */

struct CM_field_information
/*******************************************************************************
LAST MODIFIED : 2 September 2001

DESCRIPTION :
Private structure containing field information needed by cm.
==============================================================================*/
{
	/* region */
	int nr;
	/* array */
	enum CM_storage_array array;
  union
  {
		struct
    {
      int nc,nhx_index,nx;
    } fix;
		struct
    {
      int nj_typ;
    } xp;
		struct
    {
      int iy,nc,nhx_index,nx;
    } yp;
  } indices;
}; /* struct CM_field_information */

/*
Module variables
----------------
*/
/* types of data */
#define CMISS_NODE_CODE 1
#define CMISS_ELEMENT_CODE 2
#define CMISS_NODE_GROUP_CODE 3
#define CMISS_DATA_CODE 4
#define CMISS_ELEMENT_GROUP_CODE 3
/* operations */
#define CMISS_CHANGE 1
#define CMISS_DELETE 2

#if defined (OLD_CODE)
#define MAX_DEFAULT_NODAL_VALUE_NAMES 11
static char *default_nodal_value_names[MAX_DEFAULT_NODAL_VALUE_NAMES]=
{
	"Value",
	"Deriv1",
	"Deriv2",
	"Deriv3",
	"Deriv11",
	"Deriv22",
	"Deriv33",
	"Deriv12",
	"Deriv23",
	"Deriv31",
	"Deriv123",
};
#endif /* defined (OLD_CODE) */

/*
Module functions
----------------
*/
static int compare_FE_field_cm_information(
	struct FE_field_external_information *information_1,
	struct FE_field_external_information *information_2)
/*******************************************************************************
LAST MODIFIED : 2 September 2001

DESCRIPTION :
Returns -1 if <information_1> is "less than" <information_2>, 0 if they are the
same and 1 if <information_1> is "greater than" <information_2>.
==============================================================================*/
{
	int return_code;
	struct CM_field_information *cm_information_1, *cm_information_2;

	ENTER(compare_FE_field_cm_information);
	return_code=0;
	if (information_1)
	{
		if (information_2)
		{
			if ((compare_FE_field_cm_information==information_1->compare)&&
				(compare_FE_field_cm_information==information_2->compare))
			{
				/* compare the cm information */
				cm_information_1=information_1->information;
				cm_information_2=information_2->information;
				if (cm_information_1->nr<cm_information_2->nr)
				{
					return_code=1;
				}
				else
				{
					if (cm_information_1->nr>cm_information_2->nr)
					{
						return_code= -1;
					}
					else
					{
						if (cm_information_1->array<cm_information_2->array)
						{
							return_code=1;
						}
						else
						{
							if (cm_information_1->array>cm_information_2->array)
							{
								return_code= -1;
							}
							else
							{
								switch (cm_information_1->array)
								{
									case CM_ARRAY_FIX:
									{
										/* nhx_index */
										if ((cm_information_1->indices).fix.nhx_index<
											(cm_information_2->indices).fix.nhx_index)
										{
											return_code=1;
										}
										else
										{
											if ((cm_information_1->indices).fix.nhx_index>
												(cm_information_2->indices).fix.nhx_index)
											{
												return_code= -1;
											}
											else
											{
												/* nx */
												if ((cm_information_1->indices).fix.nx<
													(cm_information_2->indices).fix.nx)
												{
													return_code=1;
												}
												else
												{
													if ((cm_information_1->indices).fix.nx>
														(cm_information_2->indices).fix.nx)
													{
														return_code= -1;
													}
													else
													{
														/* nc */
														if ((cm_information_1->indices).fix.nc<
															(cm_information_2->indices).fix.nc)
														{
															return_code=1;
														}
														else
														{
															if ((cm_information_1->indices).fix.nc>
																(cm_information_2->indices).fix.nc)
															{
																return_code= -1;
															}
															else
															{
																return_code=0;
															}
														}
													}
												}
											}
										}
									} break;
									case CM_ARRAY_XP:
									{
										if ((cm_information_1->indices).xp.nj_typ<
											(cm_information_2->indices).xp.nj_typ)
										{
											return_code=1;
										}
										else
										{
											if ((cm_information_1->indices).xp.nj_typ>
												(cm_information_2->indices).xp.nj_typ)
											{
												return_code= -1;
											}
											else
											{
												return_code=0;
											}
										}
									} break;
									case CM_ARRAY_YP:
									{
										/* nhx_index */
										if ((cm_information_1->indices).yp.nhx_index<
											(cm_information_2->indices).yp.nhx_index)
										{
											return_code=1;
										}
										else
										{
											if ((cm_information_1->indices).yp.nhx_index>
												(cm_information_2->indices).yp.nhx_index)
											{
												return_code= -1;
											}
											else
											{
												/* nx */
												if ((cm_information_1->indices).yp.nx<
													(cm_information_2->indices).yp.nx)
												{
													return_code=1;
												}
												else
												{
													if ((cm_information_1->indices).yp.nx>
														(cm_information_2->indices).yp.nx)
													{
														return_code= -1;
													}
													else
													{
														/* nc */
														if ((cm_information_1->indices).yp.nc<
															(cm_information_2->indices).yp.nc)
														{
															return_code=1;
														}
														else
														{
															if ((cm_information_1->indices).yp.nc>
																(cm_information_2->indices).yp.nc)
															{
																return_code= -1;
															}
															else
															{
																/* iy */
																if ((cm_information_1->indices).yp.iy<
																	(cm_information_2->indices).yp.iy)
																{
																	return_code=1;
																}
																else
																{
																	if ((cm_information_1->indices).yp.iy>
																		(cm_information_2->indices).yp.iy)
																	{
																		return_code= -1;
																	}
																	else
																	{
																		return_code=0;
																	}
																}
															}
														}
													}
												}
											}
										}
									} break;
									default:
									{
										return_code=0;
									} break;
								}
							}
						}
					}
				}
			}
			else
			{
				/*???RC This has compiler warnings until I cast the pointer-to-functions
					to void *.  Perhaps we are not meant to compare these? */
				if ((void *)(information_1->compare) < (void *)(information_2->compare))
				{
					return_code=1;
				}
				else
				{
					if ((void *)(information_1->compare) > (void *)(information_2->compare))
					{
						return_code= -1;
					}
					else
					{
						return_code=0;
					}
				}
			}
		}
		else
		{
			return_code=1;
		}
	}
	else
	{
		if (information_2)
		{
			return_code= -1;
		}
		else
		{
			return_code=0;
		}
	}
	LEAVE;

	return (return_code);
} /* compare_FE_field_cm_information */

static int destroy_FE_field_cm_information(
	struct FE_field_external_information **information)
/*******************************************************************************
LAST MODIFIED : 2 September 2001

DESCRIPTION :
Destroys the <information>.  Returns non-zero for success.
==============================================================================*/
{
	int return_code;

	ENTER(destroy_FE_field_cm_information);
	return_code=0;
	if (information)
	{
		return_code=1;
		if (*information)
		{
			DEALLOCATE((*information)->information);
			*information=(struct FE_field_external_information *)NULL;
		}
	}
	LEAVE;

	return (return_code);
} /* destroy_FE_field_cm_information */

static struct FE_field_external_information *duplicate_FE_field_cm_information(
	struct FE_field_external_information *information)
/*******************************************************************************
LAST MODIFIED : 2 September 2001

DESCRIPTION :
Duplicates the <information>.
==============================================================================*/
{
	struct CM_field_information *cm_information,*cm_information_duplicate;
	struct FE_field_external_information *duplicate;

	ENTER(duplicate_FE_field_cm_information);
	duplicate=(struct FE_field_external_information *)NULL;
	if (information&&(compare_FE_field_cm_information==information->compare)&&
		(cm_information=(struct CM_field_information *)(information->information)))
	{
		if (ALLOCATE(duplicate,struct FE_field_external_information,1))
		{
			if (ALLOCATE(cm_information_duplicate,struct CM_field_information,1))
			{
				duplicate->compare=compare_FE_field_cm_information;
				duplicate->destroy=destroy_FE_field_cm_information;
				duplicate->duplicate=duplicate_FE_field_cm_information;
				duplicate->information=(void *)cm_information_duplicate;
				cm_information_duplicate->nr=cm_information->nr;
				cm_information_duplicate->array=cm_information->array;
				switch (cm_information->array)
				{
					case CM_ARRAY_FIX:
					{
						(cm_information_duplicate->indices).fix.nc=
							(cm_information->indices).fix.nc;
						(cm_information_duplicate->indices).fix.nhx_index=
							(cm_information->indices).fix.nhx_index;
						(cm_information_duplicate->indices).fix.nx=
							(cm_information->indices).fix.nx;
					} break;
					case CM_ARRAY_XP:
					{
						(cm_information_duplicate->indices).xp.nj_typ=
							(cm_information->indices).xp.nj_typ;
					} break;
					case CM_ARRAY_YP:
					{
						(cm_information_duplicate->indices).yp.iy=
							(cm_information->indices).yp.iy;
						(cm_information_duplicate->indices).yp.nc=
							(cm_information->indices).yp.nc;
						(cm_information_duplicate->indices).yp.nhx_index=
							(cm_information->indices).yp.nhx_index;
						(cm_information_duplicate->indices).yp.nx=
							(cm_information->indices).yp.nx;
					} break;
				}
			}
			else
			{
				DEALLOCATE(duplicate);
			}
		}
	}
	LEAVE;

	return (duplicate);
} /* duplicate_FE_field_cm_information */

static char *reformat_fortran_string(char *string)
/*******************************************************************************
LAST MODIFIED : 25 October 1994

DESCRIPTION:
Convert fortran-style strings to C-style printf strings.
???DB.  Needs tidying.  Don't like returning result.
???GMH.  Copied from socket.c as socket.c is obsolete.
==============================================================================*/
{
	char *result,*result_char,*string_char;
	int i,in_text;

	ENTER(reformat_fortran_string);
	if (string)
	{
		if (ALLOCATE(result,char,strlen(string)+1))
		{
			string_char=string;
			result_char=result;
			in_text=0;
			for (i=strlen(string);i>0;i--)
			{
				switch (*string_char)
				{
					case '$': case '(': case ')': case ',':
					{
						if (in_text)
						{
							*result_char= *string_char;
							result_char++;
						}
					} break;
					case '\'':
					{
						in_text= !in_text;
					}  break;
					case '/':
					{
						if (in_text)
						{
							*result_char= *string_char;
							result_char++;
						}
						else
						{
							*result_char='\n';
							result_char++;
						}
					} break;
					default:
					{
						if (in_text)
						{
							*result_char= *string_char;
							result_char++;
						}
					} break;
				}
				string_char++;
			}
			*result_char='\0';
		}
		else
		{
			display_message(ERROR_MESSAGE,
				"reformat_fortran_string.  Insufficient memory for result");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"reformat_fortran_string.  Missing string");
		result=(char *)NULL;
	}
	LEAVE;

	return (result);
} /* reformat_fortran_string */

#define NUM_NODE_FIELD_INFO_DATA 4
#define NUM_COMPONENT_DATA 2
static int CMISS_connection_get_data(struct CMISS_connection *connection)
/*******************************************************************************
LAST MODIFIED : 3 September 2001

DESCRIPTION :
Receives any data from the output data wormhole.
==============================================================================*/
{
	char **component_names,*field_name,*group_name,region_name[]="region_xxxxx";
	double *temp_field_values;
	enum CM_field_type cm_field_type;
	FE_value *field_values;
	int component_data[NUM_COMPONENT_DATA],field_num,i,j,
		node_field_info_data[NUM_NODE_FIELD_INFO_DATA],number_of_components,
		number_of_fields,number_of_field_values,number_of_values,primary_id,
		return_code,secondary_id;
	struct CM_field_information cm_information;
	struct Coordinate_system coordinate_system;
	struct FE_field *field,**template_node_fields;
	struct FE_field_external_information external;
	struct FE_node *node,*old_node,*template_node;
	struct FE_node_field_creator *node_field_creator;
	struct GROUP(FE_node) *old_node_group;
	struct MANAGER(FE_node) *FE_node_manager;

	ENTER(CMISS_connection_get_data);
	return_code=0;
	FE_node_manager = (struct MANAGER(FE_node) *)NULL;
	if (connection)
	{
/*???DB.  Temporary to disable data transfer */
#if defined (NOT_TEMPORARY)
		/* what is our current state */
		switch (connection->data_input_state)
		{
			case CMISS_DATA_INPUT_STATE_TYPE:
			{
				/* check for any data messages to be acted upon */
				if (wh_output_can_open(connection->data_output))
				{
					wh_output_open_message(connection->data_output,&primary_id,
						&secondary_id);
					switch (primary_id)
					{
						case CMISS_NODE_CODE: /* node information */
						{
							/* cheat and used duplicated code with data_type set to data */
							connection->data_type = CMISS_NODE_CODE;
							switch (secondary_id)
							{
								case CMISS_CHANGE: /* node details */
								{
									connection->data_input_state=
										CMISS_DATA_INPUT_STATE_NODE_FIELD_INFO;
								} break;
								case CMISS_DELETE: /* node delete */
								{
									connection->data_input_state=
										CMISS_DATA_INPUT_STATE_NODE_DELETE;
								} break;
								default:
								{
									display_message(ERROR_MESSAGE,
										"CMISS_connection_get_data.  %s",
										"state: type.  Invalid secondary type operation");
								} break;
							}
						} break;
						case CMISS_DATA_CODE: /* data information */
						{
							/* cheat and used duplicated code with data_type set to data */
							connection->data_type = CMISS_DATA_CODE;
							switch (secondary_id)
							{
								case CMISS_CHANGE: /* data details */
								{
									connection->data_input_state=
										CMISS_DATA_INPUT_STATE_NODE_FIELD_INFO;
								} break;
								case CMISS_DELETE: /* data delete */
								{
									connection->data_input_state=
										CMISS_DATA_INPUT_STATE_NODE_DELETE;
								} break;
								default:
								{
									display_message(ERROR_MESSAGE,
										"CMISS_connection_get_data.  %s",
										"state: type.  Invalid secondary type operation");
								} break;
							}
						} break;
						case CMISS_NODE_GROUP_CODE: /* node group information */
						{
							switch (secondary_id)
							{
								case CMISS_CHANGE: /* node group details */
								{
									connection->data_input_state=
										CMISS_DATA_INPUT_STATE_NODE_GROUP;
								} break;
								case CMISS_DELETE: /* node group delete */
								{
									connection->data_input_state=
										CMISS_DATA_INPUT_STATE_NODE_GROUP_DELETE;
								} break;
								default:
								{
									display_message(ERROR_MESSAGE,
										"CMISS_connection_get_data.  %s",
										"state: type.  Invalid secondary type operation");
								} break;
							}
						} break;
						default:
						{
							display_message(ERROR_MESSAGE,"CMISS_connection_get_data.  %s",
								"state: type.  Invalid primary type operation");
						} break;
					}
				}
			} break;
			case CMISS_DATA_INPUT_STATE_NODE_FIELD_INFO:
			{
				return_code=1;
				if (template_node=CREATE(FE_node)(0,(struct FE_node *)NULL))
				{
					/* get the number of fields */
					wh_output_get_int(connection->data_output,1,&number_of_fields);
					if ((0<number_of_fields)&&ALLOCATE(template_node_fields,
						struct FE_field *,number_of_fields))
					{
						for (i=0;i<number_of_fields;i++)
						{
							template_node_fields[i]=(struct FE_field *)NULL;
						}
						for (field_num=0;field_num<number_of_fields;field_num++)
						{
							/* get the field name */
							if (field_name=wh_output_get_unknown_string(
								connection->data_output))
							{
								/* get miscellaneous information */
								wh_output_get_int(connection->data_output,
									NUM_NODE_FIELD_INFO_DATA,node_field_info_data);
								/* convert CM field type */
								switch (node_field_info_data[0])
								{
									case 1:
									{
										cm_field_type=CM_ANATOMICAL_FIELD;
									} break;
									case 2:
									{
										cm_field_type=CM_COORDINATE_FIELD;
									} break;
									case 3:
									{
										cm_field_type=CM_GENERAL_FIELD;
									} break;
									default:
									{
										display_message(WARNING_MESSAGE,
											"CMISS_connection_get_data.  %s",
											"state: node field info.  Invalid CM field type");
									} break;
								}
								/* get the coordinate system */
								coordinate_system.type=
									(enum Coordinate_system_type)node_field_info_data[1];
								switch (get_coordinate_system_type(&coordinate_system))
								{
									case RECTANGULAR_CARTESIAN:
									case CYLINDRICAL_POLAR:
									case SPHERICAL_POLAR:
									case PROLATE_SPHEROIDAL:
									case OBLATE_SPHEROIDAL:
									case FIBRE:
									{
										/* do nothing */
									} break;
									default:
									{
										display_message(WARNING_MESSAGE,
											"CMISS_connection_get_data.  %s",
											"state: node field info.  Invalid coordinate system");
									} break;
								}
								/* get the number of field values */
								number_of_field_values=node_field_info_data[2];
								/* get the number of components */
								number_of_components=node_field_info_data[3];
								/* possibly get some field values */
								if (number_of_field_values)
								{
									if (ALLOCATE(temp_field_values,double,
										number_of_field_values)&&ALLOCATE(field_values,FE_value,
										number_of_field_values))
									{
										wh_output_get_double(connection->data_output,
											number_of_field_values,temp_field_values);
										/* perform type conversion */
										for (i=0;i<number_of_field_values;i++)
										{
											field_values[i]=temp_field_values[i];
										}
										DEALLOCATE(temp_field_values);
									}
									else
									{
										field_values=(FE_value *)NULL;
										display_message(ERROR_MESSAGE,
											"CMISS_connection_get_data.  %s",
											"state: node field info.  Could not allocate memory");
									}
								}
								else
								{
									field_values=(FE_value *)NULL;
								}
								/* get cm field information */
								external.compare=compare_FE_field_cm_information;
								external.destroy=destroy_FE_field_cm_information;
								external.duplicate=duplicate_FE_field_cm_information;
								external.information=(void *)&cm_information;
								wh_output_get_int(connection->data_output,2,
									node_field_info_data);
								cm_information.nr=node_field_info_data[0];
								if (CMISS_NODE_CODE==connection->data_type)
								{
									/* node */
									switch (node_field_info_data[1])
									{
										case 1:
										{
											cm_information.array=CM_ARRAY_XP;
											wh_output_get_int(connection->data_output,1,
												node_field_info_data);
											cm_information.indices.xp.nj_typ=node_field_info_data[0];
										} break;
										case 2:
										{
											cm_information.array=CM_ARRAY_YP;
											wh_output_get_int(connection->data_output,4,
												node_field_info_data);
											cm_information.indices.yp.nx=node_field_info_data[0];
											cm_information.indices.yp.nhx_index=
												node_field_info_data[1];
											cm_information.indices.yp.nc=node_field_info_data[2];
											cm_information.indices.yp.iy=node_field_info_data[3];
										} break;
										case 3:
										{
											cm_information.array=CM_ARRAY_FIX;
											wh_output_get_int(connection->data_output,3,
												node_field_info_data);
											cm_information.indices.yp.nx=node_field_info_data[0];
											cm_information.indices.yp.nhx_index=
												node_field_info_data[1];
											cm_information.indices.yp.nc=node_field_info_data[2];
										} break;
										default:
										{
											cm_information.array=CM_ARRAY_UNKNOWN;
										} break;
									}
								}
								else
								{
									/* data */
									switch (node_field_info_data[1])
									{
										case 1:
										{
											cm_information.array=CM_ARRAY_ZD;
										} break;
										case 2:
										{
											cm_information.array=CM_ARRAY_WD;
										} break;
										case 3:
										{
											cm_information.array=CM_ARRAY_LD;
										} break;
										case 4:
										{
											cm_information.array=CM_ARRAY_XID;
										} break;
										default:
										{
											cm_information.array=CM_ARRAY_UNKNOWN;
										} break;
									}
								}
								/* get components */
								ALLOCATE(component_names,char *,number_of_components);
								node_field_creator = CREATE(FE_node_field_creator)
									(number_of_components);
								if (component_names&&node_field_creator)
								{
									for (i=0;i<number_of_components;i++)
									{
										if (component_names[i]=wh_output_get_unknown_string(
											connection->data_output))
										{
											/* get miscellaneous information */
											wh_output_get_int(connection->data_output,
												NUM_COMPONENT_DATA,component_data);
											/* get the number of derivatives */
											for (j = 0 ; j < component_data[0] ; j++)
											{
												/* value types */
												/*???DB.  To be done */
												/* SAB.  So we just use the default now,
												 the FE_NODAL_VALUE has been added in by
												 default */
												FE_node_field_creator_define_derivative(
													node_field_creator, i, FE_NODAL_UNKNOWN);
											}
											/* get the number of versions */
											FE_node_field_creator_define_versions(
												node_field_creator, i, component_data[1]);
										}
										else
										{
											return_code=0;
											display_message(ERROR_MESSAGE,
												"CMISS_connection_get_data.  "
												"state: node field info.  "
												"Could not retrieve component name");
										}
									}
									/* create/find the field */
									if (return_code)
									{
										if (field=get_FE_field_manager_matched_field(
											connection->fe_field_manager,field_name,
											GENERAL_FE_FIELD, connection->fe_time,
											/*indexer_field*/(struct FE_field *)NULL,
											/*number_of_indexed_values*/0,cm_field_type,
											&coordinate_system,FE_VALUE_VALUE,
											number_of_components,component_names,
											/*number_of_times*/0,/*time_value_type*/UNKNOWN_VALUE,
											&external))
										{
											if (number_of_field_values)
											{
												for (j=0;j<number_of_field_values;j++)
												{
													set_FE_field_FE_value_value(field,j,field_values[j]);
												}
											}
											/*???DB.  Check for duplicates ? */
											if (define_FE_field_at_node(template_node,field,
												(struct FE_time_version *)NULL,
												node_field_creator))
											{
												template_node_fields[field_num]=ACCESS(FE_field)(field);
											}
											else
											{
												return_code=0;
												display_message(ERROR_MESSAGE,
													"CMISS_connection_get_data.  "
													"state: node field info.  "
													"Could not define node field");
											}
										}
										else
										{
											return_code=0;
											display_message(ERROR_MESSAGE,
												"CMISS_connection_get_data.  %s",
												"state: node field info.  Could not create field");
										}
									}
									for (i=0;i<number_of_components;i++)
									{
										DEALLOCATE(component_names[i]);
									}
								}
								else
								{
									return_code=0;
									display_message(ERROR_MESSAGE,"CMISS_connection_get_data.  "
										"state: node field info.  Could not allocate memory");
								}
								DESTROY(FE_node_field_creator)(&node_field_creator);
								DEALLOCATE(component_names);
								DEALLOCATE(field_values);
								DEALLOCATE(field_name);
							}
							else
							{
								return_code=0;
								display_message(ERROR_MESSAGE,"CMISS_connection_get_data.  %s",
									"state: node field info.  Could not get field name");
							}
						}
						if (return_code)
						{
							/* deaccess the previous */
							if (connection->template_node)
							{
								for (i=0;i<connection->template_node_number_of_fields;i++)
								{
									DEACCESS(FE_field)(connection->template_node_fields+i);
								}
								DEALLOCATE(connection->template_node_fields);
								DEACCESS(FE_node)(&(connection->template_node));
							}
							/* access this one */
							connection->template_node=ACCESS(FE_node)(template_node);
							connection->template_node_number_of_fields=number_of_fields;
							connection->template_node_fields=template_node_fields;
						}
						else
						{
							for (i=0;i<number_of_fields;i++)
							{
								DEACCESS(FE_field)(template_node_fields+i);
							}
							DEALLOCATE(template_node_fields);
						}
						if (return_code)
						{
							/* now look for node information */
							connection->data_input_state=CMISS_DATA_INPUT_STATE_NODE_DATA;
						}
						else
						{
							wh_output_get_remainder(connection->data_output);
							connection->data_input_state=CMISS_DATA_INPUT_STATE_TYPE;
						}
					}
					else
					{
						if (0<number_of_fields)
						{
							return_code=0;
							display_message(ERROR_MESSAGE,"CMISS_connection_get_data.  "
								"state: node field info.  "
								"Could not allocate template_node_fields");
						}
					}
				}
				else
				{
					return_code=0;
					display_message(ERROR_MESSAGE,"CMISS_connection_get_data.  %s",
						"state: node field info.  Could not create template node");
				}
			} break;
			case CMISS_DATA_INPUT_STATE_NODE_DATA:
			{
				if (connection->data_type==CMISS_DATA_CODE)
				{
					FE_node_manager = connection->data_manager;
				}
				else
				{
					FE_node_manager = connection->node_manager;
				}
				/* check for any data messages to be acted upon */
				while(wh_output_can_open(connection->data_output))
				{
					if (wh_output_open_message(connection->data_output,&primary_id,
						&secondary_id))
					{
						return_code=1;
						if (connection->template_node)
						{
							/* say secondary id=node number */
							number_of_values=get_FE_node_number_of_values(connection->
								template_node);
							if (ALLOCATE(temp_field_values,double,number_of_values)&&
								ALLOCATE(field_values,FE_value,number_of_values))
							{
								/* possible conversion issues here */
								if (wh_output_get_double(connection->data_output,
									number_of_values,temp_field_values))
								{
									for (i=0;i<number_of_values;i++)
									{
										field_values[i]=temp_field_values[i];
									}
									/* create the node */
									if (node=CREATE(FE_node)(secondary_id,
										connection->template_node))
									{
										FE_value *start;
										int length, ok;
										struct FE_field **field_address;

										/* have to assign values field by field in the order that
											the fields were specified */
										i=connection->template_node_number_of_fields;
										ok=1;
										field_address=connection->template_node_fields;
										start=field_values;
										while ((i>0)&&ok)
										{
											if (set_FE_nodal_field_FE_value_values(*field_address,
												node,start,&length))
											{
												i--;
												field_address++;
												start += length;
											}
											else
											{
												ok=0;
											}
										}
										if (old_node=FIND_BY_IDENTIFIER_IN_MANAGER(FE_node,
											cm_node_identifier)(secondary_id,FE_node_manager))
										{
/*???debug */
printf("Modifying node %i\n",secondary_id);
											/* modify the existing node */
											if (!MANAGER_MODIFY_NOT_IDENTIFIER(FE_node,
												cm_node_identifier)(old_node,node,FE_node_manager))
											{
												return_code=0;
												display_message(ERROR_MESSAGE,
													"CMISS_connection_get_data.  %s",
													"state: node data.  Could not modify node");
											}
											DESTROY(FE_node)(&node);
										}
										else
										{
/*???debug */
printf("Creating node %i\n",secondary_id);
											/* add it to the manager */
											if (!ADD_OBJECT_TO_MANAGER(FE_node)(node,
												FE_node_manager))
											{
												return_code=0;
												display_message(ERROR_MESSAGE,
													"CMISS_connection_get_data.  %s",
													"state: node data.  Could not add node to manager");
											}
										}
									}
									else
									{
										return_code=0;
										display_message(ERROR_MESSAGE,
											"CMISS_connection_get_data.  %s",
											"state: node data.  Could not create node");
									}
								}
								else
								{
									return_code=0;
									display_message(ERROR_MESSAGE,
										"CMISS_connection_get_data.  %s",
										"state: node data.  Could not get values");
								}
								DEALLOCATE(field_values);
								DEALLOCATE(temp_field_values);
							}
							else
							{
								return_code=0;
								display_message(ERROR_MESSAGE,"CMISS_connection_get_data.  %s",
									"state: node data.  Could not allocate memory");
							}
						}
						else
						{
							return_code=0;
							display_message(ERROR_MESSAGE,"CMISS_connection_get_data.  %s",
								"state: node data.  No current node field info");
						}
						if (return_code)
						{
							if (!wh_output_close_message(connection->data_output))
							{
								display_message(ERROR_MESSAGE,"CMISS_connection_get_data.  %s",
									"state: node data.  Could not close message");
							}
						}
						else
						{
							/* eat up any data not taken */
							wh_output_get_remainder(connection->data_output);
						}
					}
					else
					{
						display_message(ERROR_MESSAGE,"CMISS_connection_get_data.  %s",
							"state: node data.  Could not open message");
					}
				}
				if (wh_output_can_close(connection->data_output))
				{
					/* close the parent message (ie node field info) */
					if (!wh_output_close_message(connection->data_output))
					{
						display_message(ERROR_MESSAGE,"CMISS_connection_get_data.  %s",
							"state: node data.  Could not close node info message");
					}
					connection->data_input_state=
						CMISS_DATA_INPUT_STATE_TYPE;
				}
			} break;
			case CMISS_DATA_INPUT_STATE_NODE_DELETE:
			{
				if (connection->data_type==CMISS_DATA_CODE)
				{
					FE_node_manager = connection->data_manager;
				}
				else
				{
					FE_node_manager = connection->node_manager;
				}
				/* check for any data messages to be acted upon */
				while (wh_output_can_open(connection->data_output))
				{
					if (wh_output_open_message(connection->data_output,&primary_id,
						&secondary_id))
					{
						return_code=0;
						if (old_node=FIND_BY_IDENTIFIER_IN_MANAGER(FE_node,
							cm_node_identifier)(secondary_id,FE_node_manager))
						{
/*???debug */
printf("Deleting node %i\n",secondary_id);
							if (REMOVE_OBJECT_FROM_MANAGER(FE_node)(old_node,
								FE_node_manager))
							{
								return_code=1;
							}
							else
							{
								display_message(ERROR_MESSAGE,
									"CMISS_connection_get_data.  %s",
									"state: node delete.  Could not remove node from manager");
							}
						}
						else
						{
							display_message(ERROR_MESSAGE,
								"CMISS_connection_get_data.  %s",
								"state: node delete.  Could not find node in manager");
						}
						if (return_code)
						{
							if (!wh_output_close_message(connection->data_output))
							{
								display_message(ERROR_MESSAGE,"CMISS_connection_get_data.  %s",
									"state: node delete.  Could not close message");
							}
						}
						else
						{
							/* eat up any data not taken */
							wh_output_get_remainder(connection->data_output);
						}
					}
					else
					{
						display_message(ERROR_MESSAGE,"CMISS_connection_get_data.  %s",
							"state: node delete.  Could not open message");
					}
				}
				if (wh_output_can_close(connection->data_output))
				{
					/* close the parent message (ie node delete) */
					if (!wh_output_close_message(connection->data_output))
					{
						display_message(ERROR_MESSAGE,"CMISS_connection_get_data.  %s",
							"state: node delete.  Could not close node delete message");
					}
					connection->data_input_state=
						CMISS_DATA_INPUT_STATE_TYPE;
				}
			} break;
			case CMISS_DATA_INPUT_STATE_NODE_GROUP:
			{
				/* check for any data messages to be acted upon */
				if (wh_output_can_open(connection->data_output))
				{
					if (wh_output_open_message(connection->data_output,&primary_id,
						&secondary_id))
					{
						/* create new node group */
						sprintf(region_name,"region_%i",secondary_id);
						if (!(connection->new_node_group=CREATE_GROUP(FE_node)(region_name)))
						{
							display_message(ERROR_MESSAGE,"CMISS_connection_get_data.  %s",
								"state: node_group.  Could not create new node group");
						}
						connection->data_input_state=
							CMISS_DATA_INPUT_STATE_NODE_GROUP_DATA;
					}
					else
					{
						display_message(ERROR_MESSAGE,"CMISS_connection_get_data.  %s",
							"state: node_group.  Could not open message");
					}
				}
				else
				{
					if (wh_output_can_close(connection->data_output))
					{
						/* close the parent message (ie node field info) */
						if (!wh_output_close_message(connection->data_output))
						{
							display_message(ERROR_MESSAGE,"CMISS_connection_get_data.  %s",
								"state: node_group.  Could not close node_group message");
						}
						connection->data_input_state=CMISS_DATA_INPUT_STATE_TYPE;
					}
				}
			} break;
			case CMISS_DATA_INPUT_STATE_NODE_GROUP_DATA:
			{
				/* check for any data messages to be acted upon */
				while (wh_output_can_open(connection->data_output))
				{
					if (wh_output_open_message(connection->data_output,&primary_id,
						&secondary_id))
					{
						/* add the node to the new node group */
						if (connection->new_node_group)
						{
							if (node=FIND_BY_IDENTIFIER_IN_MANAGER(FE_node,
								cm_node_identifier)(secondary_id,connection->node_manager))
							{
								if (!ADD_OBJECT_TO_GROUP(FE_node)(node,
									connection->new_node_group))
								{
									display_message(ERROR_MESSAGE,
										"CMISS_connection_get_data.  %s",
										"state: node_group data.  Could not add node to group");
								}
							}
							else
							{
								display_message(ERROR_MESSAGE,"CMISS_connection_get_data.  %s",
									"state: node_group data.  Could not find node");
							}
						}
						else
						{
							display_message(ERROR_MESSAGE,"CMISS_connection_get_data.  %s",
								"state: node_group data.  Invalid new node group");
						}
						if (!wh_output_close_message(connection->data_output))
						{
							display_message(ERROR_MESSAGE,"CMISS_connection_get_data.  %s",
								"state: node_group data.  Could not close message");
						}
					}
					else
					{
						display_message(ERROR_MESSAGE,"CMISS_connection_get_data.  %s",
							"state: node_group data.  Could not open message");
					}
				}
				if (wh_output_can_close(connection->data_output))
				{
					/* modify the global node group */
					if (connection->new_node_group)
					{
            if (GET_NAME(GROUP(FE_node))(connection->new_node_group,
              &group_name))
            {
              if (old_node_group=FIND_BY_IDENTIFIER_IN_MANAGER(GROUP(FE_node),
                name)(group_name,connection->node_group_manager))
              {
                /* modify the global group */
/*???debug */
printf("Modifying node group %s\n",group_name);
                if (!MANAGER_MODIFY_NOT_IDENTIFIER(GROUP(FE_node),
                  name)(old_node_group,connection->new_node_group,
                    connection->node_group_manager))
                {
                  display_message(ERROR_MESSAGE,
                    "CMISS_connection_get_data.  %s",
                    "state: node_group data.  Could not modify node group");
                }
                DESTROY(GROUP(FE_node))(&connection->new_node_group);
              }
              else
              {
                /* create a new global group */
                display_message(INFORMATION_MESSAGE,"Creating node group %s\n",
                  group_name);
                /* add it to the manager */
                if (ADD_OBJECT_TO_MANAGER(GROUP(FE_node))(
                  connection->new_node_group,connection->node_group_manager))
                {
                  connection->new_node_group = (struct GROUP(FE_node) *)NULL;
                }
                else
                {
                  DESTROY(GROUP(FE_node))(&connection->new_node_group);
                  display_message(ERROR_MESSAGE,
                    "CMISS_connection_get_data.  state: node_group data.  "
                    "Could not add node group to manager");
                }
              }
              DEALLOCATE(group_name);
            }
					}
					else
					{
						display_message(ERROR_MESSAGE,"CMISS_connection_get_data.  %s",
							"state: node_group data.  Invalid new node group");
					}
					/* close the parent message (ie node_group) */
					if (!wh_output_close_message(connection->data_output))
					{
						display_message(ERROR_MESSAGE,"CMISS_connection_get_data.  %s",
							"state: node_group data.  Could not close node info message");
					}
					/* go back to looking for a group number */
					connection->data_input_state=
						CMISS_DATA_INPUT_STATE_NODE_GROUP;
				}
			} break;
			case CMISS_DATA_INPUT_STATE_NODE_GROUP_DELETE:
			{
				/* check for any data messages to be acted upon */
				while(wh_output_can_open(connection->data_output))
				{
					if (wh_output_open_message(connection->data_output,&primary_id,
						&secondary_id))
					{
						return_code=0;
						sprintf(region_name,"region_%i",secondary_id);
						if (old_node_group=FIND_BY_IDENTIFIER_IN_MANAGER(GROUP(FE_node),
							name)(region_name,connection->node_group_manager))
						{
/*???debug */
if (GET_NAME(GROUP(FE_node))(old_node_group,&group_name))
{
  printf("Deleting node group %s\n",group_name);
  DEALLOCATE(group_name);
}
							if (REMOVE_OBJECT_FROM_MANAGER(GROUP(FE_node))(old_node_group,
								connection->node_group_manager))
							{
								return_code=1;
							}
							else
							{
								display_message(ERROR_MESSAGE,
									"CMISS_connection_get_data.  %s",
									"state: node_group delete.  Could not remove node group from manager");
							}
						}
						else
						{
							display_message(ERROR_MESSAGE,
								"CMISS_connection_get_data.  %s",
								"state: node_group delete.  Could not find node group in manager");
						}
						if (return_code)
						{
							if (!wh_output_close_message(connection->data_output))
							{
								display_message(ERROR_MESSAGE,"CMISS_connection_get_data.  %s",
									"state: node_group delete.  Could not close message");
							}
						}
						else
						{
							/* eat up any data not taken */
							wh_output_get_remainder(connection->data_output);
						}
					}
					else
					{
						display_message(ERROR_MESSAGE,"CMISS_connection_get_data.  %s",
							"state: node_group delete.  Could not open message");
					}
				}
				if (wh_output_can_close(connection->data_output))
				{
					/* close the parent message (ie node delete) */
					if (!wh_output_close_message(connection->data_output))
					{
						display_message(ERROR_MESSAGE,"CMISS_connection_get_data.  %s",
							"state: node_group delete.  Could not close node delete message");
					}
					connection->data_input_state=
						CMISS_DATA_INPUT_STATE_TYPE;
				}
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,"CMISS_connection_get_data.  %s",
					"Invalid current state");
			} break;
		}
#else /* defined (NOT_TEMPORARY) */
while (wh_output_can_open(connection->data_output))
{
	wh_output_get_remainder(connection->data_output);
}
#endif /* defined (NOT_TEMPORARY) */
	}
	else
	{
		display_message(ERROR_MESSAGE,"CMISS_connection_get_data.  %s",
			"Invalid connection");
	}
	LEAVE;

	return (return_code);
} /* CMISS_connection_get_data */

static int CMISS_connection_count_node_field(struct FE_node *node,
	struct FE_field *field,void *count_void)
/*******************************************************************************
LAST MODIFIED : 23 November 2001

DESCRIPTION :
Increments the <count_void> if the field is a cm field.
???RC external_information is not checked in any way.
==============================================================================*/
{
	int *count,return_code;
	struct FE_field_external_information *external;

	ENTER(CMISS_connection_count_node_field);
	return_code=0;
	if (node&&field&&(count=(int *)count_void))
	{
		return_code=1;
		if (get_FE_field_external_information(field, &external) && external &&
			(compare_FE_field_cm_information == external->compare) &&
			(external->information))
		{
			(*count)++;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"CMISS_connection_count_node_field.  "
			"Invalid arguments");
	}
	LEAVE;

	return (return_code);
} /* CMISS_connection_count_node_field */

struct CMISS_connection_send_node_struct
/*******************************************************************************
LAST MODIFIED : 2 September 2001

DESCRIPTION :
Contains local information for this routine.
==============================================================================*/
{
	int in_message,number_of_cm_fields;
	struct CMISS_connection *connection;
	struct FE_node *template_node;
}; /* struct CMISS_connection_send_node_struct */

static int CMISS_connection_send_node_field(struct FE_node *node,
	struct FE_field *field,void *user_data)
/*******************************************************************************
LAST MODIFIED : 13 September 2001

DESCRIPTION :
Send out node_field information for this node_field.
==============================================================================*/
{
	char *name;
	double *temp_field_values;
	int component_number,i,int_data[4],number_of_components,number_of_values,
		return_code;
	struct CM_field_information *cm_information;
	struct CMISS_connection_send_node_struct *send_node_struct;
	struct FE_field_external_information *external;

	ENTER(CMISS_connection_send_node_field);
	return_code=0;
	if (node&&field&&
		(send_node_struct=(struct CMISS_connection_send_node_struct *)user_data))
	{
		if (get_FE_field_external_information(field,&external)&&external&&
			(compare_FE_field_cm_information==external->compare)&&(cm_information=
			(struct CM_field_information *)(external->information))&&
			(send_node_struct=(struct CMISS_connection_send_node_struct *)user_data))
		{
			return_code=1;
			name=(char *)NULL;
			if (GET_NAME(FE_field)((struct FE_field *)field,&name))
			{
				wh_input_add_char(send_node_struct->connection->data_input,strlen(name),
					name);
				DEALLOCATE(name);
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"CMISS_connection_send_node_field.  Could not get field name");
			}
			int_data[0]=get_FE_field_CM_field_type(field);
			int_data[1]=get_coordinate_system_type(
				get_FE_field_coordinate_system(field));
			number_of_values=get_FE_field_number_of_values(field);
			int_data[2]=number_of_values;
			number_of_components=get_FE_field_number_of_components(
				(struct FE_field *)field);
			int_data[3]=number_of_components;
			wh_input_add_int(send_node_struct->connection->data_input,4,int_data);
			if (0<number_of_values)
			{
				if (ALLOCATE(temp_field_values,double,number_of_values))
				{
					for (i=0;i<number_of_values;i++)
					{
						get_FE_field_FE_value_value((struct FE_field *)field,i,
							(FE_value *)(temp_field_values+i));
					}
					wh_input_add_double(send_node_struct->connection->data_input,
						number_of_values,temp_field_values);
					DEALLOCATE(temp_field_values);
				}
				else
				{
					display_message(ERROR_MESSAGE,"CMISS_connection_send_node_field.  %s",
						"Could not allocate memory");
				}
			}
			/* send cm field information */
			int_data[0]=cm_information->nr;
			switch (cm_information->array)
			{
				case CM_ARRAY_FIX:
				{
					int_data[1]=3;
				} break;
				case CM_ARRAY_LD:
				{
					int_data[1]=3;
				} break;
				case CM_ARRAY_WD:
				{
					int_data[1]=2;
				} break;
				case CM_ARRAY_XID:
				{
					int_data[1]=3;
				} break;
				case CM_ARRAY_XP:
				{
					int_data[1]=1;
				} break;
				case CM_ARRAY_YP:
				{
					int_data[1]=2;
				} break;
				case CM_ARRAY_ZD:
				{
					int_data[1]=1;
				} break;
				default:
				{
					int_data[1]=0;
				} break;
			}
			wh_input_add_int(send_node_struct->connection->data_input,2,int_data);
			switch (cm_information->array)
			{
				case CM_ARRAY_FIX:
				{
					int_data[0]=(cm_information->indices).yp.nx;
					int_data[1]=(cm_information->indices).yp.nhx_index;
					int_data[2]=(cm_information->indices).yp.nc;
					wh_input_add_int(send_node_struct->connection->data_input,3,int_data);
				} break;
				case CM_ARRAY_XP:
				{
					int_data[0]=(cm_information->indices).xp.nj_typ;
					wh_input_add_int(send_node_struct->connection->data_input,1,int_data);
				} break;
				case CM_ARRAY_YP:
				{
					int_data[0]=(cm_information->indices).yp.nx;
					int_data[1]=(cm_information->indices).yp.nhx_index;
					int_data[2]=(cm_information->indices).yp.nc;
					int_data[3]=(cm_information->indices).yp.iy;
					wh_input_add_int(send_node_struct->connection->data_input,4,int_data);
				} break;
			}
			for (component_number=0;component_number<number_of_components;
				component_number++)
			{
				name=(char *)NULL;
				if (name=get_FE_field_component_name(field,component_number))
				{
					wh_input_add_char(send_node_struct->connection->data_input,
						strlen(name),name);
					DEALLOCATE(name);
				}
				else
				{
					display_message(ERROR_MESSAGE,"CMISS_connection_send_node_field.  "
						"Could not get field component name");
				}
				int_data[0]=get_FE_node_field_component_number_of_derivatives(node,
					field,component_number);
				int_data[1]=get_FE_node_field_component_number_of_versions(node,field,
					component_number);
				/*???DB.  Nodal value types ? */
				wh_input_add_int(send_node_struct->connection->data_input,2,int_data);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"CMISS_connection_send_node_field.  "
			"Invalid arguments");
	}
	LEAVE;

	return (return_code);
} /* CMISS_connection_send_node_field */

struct CMISS_connection_send_nodal_values_data
/*******************************************************************************
LAST MODIFIED : 2 November 1998

DESCRIPTION :
Contains local information for this routine.
==============================================================================*/
{
	double *nodal_values;
	int number_of_nodal_values;
}; /* struct CMISS_connection_send_nodal_values_data */

static int CMISS_connection_send_nodal_values(struct FE_node *node,
	struct FE_field *field,void *user_data)
/*******************************************************************************
LAST MODIFIED : 21 September 1999

DESCRIPTION :
Send out nodal values for this field.
==============================================================================*/
{
	double *temp_values;
	enum Value_type value_type;
	int i,number_of_nodal_values,return_code;
	struct CMISS_connection_send_nodal_values_data *send_nodal_values;

	ENTER(CMISS_connection_send_nodal_values);
	return_code=0;
	if (node&&field&&(send_nodal_values=
		(struct CMISS_connection_send_nodal_values_data *)user_data))
	{
		value_type=get_FE_field_value_type(field);
		if (FE_VALUE_VALUE==value_type)
		{
			FE_value *nodal_values;

			if (get_FE_nodal_field_FE_value_values(field,node,&number_of_nodal_values,
				&nodal_values))
			{
				if (REALLOCATE(temp_values,send_nodal_values->nodal_values,double,
					number_of_nodal_values+(send_nodal_values->number_of_nodal_values)))
				{
					send_nodal_values->nodal_values=temp_values;
					temp_values += send_nodal_values->number_of_nodal_values;
					send_nodal_values->number_of_nodal_values += number_of_nodal_values;
					for (i=0;i<number_of_nodal_values;i++)
					{
						temp_values[i]=(double)nodal_values[i];
					}
					return_code=1;
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"CMISS_connection_send_nodal_values.  Not enough memory");
				}
				DEALLOCATE(nodal_values);
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"CMISS_connection_send_nodal_values.  Could not get values");
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,"CMISS_connection_send_nodal_values.  "
				"Type %s not supported",Value_type_string(value_type));
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CMISS_connection_send_nodal_values.  Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
} /* CMISS_connection_send_nodal_values */

static int CMISS_connection_send_node(struct FE_node *node,void *user_data)
/*******************************************************************************
LAST MODIFIED : 2 September 2001

DESCRIPTION :
Something has changed globally about the objects this widget uses, so refresh.
==============================================================================*/
{
	int number_of_fields,return_code;
	struct CMISS_connection_send_nodal_values_data send_nodal_values_data;
	struct CMISS_connection_send_node_struct *send_node_struct;

	ENTER(CMISS_connection_send_node);
	return_code=0;
	if (node&&
		(send_node_struct=(struct CMISS_connection_send_node_struct *)user_data))
	{
		if (!equivalent_FE_fields_at_nodes(node,send_node_struct->template_node))
		{
			if (send_node_struct->in_message)
			{
				wh_input_close_message(send_node_struct->connection->data_input);
				send_node_struct->in_message=0;
			}
			send_node_struct->template_node=node;
			send_node_struct->number_of_cm_fields=0;
			for_each_FE_field_at_node(CMISS_connection_count_node_field,
				(void *)&(send_node_struct->number_of_cm_fields),
				send_node_struct->template_node);
			if (0<send_node_struct->number_of_cm_fields)
			{
				send_node_struct->in_message=1;
				wh_input_open_message(send_node_struct->connection->data_input,
					send_node_struct->connection->data_type,CMISS_CHANGE);
				/* send the field information */
				number_of_fields=get_FE_node_number_of_fields(node);
				wh_input_add_int(send_node_struct->connection->data_input,1,
					&number_of_fields);
				for_each_FE_field_at_node(CMISS_connection_send_node_field,user_data,
					send_node_struct->template_node);
			}
		}
		if (0<send_node_struct->number_of_cm_fields)
		{
			/* send the node data */
			wh_input_open_message(send_node_struct->connection->data_input,0,
				get_FE_node_cm_node_identifier(node));
			send_nodal_values_data.nodal_values=(double *)NULL;
			send_nodal_values_data.number_of_nodal_values=0;
			if (for_each_FE_field_at_node(CMISS_connection_send_nodal_values,
				&send_nodal_values_data,node))
			{
				/* possible conversion issues here */
				wh_input_add_double(send_node_struct->connection->data_input,
					send_nodal_values_data.number_of_nodal_values,
					send_nodal_values_data.nodal_values);
				return_code=1;
			}
			else
			{
				display_message(ERROR_MESSAGE,"CMISS_connection_send_node.  %s",
					"Could not allocate memory");
			}
			DEALLOCATE(send_nodal_values_data.nodal_values);
			wh_input_close_message(send_node_struct->connection->data_input);
		}
		else
		{
			return_code=1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"CMISS_connection_send_node.  %s",
			"Invalid arguments");
	}
	LEAVE;

	return (return_code);
} /* CMISS_connection_send_node */

static int CMISS_connection_send_delete_node(struct FE_node *node,
	void *connection_void)
/*******************************************************************************
LAST MODIFIED : 30 May 2001

DESCRIPTION :
Sends a destroy message for <node> through the wormhole.
==============================================================================*/
{
	int return_code;
	struct CMISS_connection *connection;

	ENTER(CMISS_connection_send_delete_node);
	if (node && (connection = (struct CMISS_connection *)connection_void))
	{
		wh_input_open_message(connection->data_input,
			connection->data_type, CMISS_DELETE);
		wh_input_open_message(connection->data_input, 0,
			get_FE_node_cm_node_identifier(node));
		wh_input_close_message(connection->data_input);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CMISS_connection_send_delete_node.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* CMISS_connection_send_delete_node */

static void CMISS_connection_node_global_change(
	struct MANAGER_MESSAGE(FE_node) *message, void *connection_void)
/*******************************************************************************
LAST MODIFIED : 30 May 2001

DESCRIPTION :
Something has changed globally about the objects this widget uses, so refresh.
==============================================================================*/
{
	struct CMISS_connection *connection;
	struct CMISS_connection_send_node_struct send_node_struct;
	
	ENTER(CMISS_connection_node_global_change);
	if (message && (connection = (struct CMISS_connection *)connection_void))
	{
		connection->data_type = CMISS_NODE_CODE;
		switch (message->change)
		{
			case MANAGER_CHANGE_ADD(FE_node):
			case MANAGER_CHANGE_IDENTIFIER(FE_node):
			case MANAGER_CHANGE_OBJECT(FE_node):
			case MANAGER_CHANGE_OBJECT_NOT_IDENTIFIER(FE_node):
			{
				send_node_struct.in_message = 0;
				send_node_struct.connection = connection;
				send_node_struct.template_node = (struct FE_node *)NULL;
				if (!FOR_EACH_OBJECT_IN_LIST(FE_node)(CMISS_connection_send_node,
					&send_node_struct, message->changed_object_list))
				{
					display_message(ERROR_MESSAGE,
						"CMISS_connection_node_global_change.  "
						"Could not send modified or added nodes");
				}
				if (send_node_struct.in_message)
				{
					wh_input_close_message(connection->data_input);
				}
			} break;
			case MANAGER_CHANGE_REMOVE(FE_node):
			{
				/* ???RC Without completely knowing how this works, I'm saving the old
					 code for comparison with the new list iterator code. Certainly gives
					 the same result when only one node removed. */
#if defined (OLD_CODE)
				wh_input_open_message(connection->data_input,
					CMISS_NODE_CODE,CMISS_DELETE);
				wh_input_open_message(connection->data_input,0,
					get_FE_node_cm_node_identifier(message->object_changed));
				wh_input_close_message(connection->data_input);
#endif /* defined (OLD_CODE) */
				/* Note we had to set connection->data_type to CMISS_NODE_CODE, as
					 above to get the following to work */
				if (!FOR_EACH_OBJECT_IN_LIST(FE_node)(CMISS_connection_send_delete_node,
					(void *)&connection, message->changed_object_list))
				{
					display_message(ERROR_MESSAGE,
						"CMISS_connection_node_global_change.  "
						"Could not send deleted nodes");
				}
				/*???RC old code always ended in 2 of the following lines: */
				wh_input_close_message(connection->data_input);
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"CMISS_connection_node_global_change.  Invalid manager message");
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CMISS_connection_node_global_change.  Invalid argument(s)");
	}
	LEAVE;
} /* CMISS_connection_node_global_change */

static void CMISS_connection_data_global_change(
	struct MANAGER_MESSAGE(FE_node) *message, void *connection_void)
/*******************************************************************************
LAST MODIFIED : 30 May 2001

DESCRIPTION :
Something has changed globally about the objects this widget uses, so refresh.
==============================================================================*/
{
	struct CMISS_connection *connection;
	struct CMISS_connection_send_node_struct send_node_struct;

	ENTER(CMISS_connection_data_global_change);
	if (message && (connection = (struct CMISS_connection *)connection_void))
	{
		connection->data_type = CMISS_DATA_CODE;
		switch (message->change)
		{
			case MANAGER_CHANGE_ADD(FE_node):
			case MANAGER_CHANGE_IDENTIFIER(FE_node):
			case MANAGER_CHANGE_OBJECT(FE_node):
			case MANAGER_CHANGE_OBJECT_NOT_IDENTIFIER(FE_node):
			{
				send_node_struct.in_message = 0;
				send_node_struct.connection = connection;
				send_node_struct.template_node = (struct FE_node *)NULL;
				if (!FOR_EACH_OBJECT_IN_LIST(FE_node)(CMISS_connection_send_node,
					&send_node_struct, message->changed_object_list))
				{
					display_message(ERROR_MESSAGE,
						"CMISS_connection_data_global_change.  "
						"Could not send modified or added data");
				}
				if (send_node_struct.in_message)
				{
					wh_input_close_message(connection->data_input);
				}
			} break;
			case MANAGER_CHANGE_REMOVE(FE_node):
			{
				/* ???RC Without completely knowing how this works, I'm saving the old
					 code for comparison with the new list iterator code. Certainly gives
					 the same result when only one node removed. */
#if defined (OLD_CODE)
				wh_input_open_message(connection->data_input,CMISS_DATA_CODE,
					CMISS_DELETE);
				wh_input_open_message(connection->data_input,0,
					get_FE_node_cm_node_identifier(message->object_changed));
				wh_input_close_message(connection->data_input);
#endif /* defined (OLD_CODE) */
				/* Note we had to set connection->data_type to CMISS_DATA_CODE, as
					 above to get the following to work */
				if (!FOR_EACH_OBJECT_IN_LIST(FE_node)(CMISS_connection_send_delete_node,
					(void *)&connection, message->changed_object_list))
				{
					display_message(ERROR_MESSAGE,
						"CMISS_connection_data_global_change.  "
						"Could not send deleted data");
				}
				/*???RC old code always ended in 2 of the following lines: */
				wh_input_close_message(connection->data_input);
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"CMISS_connection_data_global_change.  Invalid manager message");
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CMISS_connection_data_global_change.  Invalid argument(s)");
	}
	LEAVE;
} /* CMISS_connection_data_global_change */

#if defined (NOT_DEBUG)
#if defined (NOT_TEMPORARY)

struct CMISS_connection_send_element_struct
/*******************************************************************************
LAST MODIFIED : 24 February 1997

DESCRIPTION :
Contains local information for this routine.
==============================================================================*/
{
	int in_message;
	struct FE_element_field_info *element_field_info;
	struct FE_element_shape *element_shape;
	struct CMISS_connection *connection;
}; /* struct CMISS_connection_send_element_struct */

static int CMISS_connection_send_element_field(
	struct FE_element_field *element_field,void *user_data)
/*******************************************************************************
LAST MODIFIED : 18 November 1998

DESCRIPTION :
Send out element_field information for this element_field.
==============================================================================*/
{
	char *name;
	double temp_field_value,*temp_field_values;
	int component_number,i,int_data[4],j,k,*node_indices,number_of_components,
		number_of_nodes,number_of_scale_factors,number_of_values,return_code,
		*scale_factor_indices;
	struct CMISS_connection_send_element_struct *send_element_struct;
	struct FE_field_component field_component;

	ENTER(CMISS_connection_send_element_field);
	return_code=0;
	if (element_field&&(send_element_struct=
		(struct CMISS_connection_send_element_struct *)user_data))
	{
		name=(char *)NULL;
		if (GET_NAME(FE_field)(element_field->field,&name))
		{
			wh_input_add_char(send_element_struct->connection->data_input,
				strlen(name),name);
			DEALLOCATE(name);
		}
		else
		{
			display_message(ERROR_MESSAGE,"CMISS_connection_send_element_field.  %s",
				"Could not get field name");
		}
		int_data[0]=get_FE_field_CM_field_type(element_field->field);
		int_data[1]=get_coordinate_system_type(get_FE_field_coordinate_system(
			element_field->field));
		number_of_values=get_FE_field_number_of_values(element_field->field);
		int_data[2]=number_of_values;
		number_of_components=
			get_FE_field_number_of_components(element_field->field);
		int_data[3]=number_of_components;
		wh_input_add_int(send_element_struct->connection->data_input,4,int_data);
		if (number_of_values)
		{
			if (ALLOCATE(temp_field_values,double,number_of_values))
			{
				for (i=0;i<number_of_values;i++)
				{
					get_FE_field_FE_value_value(element_field->field,i,
						(FE_value *)(temp_field_values+i));
				}
				wh_input_add_double(send_element_struct->connection->data_input,
					number_of_values,temp_field_values);
				DEALLOCATE(temp_field_values);
			}
			else
			{
				for (i=0;i<number_of_values;i++)
				{
					get_FE_field_FE_value_value(element_field->field,i,
						(FE_value *)&temp_field_value);
					wh_input_add_double(send_element_struct->connection->data_input,
						1,&temp_field_value);
				}
				display_message(ERROR_MESSAGE,
					"CMISS_connection_send_element_field.  %s",
					"Could not allocate memory");
			}
		}
		field_component.field=element_field->field;
		for (component_number=0;component_number<number_of_components;
			component_number++)
		{
			field_component.number=component_number;
			name=(char *)NULL;
			if (GET_NAME(FE_field_component)(&field_component,&name))
			{
				wh_input_add_char(send_element_struct->connection->data_input,
					strlen(name),name);
				DEALLOCATE(name);
			}
			else
			{
				display_message(ERROR_MESSAGE,
					"CMISS_connection_send_element_field.  %s",
					"Could not get field component name");
			}
			/* send the basis type */
			wh_input_add_int(send_element_struct->connection->data_input,
				((send_element_struct->element_shape->dimension)*
				(1+send_element_struct->element_shape->dimension))/2,
				(element_field->components)[component_number]->basis->type);
			switch ((element_field->components)[component_number]->type)
			{
				case STANDARD_NODE_TO_ELEMENT_MAP:
				{
					number_of_nodes=(element_field->components)[component_number]->map.
						standard_node_based.number_of_nodes;
					number_of_scale_factors=0;
					for (i=0;i<number_of_nodes;i++)
					{
						number_of_scale_factors += (((element_field->components)[
							component_number]->map).standard_node_based.
							node_to_element_maps)[i]->number_of_nodal_values;
					}
					/* send the node indices */
					if (ALLOCATE(node_indices,int,number_of_nodes))
					{
						for (i=0;i<number_of_nodes;i++)
						{
							node_indices[i]=(((element_field->components)[component_number]->
								map).standard_node_based.node_to_element_maps)[i]->node_index;
						}
						wh_input_add_int(send_element_struct->connection->data_input,
							number_of_nodes,node_indices);
						DEALLOCATE(node_indices);
					}
					else
					{
						for (i=0;i<number_of_nodes;i++)
						{
							wh_input_add_int(send_element_struct->connection->data_input,1,
								&((((element_field->components)[component_number]->
								map).standard_node_based.node_to_element_maps)[i]->node_index));
						}
						display_message(ERROR_MESSAGE,
							"CMISS_connection_send_element_field.  %s",
							"Could not allocate memory");
					}
					/* send the scale factor indices */
					if (ALLOCATE(scale_factor_indices,int,number_of_scale_factors))
					{
						k=0;
						for (i=0;i<number_of_nodes;i++)
						{
							for (j=0;j<(((element_field->components)[component_number]->map).
								standard_node_based.node_to_element_maps)[i]->
								number_of_nodal_values;j++)
							{
								scale_factor_indices[k]=((((element_field->components)[
									component_number]->map).standard_node_based.
									node_to_element_maps)[i]->scale_factor_indices)[j];
								k++;
							}
						}
						wh_input_add_int(send_element_struct->connection->data_input,
							number_of_scale_factors,scale_factor_indices);
						DEALLOCATE(scale_factor_indices);
					}
					else
					{
						k=0;
						for (i=0;i<number_of_nodes;i++)
						{
							for (j=0;j<((element_field->components)[component_number]->map.
								standard_node_based.node_to_element_maps)[i]->
								number_of_nodal_values;j++)
							{
								wh_input_add_int(send_element_struct->connection->
									data_input,1,&(((((element_field->components)[
									component_number]->map).standard_node_based.
									node_to_element_maps)[i]->scale_factor_indices)[j]));
								k++;
							}
						}
						display_message(ERROR_MESSAGE,
							"CMISS_connection_send_element_field.  %s",
							"Could not allocate memory");
					}
				} break;
				default:
				{
					display_message(ERROR_MESSAGE,
						"CMISS_connection_send_element_field.  %s",
						"Unknown global to element map");
					return_code=0;
				} break;
			}
		}
		return_code=1;
	}
	else
	{
		display_message(ERROR_MESSAGE,"CMISS_connection_send_element_field.  %s",
			"Invalid arguments");
	}
	LEAVE;

	return (return_code);
} /* CMISS_connection_send_element_field */

static int CMISS_connection_send_element(struct FE_element *element,
	void *user_data)
/*******************************************************************************
LAST MODIFIED : 15 February 1999

DESCRIPTION :
Something has changed globally about the objects this widget uses, so refresh.
==============================================================================*/
{
	double scale_factor,*scale_factors;
	int i,node_number,*node_numbers,number_of_fields,number_of_nodes,
		number_of_scale_factors,return_code;
	struct CMISS_connection_send_element_struct *send_element_struct;

	ENTER(CMISS_connection_send_element);
	return_code=0;
	if (element&&(send_element_struct=
		(struct CMISS_connection_send_element_struct *)user_data))
	{
		/* only deal with elements (not faces or lines) */
		if ( (0<(element->cm).number)&& ((element->cm).type==CM_ELEMENT) )
		{
			if ((element->shape!=send_element_struct->element_shape)||
				(element->information->fields!=send_element_struct->element_field_info))
				/*???DB.  Could split in two, as in read_FE_element_group, but this
					would require another message type */
			{
				if (send_element_struct->in_message)
				{
					wh_input_close_message(send_element_struct->connection->data_input);
				}
				send_element_struct->in_message=1;
				send_element_struct->element_shape=element->shape;
				send_element_struct->element_field_info=element->information->fields;
				wh_input_open_message(send_element_struct->connection->data_input,
					send_element_struct->connection->data_type,CMISS_CHANGE);
				/* send the shape */
				wh_input_add_int(send_element_struct->connection->data_input,1,
					&(element->shape->dimension));
					/*???DB.  Need more for shape description */
				/* send the number of nodes */
				wh_input_add_int(send_element_struct->connection->data_input,1,
					&(element->information->number_of_nodes));
				/* send the number of scale factors */
				wh_input_add_int(send_element_struct->connection->data_input,1,
					&(element->information->number_of_scale_factors));
				/* send the field information */
				number_of_fields=			
					NUMBER_IN_LIST(FE_element_field)(send_element_struct->
						element_field_info->element_field_list);
				wh_input_add_int(send_element_struct->connection->data_input,1,
					&number_of_fields);			
				FOR_EACH_OBJECT_IN_LIST(FE_element_field)(
					CMISS_connection_send_element_field,user_data,send_element_struct->
					element_field_info->element_field_list);
			}
			/* send the element data */
			wh_input_open_message(send_element_struct->connection->data_input,0,
				(element->cm).number);
			number_of_nodes=element->information->number_of_nodes;
			if (ALLOCATE(node_numbers,int,number_of_nodes))
			{
				for (i=0;i<number_of_nodes;i++)
				{
					node_numbers[i]=
						get_FE_node_cm_node_identifier((element->information->nodes)[i]);
				}
				wh_input_add_int(send_element_struct->connection->data_input,
					number_of_nodes,node_numbers);
				DEALLOCATE(node_numbers);
			}
			else
			{
				display_message(ERROR_MESSAGE,"CMISS_connection_send_element.  %s",
					"Could not allocate memory");
				for (i=0;i<number_of_nodes;i++)
				{
					node_number=
						get_FE_node_cm_node_identifier((element->information->nodes)[i]);
					wh_input_add_int(send_element_struct->connection->data_input,1,
						&node_number);
				}
			}
			number_of_scale_factors=element->information->number_of_scale_factors;
			if (ALLOCATE(scale_factors,double,number_of_scale_factors))
			{
				/* possible conversion issues here */
				for (i=0;i<number_of_scale_factors;i++)
				{
					scale_factors[i]=(element->information->scale_factors)[i];
				}
				wh_input_add_double(send_element_struct->connection->data_input,
					number_of_scale_factors,scale_factors);
				DEALLOCATE(scale_factors);
			}
			else
			{
				display_message(ERROR_MESSAGE,"CMISS_connection_send_element.  %s",
					"Could not allocate memory");
				for (i=0;i<number_of_scale_factors;i++)
				{
					scale_factor=(element->information->scale_factors)[i];
					wh_input_add_double(send_element_struct->connection->data_input,1,
						&scale_factor);
				}
			}
			wh_input_close_message(send_element_struct->connection->data_input);
			return_code=1;
		}
		else
		{
			return_code=1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"CMISS_connection_send_element.  %s",
			"Invalid arguments");
	}
	LEAVE;

	return (return_code);
} /* CMISS_connection_send_element */

static int CMISS_connection_send_delete_element(struct FE_element *element,
	void *connection_void)
/*******************************************************************************
LAST MODIFIED : 30 May 2001

DESCRIPTION :
If <element> is top-level, sends a destroy message for it through the wormhole.
==============================================================================*/
{
	int return_code;
	struct CMISS_connection *connection;

	ENTER(CMISS_connection_send_delete_element);
	if (element && (connection = (struct CMISS_connection *)connection_void))
	{
		/* only deal with top-level elements (not faces or lines) */
		if ((element->cm.type == CM_ELEMENT) && (0 < element->cm.number))
		{
			wh_input_open_message(connection->data_input, CMISS_ELEMENT_CODE,
				CMISS_DELETE);
			wh_input_open_message(connection->data_input, 0, element->cm.number);
			wh_input_close_message(connection->data_input);
		}
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CMISS_connection_send_delete_element.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
} /* CMISS_connection_send_delete_element */

static void CMISS_connection_element_global_change(
	struct MANAGER_MESSAGE(FE_element) *message, void *connection_void)
/*******************************************************************************
LAST MODIFIED : 30 May 2001

DESCRIPTION :
Something has changed globally about the objects this widget uses, so refresh.
==============================================================================*/
{
	struct CMISS_connection *connection;
	struct CMISS_connection_send_element_struct send_element_struct;

	ENTER(CMISS_connection_element_global_change);
	if (message && (connection = (struct CMISS_connection *)connection_void))
	{
		connection->data_type = CMISS_ELEMENT_CODE;
		switch (message->change)
		{
			case MANAGER_CHANGE_ADD(FE_element):
			case MANAGER_CHANGE_IDENTIFIER(FE_element):
			case MANAGER_CHANGE_OBJECT(FE_element):
			case MANAGER_CHANGE_OBJECT_NOT_IDENTIFIER(FE_element):
			{
				send_element_struct.in_message = 0;
				send_element_struct.connection = connection;
				send_element_struct.element_field_info =
					(struct FE_element_field_info *)NULL;
				if (!FOR_EACH_OBJECT_IN_LIST(FE_element)(
					CMISS_connection_send_element, (void *)&send_element_struct,
					message->changed_object_list))
				{
					display_message(ERROR_MESSAGE,
						"CMISS_connection_element_global_change.  "
						"Could not send modified or added elements");
				}
				if (send_element_struct.in_message)
				{
					wh_input_close_message(connection->data_input);
				}
			} break;
			case MANAGER_CHANGE_REMOVE(FE_element):
			{
				/* ???RC Without completely knowing how this works, I'm saving the old
					 code for comparison with the new list iterator code. Certainly gives
					 the same result when only one element removed. */
#if defined (OLD_CODE)
				/* only deal with elements (not faces or lines) */
				if ( (0<(message->object_changed->cm).number)&&
					((message->object_changed->cm).type==CM_ELEMENT) )
				{
					wh_input_open_message(connection->data_input,CMISS_ELEMENT_CODE,
						CMISS_DELETE);
					wh_input_open_message(connection->data_input,0,
						(message->object_changed->cm).number);
					wh_input_close_message(connection->data_input);
					wh_input_close_message(connection->data_input);
				}
#endif /* defined (OLD_CODE) */
				/* only deal with top-level elements (not faces or lines) */
				if (FIRST_OBJECT_IN_LIST_THAT(FE_element)(
					FE_element_is_top_level, (void *)NULL, message->changed_object_list))
				{
					if (!FOR_EACH_OBJECT_IN_LIST(FE_element)(
						CMISS_connection_send_delete_element, (void *)&connection,
						message->changed_object_list))
					{
						display_message(ERROR_MESSAGE,
							"CMISS_connection_element_global_change.  "
							"Could not send deleted elements");
					}
					/*???RC old code always ended in 2 of the following lines: */
					wh_input_close_message(connection->data_input);
				}
			} break;
			default:
			{
				display_message(ERROR_MESSAGE,
					"CMISS_connection_element_global_change.  %s",
					"Invalid manager message");
			} break;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"CMISS_connection_element_global_change.  %s",
			"Invalid arguments");
	}
	LEAVE;
} /* CMISS_connection_element_global_change */

#endif /* defined (NOT_TEMPORARY) */
#endif /* defined (NOT_DEBUG) */

/*
Global functions
----------------
*/

struct CMISS_connection *CREATE(CMISS_connection)(char *machine,
	enum Machine_type type,int attach,double wormhole_timeout,char mycm_flag,
	char asynchronous_commands,struct MANAGER(FE_element) *element_manager,
	struct MANAGER(GROUP(FE_element)) *element_group_manager,
	struct MANAGER(FE_field) *fe_field_manager, struct FE_time *fe_time,
	struct MANAGER(FE_node) *node_manager,struct MANAGER(FE_node) *data_manager,
	struct MANAGER(GROUP(FE_node)) *node_group_manager,
	struct MANAGER(GROUP(FE_node)) *data_group_manager,
	struct Prompt_window **prompt_window_address,char *parameters_file_name,
	char *examples_directory_path,struct User_interface *user_interface)
/*******************************************************************************
LAST MODIFIED : 3 October 2001

DESCRIPTION :
Creates a connection to the machine specified in <machine>.  If <attach> is 
not zero then cm already exists and <attach> is the base port number to connect
on, otherwise a new cm is spawned.  If <asynchronous_commands> is set then cmgui
does not wait for cm commands to complete, otherwise it does.
==============================================================================*/
{
	struct CMISS_connection *return_struct;
	int i,j,base_number,count;
	char connection_info[3][2][100],cmiss_parameter[300],temp[100];
		/*???GMH.  Naughty */
	char *cm_executable,*cwd,*mycm_executable;
#if defined (VMS)
	struct dsc$descriptor_s command_descriptor;
	unsigned long flags;
#endif /* defined (VMS) */
#if defined (MOTIF)
#define XmNcmExecutable "cmExecutable"
#define XmCCmExecutable "CmExecutable"
#define XmNmycmExecutable "mycmExecutable"
#define XmCMycmExecutable "MycmExecutable"
	static XtResource
		cm_resource[]=
		{{
			XmNcmExecutable,
			XmCCmExecutable,
			XmRString,
			sizeof(char *),
			0,
			XmRString,
			"cm"
		}},
		mycm_resource[]=
		{{
			XmNmycmExecutable,
			XmCMycmExecutable,
			XmRString,
			sizeof(char *),
			0,
			XmRString,
			"mycm"
		}};
#endif /* defined (MOTIF) */

	ENTER(CREATE(CMISS_connection));
	return_struct=(struct CMISS_connection *)NULL;
#if defined (NOT_DEBUG)
#endif /* defined (NOT_DEBUG) */
	if (machine&&element_manager&&element_group_manager&&fe_field_manager&&
		node_manager&&data_manager&&node_group_manager&&data_group_manager&&
		user_interface)
	{
		if (ALLOCATE(return_struct,struct CMISS_connection,1))
		{
			return_struct->asynchronous_commands=asynchronous_commands;
			return_struct->command_in_progress=0;
			return_struct->cm_quit=0;
			if (ALLOCATE(return_struct->name,char,strlen(machine)+1))
			{
				strcpy(return_struct->name,machine);
				if (attach)
				{
					/* this tells us how to communicate */
					base_number=attach;
				}
				else
				{
					/* get a random number for the connection */
					base_number = rand() & 0xFF; /* a number 0-255 */
					base_number *= 4; /* start every fourth number */
					/* add a couple of thousand... (low number ports are privileged) */
					base_number += 2000;
				}
#if defined (USE_WORMHOLE_PIPES)
				if ((user_interface->local_machine_info->type==MACHINE_UNIX)&&
					!strcmp(machine,user_interface->local_machine_info->name))
				{
					/* can use pipes */
					/* local process, so connect via /tmp pipe */
					for (i=0;i<3;i++)
					{
						for (j=0;j<2;j++)
						{
							strcpy(connection_info[i][j],"file:/tmp/cmiss_link");
						}
					}
					/* parameter is a file name, so should be the same */
					strcpy(cmiss_parameter,connection_info[0][0]);
				}
				else
#endif /* USE_WORMHOLE_PIPES */
				{
					/* remote process - use sockets */
					for (i=0;i<3;i++)
					{
						for (j=0;j<2;j++)
						{
							sprintf(connection_info[i][j],"sock:%s:",machine);
						}
					}
					sprintf(cmiss_parameter,"sock:%s:",
						user_interface->local_machine_info->name);
				}
				/* we need a different name for each wormhole */
				count=0;
				for (i=0;i<3;i++)
				{
					for (j=0;j<2;j++)
					{
						/* get a unique number */
						sprintf(temp,"%i",base_number+count);
						strcat(connection_info[i][j],temp);
						count++;
					}
				}
				sprintf(temp,"%i",base_number);
				strcat(cmiss_parameter,temp);
				if (parameters_file_name)
				{
					strcat(cmiss_parameter," -parameters ");
					strcat(cmiss_parameter,parameters_file_name);
				}
				if (examples_directory_path)
				{
					strcat(cmiss_parameter," -epath ");
					strcat(cmiss_parameter,examples_directory_path);
				}
				if (!attach)
				{
#if defined (MOTIF)
					XtVaGetApplicationResources(user_interface->application_shell,
						&cm_executable,cm_resource,XtNumber(cm_resource),NULL);
					XtVaGetApplicationResources(user_interface->application_shell,
						&mycm_executable,mycm_resource,XtNumber(mycm_resource),NULL);
#else
					cm_executable="cm";
					mycm_executable="mycm";
#endif /* defined (MOTIF) */
#if defined (UNIX)
					if (!strcmp(machine,user_interface->local_machine_info->name))
					{
						if (mycm_flag)
						{
							sprintf(global_temp_string,"%s -cmgui %s &",mycm_executable,
								cmiss_parameter);
#if defined (OLD_CODE)
							/* work out the command using environment variables */
							if (getenv("CMISS_MYCMISSPATH"))
							{
								sprintf(global_temp_string,"%s/bin/mycm -cmgui %s &",
									getenv("CMISS_MYCMISSPATH"),cmiss_parameter);
							}
							else
							{
								display_message(WARNING_MESSAGE,
									"CREATE(CMISS_connection).  %s",
									"Could not get CMISS_MYCMISSPATH environment variable");
								sprintf(global_temp_string,"mycm -cmgui %s &",
									cmiss_parameter);
							}
#endif /* defined (OLD_CODE) */
						}
						else
						{
							sprintf(global_temp_string,"%s -cmgui %s &",cm_executable,
								cmiss_parameter);
#if defined (OLD_CODE)
							/* work out the command using environment variables */
							if (getenv("CMISS_ROOT"))
							{
								sprintf(global_temp_string,"%s/cm/bin/cm_n32 -cmgui %s &",
									getenv("CMISS_ROOT"),cmiss_parameter);
							}
							else
							{
								display_message(WARNING_MESSAGE,
									"CREATE(CMISS_connection).  %s",
									"Could not get CMISS_ROOT environment variable");
								sprintf(global_temp_string,"cm -cmgui %s &",
									cmiss_parameter);
							}
#endif /* defined (OLD_CODE) */
						}
					}
					else
					{
						/* remote, so work out where to start cmiss */
						if (cwd=getcwd((char *)NULL,1000))
						{
							if (type==MACHINE_VMS)
							{
								if (mycm_flag)
								{
									sprintf(global_temp_string,
										"rsh %s \"mycm -cmgui %s\" > /dev/null &",machine,
										cmiss_parameter);
								}
								else
								{
									sprintf(global_temp_string,
										"rsh %s \"cm -cmgui %s\" > /dev/null &",machine,
										cmiss_parameter);
								}
							}
							else
							{
								if (mycm_flag)
								{
									sprintf(global_temp_string,
										"rsh %s \"(cd %s; mycm -cmgui %s)\" > /dev/null &",machine,
										cwd,cmiss_parameter);
								}
								else
								{
									sprintf(global_temp_string,
										"rsh %s \"(cd %s; cm -cmgui %s)\" > /dev/null &",machine,
										cwd,cmiss_parameter);
								}
							}
							/* this should explicitly be a free */
							free(cwd);
						}
						else
						{
							if (mycm_flag)
							{
								sprintf(global_temp_string,"rsh %s mycm -cmgui %s &",
									machine,cmiss_parameter);
							}
							else
							{
								sprintf(global_temp_string,"rsh %s cm -cmgui %s &",
									machine,cmiss_parameter);
							}
							display_message(WARNING_MESSAGE,
								"CREATE(CMISS_connection).  %s",
								"Could not get current directory");
						}
					}
					/* shell off cm */
					printf("executing command '%s'\n",global_temp_string);
					if (-1==system(global_temp_string))
					{
						display_message(WARNING_MESSAGE,"CREATE(CMISS_connection).  %s",
							"Could not shell off cmiss");
					}
#endif /* UNIX */
#if defined (VMS)
					/* spawn cm */
					if (mycm_flag)
					{
						sprintf(global_temp_string,"%s -cmgui %s",mycm_executable,
							cmiss_parameter);
					}
					else
					{
						sprintf(global_temp_string,"%s -cmgui %s",cm_executable,
							cmiss_parameter);
					}
#if defined (OLD_CODE)
					sprintf(global_temp_string,"cmc -cmgui %s",cmiss_parameter);
#endif /* defined (OLD_CODE) */
					printf("spawning command '%s'\n",global_temp_string);
					command_descriptor.dsc$w_length=strlen(global_temp_string);
					command_descriptor.dsc$a_pointer=global_temp_string;
					command_descriptor.dsc$b_class=DSC$K_CLASS_S;
					command_descriptor.dsc$b_dtype=DSC$K_DTYPE_T;
					flags=CLI$M_NOWAIT;
					lib$spawn(&command_descriptor,NULL,NULL,&flags);
#endif /* defined (VMS) */
				}
				if ((return_struct->command_input=CREATE(Wh_input)(
					connection_info[0][0],(char *)NULL,wormhole_timeout))&&
					(return_struct->command_output=
					CREATE(Wh_output)(connection_info[0][1],(char *)NULL))&&
					(return_struct->prompt_input=CREATE(Wh_input)(connection_info[1][0],
					(char *)NULL,wormhole_timeout))&&
					(return_struct->prompt_output=
					CREATE(Wh_output)(connection_info[1][1],(char *)NULL))&&
					(return_struct->data_input=CREATE(Wh_input)(connection_info[2][0],
					(char *)NULL,wormhole_timeout))&&
					(return_struct->data_output=
					CREATE(Wh_output)(connection_info[2][1],(char *)NULL)))
				{
					return_struct->element_manager=element_manager;
/*???DB.  Element transfer needs debugging */
#if defined (NOT_DEBUG)
/*???DB.  Temporary to disable data transfer */
#if defined (NOT_TEMPORARY)
					return_struct->element_manager_callback_id=
						MANAGER_REGISTER(FE_element)(
							CMISS_connection_element_global_change,return_struct,
							return_struct->element_manager);
#else /* defined (NOT_TEMPORARY) */
					return_struct->element_manager_callback_id=NULL;
#endif /* defined (NOT_TEMPORARY) */
#else /* defined (NOT_DEBUG) */
					return_struct->element_manager_callback_id=NULL;
#endif /* defined (NOT_DEBUG) */
					return_struct->element_group_manager=element_group_manager;
					return_struct->fe_field_manager=fe_field_manager;
					return_struct->fe_time=fe_time;
					return_struct->node_manager=node_manager;
					return_struct->data_manager=data_manager;
/*???DB.  Temporary to disable data transfer */
#if defined (NOT_TEMPORARY)
					return_struct->data_manager_callback_id=
						MANAGER_REGISTER(FE_node)(
							CMISS_connection_data_global_change,return_struct,
							return_struct->data_manager);
#else /* defined (NOT_TEMPORARY) */
					return_struct->data_manager_callback_id=NULL;
#endif /* defined (NOT_TEMPORARY) */
/*???DB.  Temporary to disable data transfer */
#if defined (NOT_TEMPORARY)
					return_struct->node_manager_callback_id=
						MANAGER_REGISTER(FE_node)(
							CMISS_connection_node_global_change,return_struct,
							return_struct->node_manager);
#else /* defined (NOT_TEMPORARY) */
					return_struct->node_manager_callback_id=NULL;
#endif /* defined (NOT_TEMPORARY) */
					return_struct->node_group_manager=node_group_manager;
					return_struct->data_group_manager=data_group_manager;
					return_struct->prompt_window_address=prompt_window_address;
					return_struct->user_interface=user_interface;
					/* fsm information */
					return_struct->data_input_state=CMISS_DATA_INPUT_STATE_TYPE;
					return_struct->new_element_group=(struct GROUP(FE_element) *)NULL;
					return_struct->current_element_field_info=
						(struct FE_element_field_info *)NULL;
					return_struct->new_node_group=(struct GROUP(FE_node) *)NULL;
					return_struct->template_node=(struct FE_node *)NULL;
					return_struct->template_node_fields=(struct FE_field **)NULL;
					return_struct->template_node_number_of_fields=0;
					/* I guess we shell out something here... */
				}
				else
				{
					DEALLOCATE(return_struct->name);
					DEALLOCATE(return_struct);
					display_message(ERROR_MESSAGE,"CREATE(CMISS_connection).  %s",
						"Could not create wormholes");
				}
			}
			else
			{
				DEALLOCATE(return_struct);
				display_message(ERROR_MESSAGE,"CREATE(CMISS_connection).  %s",
					"Could not allocate memory");
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,"CREATE(CMISS_connection).  %s",
				"Could not allocate memory for connection");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"CREATE(CMISS_connection).  %s",
			"Invalid arguments");
	}
	LEAVE;

	return (return_struct);
} /* CREATE(CMISS_connection) */

int DESTROY(CMISS_connection)(struct CMISS_connection **connection_address)
/*******************************************************************************
LAST MODIFIED : 23 January 2002

DESCRIPTION :
Frees the memory for the connection, sets <*node_address> to NULL.
==============================================================================*/
{
	int i,return_code;
	struct CMISS_connection *connection;

	ENTER(DESTROY(CMISS_connection));
	return_code=0;
	if ((connection_address)&&(connection= *connection_address))
	{
		/* tell the back end to quit */
		if (!connection->cm_quit)
		{
			/* Don't send a message if this destroy is in response to the 
				back end going */
			CMISS_connection_process_command(connection_address,"quit",(Widget)NULL);
		}
		if (connection= *connection_address)
		{
			/* destroy the connection */
			DEALLOCATE(connection->name);
/*???DB.  Element transfer needs debugging */
#if defined (NOT_DEBUG)
/*???DB.  Temporary to disable data transfer */
#if defined (NOT_TEMPORARY)
			MANAGER_DEREGISTER(FE_element)(connection->element_manager_callback_id,
				connection->element_manager);
#endif /* defined (NOT_TEMPORARY) */
#endif /* defined (NOT_DEBUG) */
			MANAGER_DEREGISTER(FE_node)(connection->node_manager_callback_id,
				connection->node_manager);
			MANAGER_DEREGISTER(FE_node)(connection->data_manager_callback_id,
				connection->data_manager);
			if (DESTROY(Wh_input)(&(connection->command_input))&&
				DESTROY(Wh_output)(&(connection->command_output))&&
				DESTROY(Wh_input)(&(connection->prompt_input))&&
				DESTROY(Wh_output)(&(connection->prompt_output))&&
				DESTROY(Wh_input)(&(connection->data_input))&&
				DESTROY(Wh_output)(&(connection->data_output)))
			{
				if (connection->new_node_group)
				{
					display_message(WARNING_MESSAGE,"DESTROY(CMISS_connection).  %s",
						"Non-NULL new node group");
					DESTROY(GROUP(FE_node))(&(connection->new_node_group));
				}
				if (connection->template_node)
				{
					for (i=0;i<connection->template_node_number_of_fields;i++)
					{
						DEACCESS(FE_field)((connection->template_node_fields)+i);
					}
					DEALLOCATE(connection->template_node_fields);
					DEACCESS(FE_node)(&(connection->template_node));
				}
				return_code=1;
			}
			else
			{
				display_message(ERROR_MESSAGE,"DESTROY(CMISS_connection).  %s",
					"Could not destroy wormholes");
			}
			DEALLOCATE(*connection_address);
		}
		else
		{
			return_code=1;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"DESTROY(CMISS_connection).  %s",
			"Invalid arguments");
	}
	LEAVE;

	return (return_code);
} /* DESTROY(CMISS_connection) */

int CMISS_connection_process_command(
	struct CMISS_connection **connection_address,char *command,
	Widget modal_widget)
/*******************************************************************************
LAST MODIFIED : 3 October 2001

DESCRIPTION :
Executes the given command within CMISS.
==============================================================================*/
{
	int return_code;
	struct CMISS_connection *connection;
	struct User_interface *user_interface;

	ENTER(CMISS_connection_process_command);
	return_code=0;
	if (connection_address&&(connection= *connection_address))
	{
		if (wh_input_open_message(connection->command_input,0,2))
		{
			if (wh_input_add_char(connection->command_input,strlen(command),command))
			{
				return_code=1;
			}
			else
			{
				display_message(ERROR_MESSAGE,"CMISS_connection_process_command.  "
					"Could not add characters");
			}
			if (wh_input_close_message(connection->command_input))
			{
				wh_input_update(connection->command_input);
				/*???DB.  Wait for cm to acknowledge receipt of command ? */
				if (!(connection->asynchronous_commands)&&!(connection->cm_quit))
				{
					/* Hold on to the user interface in case the connection
						closes on us */
					user_interface = connection->user_interface;
					busy_cursor_on(
#if defined (MOTIF)
						modal_widget,
#endif /* defined (MOTIF) */
						user_interface);
					/* wait for command to complete */
					connection->command_in_progress=1;
					/* when we start processing with application main step the back end
						might quit and then the connection would be destroyed so we need
						to keep checking the connection address pointer */
					while (*connection_address&&((*connection_address)->command_in_progress))
					{
						/* wh_output_wait(connection->command_output,0.001);
						CMISS_connection_update(connection_address);
						connection= *connection_address; */
						application_main_step(user_interface);
					}
					busy_cursor_off(
#if defined (MOTIF)
						modal_widget,
#endif /* defined (MOTIF) */
						user_interface);
				}
			}
			else
			{
				display_message(ERROR_MESSAGE,"CMISS_connection_process_command.  "
					"Could not close message");
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,"CMISS_connection_process_command.  "
				"Could not open message");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"CMISS_connection_process_command.  "
			"Invalid connection");
	}
	LEAVE;

	return (return_code);
} /* CMISS_connection_process_command */

int CMISS_connection_process_prompt_reply(struct CMISS_connection *connection,
	char *reply)
/*******************************************************************************
LAST MODIFIED : 3 December 1996

DESCRIPTION :
Sends the given text in response to the prompt.
==============================================================================*/
{
	int return_code;
	static char default_reply[]="<default>";

	ENTER(CMISS_connection_process_prompt_reply);
	return_code=0;
	if (connection)
	{
		if (wh_input_open_message(connection->prompt_input,0,2))
		{
			/* cant send zero length strings, so check first */
			if (!strlen(reply))
			{
				reply = default_reply;
			}
			if (wh_input_add_char(connection->prompt_input,strlen(reply),reply))
			{
				return_code=1;
			}
			else
			{
				display_message(ERROR_MESSAGE,"CMISS_connection_process_prompt_reply.  "
					"Could not add characters");
			}
			if (!wh_input_close_message(connection->prompt_input))
			{
				display_message(ERROR_MESSAGE,"CMISS_connection_process_prompt_reply.  "
					"Could not close message");
			}
		}
		else
		{
			display_message(ERROR_MESSAGE,"CMISS_connection_process_prompt_reply.  "
				"Could not open message");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"CMISS_connection_process_prompt_reply.  %s",
			"Invalid connection");
	}
	LEAVE;

	return (return_code);
} /* CMISS_connection_process_prompt_reply */

int CMISS_connection_update(struct CMISS_connection **connection_address)
/*******************************************************************************
LAST MODIFIED : 3 October 2001

DESCRIPTION :
Performs any updating necessary.
NOTE:  This routine may cause the link to 'commit suicide'.  Do not rely on
the CMISS_connection being valid after a call to this routine.
==============================================================================*/
{
	int return_code;
	int close_message,i,primary_id,secondary_id,num_items;
	char *output_string,*formatted_output_string;
	int commit_suicide;
	struct CMISS_connection *connection;

	ENTER(CMISS_connection_update);
	return_code=0;
	commit_suicide=0;
	if (connection_address&&(connection= *connection_address))
	{
		if (wh_input_update(connection->command_input)&&
			wh_output_update(connection->command_output)&&
			wh_input_update(connection->prompt_input)&&
			wh_output_update(connection->prompt_output)&&
			wh_input_update(connection->data_input)&&
			wh_output_update(connection->data_output))
		{
			/* check for any data messages to be acted upon */
			CMISS_connection_get_data(connection);
			/* check for any commands */
			while(wh_output_can_open(connection->command_output))
			{
				wh_output_open_message(connection->command_output,&primary_id,
					&secondary_id);
				close_message=1;
				switch (secondary_id)
				{
					case 1: /* idle message - echo it back */
					{
						wh_input_open_message(connection->command_input,0,1);
						wh_input_close_message(connection->command_input);
					} break;
					case 2: /* standard output */
					{
						wh_output_get_int(connection->command_output,1,&num_items);
						for (i=0;i<num_items;i++)
						{
							if (output_string=wh_output_get_unknown_string(
								connection->command_output))
							{
								display_message(INFORMATION_MESSAGE,"%s\n",output_string);
								DEALLOCATE(output_string);
							}
							else
							{
								display_message(ERROR_MESSAGE,"CMISS_connection_update.  "
									"Could not retrieve output string");
							}
						}
					} break;
					case 3: /* the back end is closing down */
					{
						commit_suicide=1;
						connection->command_in_progress=0;
						connection->cm_quit=1;
					} break;
					case 5: /* cm command completion */
					{
						wh_output_get_remainder(connection->command_output);
						close_message=0;
						connection->command_in_progress=0;
					} break;
					default: /* unknown message */
					{
						wh_output_get_remainder(connection->command_output);
						close_message=0;
						display_message(WARNING_MESSAGE,"CMISS_connection_update.  "
							"Invalid secondary id %d",secondary_id);
					} break;
				}
				if (close_message)
				{
					if (!wh_output_close_message(connection->command_output))
					{
						/* last ditch attempt to resync */
						display_message(ERROR_MESSAGE,"CMISS_connection_update.  %s",
							"Error receiving command");
						if (wh_output_get_remainder(connection->command_output))
						{
							display_message(ERROR_MESSAGE,"CMISS_connection_update.  %s",
								"Attempting to rectify");
						}
						else
						{
							display_message(ERROR_MESSAGE,"CMISS_connection_update.  %s",
								"Could not attempt to rectify");
						}
					}
				}
			}
			while(wh_output_can_open(connection->prompt_output))
			{
				wh_output_open_message(connection->prompt_output,&primary_id,
					&secondary_id);
				close_message=1;
				switch (secondary_id)
				{
					case 1: /* idle message - echo it back */
					{
						wh_input_open_message(connection->prompt_input,0,1);
						wh_input_close_message(connection->prompt_input);
					} break;
					case 2: /* question prompt */
					{
						if (output_string=wh_output_get_unknown_string(
							connection->prompt_output))
						{
							if (formatted_output_string=
								reformat_fortran_string(output_string))
							{
								write_question(formatted_output_string,
									connection->prompt_window_address,
									connection->user_interface->application_shell);
								DEALLOCATE(formatted_output_string);
							}
							else
							{
								display_message(ERROR_MESSAGE,"CMISS_connection_update.  %s",
									"Could not format prompt string");
							}
							DEALLOCATE(output_string);
						}
						else
						{
							display_message(ERROR_MESSAGE,"CMISS_connection_update.  %s",
								"Could not retrieve prompt string");
						}
					} break;
					case 3: /* the back end in main loop -ie finished asking questions */
					{
						if (*(connection->prompt_window_address))
						{
							popdown_prompt_window(*(connection->prompt_window_address));
						}
					} break;
					default: /* unknown message */
					{
						wh_output_get_remainder(connection->prompt_output);
						close_message=0;
						display_message(WARNING_MESSAGE,"CMISS_connection_update.  %s",
							"Invalid secondary id");
					} break;
				}
				if (close_message)
				{
					if (!wh_output_close_message(connection->prompt_output))
					{
						/* last ditch attempt to resync */
						display_message(ERROR_MESSAGE,"CMISS_connection_update.  %s",
							"Error receiving prompt");
						if (wh_output_get_remainder(connection->prompt_output))
						{
							display_message(ERROR_MESSAGE,"CMISS_connection_update.  %s",
								"Attempting to rectify");
						}
						else
						{
							display_message(ERROR_MESSAGE,"CMISS_connection_update.  %s",
								"Could not attempt to rectify");
						}
					}
				}
			}
			return_code=1;
		}
		else
		{
			display_message(ERROR_MESSAGE,"CMISS_connection_update.  %s",
				"Error updating wormholes");
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,"CMISS_connection_update.  %s",
			"Invalid connection");
	}
	/* this MUST be the last portion of code */
	if (commit_suicide)
	{
		DESTROY(CMISS_connection)(connection_address);
		/* and we now do not exist anymore */
	}
	LEAVE;

	return (return_code);
} /* CMISS_connection_update */
