/*******************************************************************************
FILE : comfile.c

LAST MODIFIED : 29 June 2002

DESCRIPTION :
Commands for comfiles.
==============================================================================*/
/* OpenCMISS-Cmgui Application
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#if 1
#include "configure/cmgui_configure.h"
#endif /* defined (1) */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if !defined (WIN32_SYSTEM)
#include <unistd.h>
#endif /* !defined (WIN32_SYSTEM) */
#include "general/debug.h"
#include "comfile/comfile.h"
#if defined (WX_USER_INTERFACE)
#include "comfile/comfile_window_wx.h"
#endif /* WX_USER_INTERFACE */
#include "command/command.h"
#include "general/mystring.h"
#include "general/object.h"
#include "user_interface/confirmation.h"
#include "user_interface/filedir.h"
#include "general/message.h"

int open_comfile(struct Parse_state *state,void *dummy_to_be_modified,
	void *open_comfile_data_void)
/*******************************************************************************
LAST MODIFIED : 18 April 2002

DESCRIPTION :
Opens a comfile, and a window if it is to be executed.  If a comfile is not
specified on the command line, a file selection box is presented to the user.
==============================================================================*/
{
	 char *command_string, *filename;
#if defined (WX_USER_INTERFACE)
	char *name;
#endif /* defined (WX_USER_INTERFACE) */
	int i,length,return_code;
#if defined (WX_USER_INTERFACE)
	struct Comfile_window *comfile_window;
#endif /* defined (WX_USER_INTERFACE) */
	struct Open_comfile_data *open_comfile_data;
	struct Option_table *option_table;
#if defined (WX_USER_INTERFACE)
	char *last, *pathname, *temp_string, *old_directory, *old_directory_name;
	temp_string=NULL;
	pathname=NULL;
	last=NULL;
	old_directory = NULL;
	old_directory_name = NULL;
#endif /* defined (WX_USER_INTERFACE) */

	ENTER(open_comfile);
	USE_PARAMETER(dummy_to_be_modified);
	/* check arguments */
	open_comfile_data=(struct Open_comfile_data *)open_comfile_data_void;
	if (open_comfile_data != 0)
	{
		if (state)
		{
			if (open_comfile_data->file_name)
			{
				filename = duplicate_string(open_comfile_data->file_name);
			}
			else
			{
				filename = (char *)NULL;
			}
			option_table = CREATE(Option_table)();
			/* example */
			Option_table_add_entry(option_table, open_comfile_data->example_symbol,
				&(open_comfile_data->example_flag), NULL, set_char_flag);
			/* execute */
			Option_table_add_entry(option_table, "execute",
				&(open_comfile_data->execute_count), NULL, set_int_optional);
			/* name */
			Option_table_add_entry(option_table, "name",
				&filename, (void *)1, set_name);
			/* default */
			Option_table_add_entry(option_table, (const char *)NULL,
				&filename, NULL, set_name);
			return_code=Option_table_multi_parse(option_table,state);
			DESTROY(Option_table)(&option_table);
			/* no errors, not asking for help */
			if (return_code)
			{
				/* Prompt if we don't have a filename */
				if (!filename)
				{
					if (!(filename = confirmation_get_read_filename(
									 open_comfile_data->file_extension, open_comfile_data->user_interface
#if defined (WX_USER_INTERFACE)
, open_comfile_data->execute_command
#endif /*defined (WX_USER_INTERFACE)*/
									 )))
					{
						/* Cancelling dialog and returning an empty filename
						 * is not an error but we don't want to do any more work */
						return 1;
					}
				}
			}
			if (return_code)
			{
				if (open_comfile_data->example_flag)
				{
					/* include the relative path */
					length=strlen(filename)+1;
					if (open_comfile_data->examples_directory)
					{
						length += strlen(open_comfile_data->examples_directory);
					}
					if (ALLOCATE(command_string,char,length))
					{
						*command_string='\0';
						if (open_comfile_data->examples_directory)
						{
							strcat(command_string,open_comfile_data->examples_directory);
						}
						strcat(command_string,filename);
						DEALLOCATE(filename);
						filename=command_string;
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"open_comfile.  Insufficient memory");
					}
				}
				else
				{
#if defined (WX_USER_INTERFACE)
#if defined (WIN32_SYSTEM)
					char *drive_name = NULL;
					char *first = strchr(filename, '\\');
					char *last = strrchr(filename, '\\');
					int lastlength = (last) ? (last - filename + 1) : 0;
					length = (first) ? (first - filename + 1) : 0;
					if (length > 1)
					{
						if (ALLOCATE(temp_string, char, length + 9))
						{
							strcpy(temp_string, "set dir ");
							strncat(temp_string, filename, length - 1);
							Execute_command_execute_string(open_comfile_data->execute_command, temp_string);
							DEALLOCATE(temp_string);
						}
					}
					if (lastlength > length)
					{
						if (ALLOCATE(temp_string, char, lastlength - length + 12))
						{
							strcpy(temp_string, "set dir '\\");
							strncat(temp_string, filename + length, lastlength - length - 1);
							strcat(temp_string, "'");
							Execute_command_execute_string(open_comfile_data->execute_command, temp_string);
							DEALLOCATE(temp_string);
						}
					}
#else /* defined (WIN32_SYSTEM)*/
					/* Save the current working directory */
					old_directory = (char *)malloc(4096);
					if ((NULL != getcwd(old_directory, 4096)) &&
						(NULL != old_directory))
					{
						length = strlen(old_directory);
						if (ALLOCATE(old_directory_name,char,length+2))
						{
							strcpy(old_directory_name, old_directory);
							strcat(old_directory_name,"/");
							old_directory_name[length+1]='\0';
						}
					}

					/* Set the current directory to that of filename */
					if (!(0 < open_comfile_data->execute_count))
					{
						last = strrchr(filename, '/');
						if (last != NULL)
						{
							length = last-filename+1;
							pathname = NULL;
							temp_string = NULL;
							if (ALLOCATE(pathname,char,length+1))
							{
								strncpy(pathname,filename,length);
								pathname[length]='\0';
								if ((NULL != old_directory_name) &&
									 strcmp (old_directory_name,pathname) != 0)
								{
									make_valid_token(&pathname);
									length = strlen(pathname);
									if (ALLOCATE(temp_string,char,length+9))
									{
										strcpy(temp_string, "set dir ");
										strcat(temp_string, pathname);
										temp_string[length+8]='\0';
										Execute_command_execute_string(open_comfile_data->execute_command,temp_string);
										DEALLOCATE(temp_string);
									}
								}
							}
						}
					}
#endif /*defined (WIN32_SYSTEM) */
#endif /*defined (WX_USER_INTERFACE) */
				}
#if defined (WX_USER_INTERFACE)  && defined (WIN32_SYSTEM)
				const char *last = strrchr(filename, '\\');
				if (last)
				{
					memmove(filename, last + 1, strlen(last));
				}
#endif /* defined (WX_USER_INTERFACE)  && (WIN32_SYSTEM)*/
						 /* open the file */
				return_code = check_suffix(&filename,
					open_comfile_data->file_extension);
				if (return_code)
				{
					if (0 < open_comfile_data->execute_count)
					{
						for (i=open_comfile_data->execute_count;i>0;i--)
						{
							 execute_comfile(filename, open_comfile_data->io_stream_package,
								open_comfile_data->execute_command);

						}
#if defined (WX_USER_INTERFACE)
						/* Change back to original dir */
						if ((old_directory_name != NULL) && (pathname != NULL))
						{
							 if (strcmp (old_directory_name,pathname) != 0)
							 {
								make_valid_token(&old_directory_name);
								length = strlen(old_directory_name);
								temp_string = NULL;
								if (ALLOCATE(temp_string,char,length+9))
								{
									strcpy(temp_string, "set dir ");
									strcat(temp_string, old_directory_name);
									temp_string[length+8]='\0';
									Execute_command_execute_string(open_comfile_data->execute_command,temp_string);
									DEALLOCATE(temp_string);
								}
							}
						}
#endif /*defined (WX_USER_INTERFACE)*/
					}
					else
					{
#if defined (WX_USER_INTERFACE)
							name = Comfile_window_manager_make_unique_name(
										 open_comfile_data->comfile_window_manager,
										 filename);
							if (name != 0)
							{
								 comfile_window = CREATE(Comfile_window)(name,
											 filename, open_comfile_data->io_stream_package,
											 open_comfile_data->execute_command,
											 open_comfile_data->set_command,
											 open_comfile_data->user_interface);
								 if (comfile_window != 0)
								 {
										if (ADD_OBJECT_TO_MANAGER(Comfile_window)(comfile_window,
													open_comfile_data->comfile_window_manager))
										{
											 return_code = 1;
										}
										else
										{
											 display_message(ERROR_MESSAGE,
													"open_comfile.  Could not manage comfile window");
											 DESTROY(Comfile_window)(&comfile_window);
											 return_code = 0;
										}
								 }
								 else
								 {
										display_message(ERROR_MESSAGE,
											 "open_comfile.  Could not create comfile window");
										return_code=0;
								 }
								 DEALLOCATE(name);
							}
							else
							{
								 display_message(ERROR_MESSAGE,
										"open_comfile.  Could not allocate window name");
								 return_code=0;
							}
#else /* defined (WX_USER_INTERFACE) */
							display_message(ERROR_MESSAGE,
								 "open_comfile.  Cannot create a comfile dialog, use execute.");
						return_code=0;
#endif /* defined (WX_USER_INTERFACE) */
					 }
#if defined (WX_USER_INTERFACE)
					 if (old_directory_name)
					 {
							DEALLOCATE(old_directory_name);
					 }
					 if (old_directory)
					 {
							free(old_directory);
					 }
					 if (pathname)
					 {
							DEALLOCATE(pathname);
					 }
#endif /*defined (WX_USER_INTERFACE)*/
				}
			}
			if (filename)
			{
				 DEALLOCATE(filename);
			}
		}
		else
		{
			 display_message(ERROR_MESSAGE,"open_comfile.  Missing state");
			 return_code=0;
		}
	}
	else
	{
		 display_message(ERROR_MESSAGE,"open_comfile.  Missing open_comfile_data");
		 return_code=0;
	}

	LEAVE;

	return (return_code);
} /* open_comfile */
