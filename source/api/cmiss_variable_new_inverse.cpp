/*******************************************************************************
FILE : api/cmiss_variable_new_inverse.cpp

LAST MODIFIED : 2 February 2004

DESCRIPTION :
The public interface to the Cmiss_variable_new inverse object.
==============================================================================*/

#include <new>
#include "api/cmiss_variable_new_inverse.h"
#include "computed_variable/variable_inverse.hpp"

/*
Global functions
----------------
*/
Cmiss_variable_new_id Cmiss_variable_new_inverse_create(
	Cmiss_variable_new_input_id dependent_variable,
	Cmiss_variable_new_id independent_variable)
/*******************************************************************************
LAST MODIFIED : 2 February 2004

DESCRIPTION :
Creates a Cmiss_variable_new inverse with the supplied <dependent_variable>
and <independent_variable>.
==============================================================================*/
{
	Cmiss_variable_new_id result;
#if defined (USE_SMART_POINTER)
	Variable_handle *independent_variable_handle_address;
#if defined (USE_VARIABLE_INPUT)
	Variable_input_handle
#else // defined (USE_VARIABLE_INPUT)
	Variable_io_specifier_handle
#endif // defined (USE_VARIABLE_INPUT)
		*dependent_variable_handle_address;
#else /* defined (USE_SMART_POINTER) */
	Variable_handle independent_variable_handle;
#if defined (USE_VARIABLE_INPUT)
	Variable_input_handle
#else // defined (USE_VARIABLE_INPUT)
	Variable_io_specifier_handle
#endif // defined (USE_VARIABLE_INPUT)
		dependent_variable_handle;
#endif /* defined (USE_SMART_POINTER) */

	result=0;
	if (
#if defined (USE_SMART_POINTER)
		(dependent_variable_handle_address=
		reinterpret_cast<
#if defined (USE_VARIABLE_INPUT)
		Variable_input_handle
#else // defined (USE_VARIABLE_INPUT)
		Variable_io_specifier_handle
#endif // defined (USE_VARIABLE_INPUT)
		*>(dependent_variable))&&
		(independent_variable_handle_address=reinterpret_cast<Variable_handle *>(
		independent_variable))
#else /* defined (USE_SMART_POINTER) */
		(dependent_variable_handle=reinterpret_cast<
#if defined (USE_VARIABLE_INPUT)
		Variable_input_handle
#else // defined (USE_VARIABLE_INPUT)
		Variable_io_specifier_handle
#endif // defined (USE_VARIABLE_INPUT)
		>(dependent_variable))&&
		(independent_variable_handle=reinterpret_cast<Variable_handle>(
		independent_variable))
#endif /* defined (USE_SMART_POINTER) */
		)
	{
		result=reinterpret_cast<Cmiss_variable_new_id>(
#if defined (USE_SMART_POINTER)
			new Variable_inverse_handle(new Variable_inverse(
			*dependent_variable_handle_address,*independent_variable_handle_address))
#else /* defined (USE_SMART_POINTER) */
			new Variable_inverse(dependent_variable_handle,
			*independent_variable_address)
#endif /* defined (USE_SMART_POINTER) */
			);
	}

	return (result);
}

Cmiss_variable_new_input_id Cmiss_variable_new_input_inverse_independent(
	Cmiss_variable_new_id variable_inverse)
/*******************************************************************************
LAST MODIFIED : 2 February 2004

DESCRIPTION :
Returns the independent input for the <variable_inverse>.
==============================================================================*/
{
	Cmiss_variable_new_input_id result;
	Variable_inverse_handle variable_inverse_handle;
#if defined (USE_SMART_POINTER)
	Variable_handle *variable_handle_address;
#else /* defined (USE_SMART_POINTER) */
	Variable *variable_address;
#endif /* defined (USE_SMART_POINTER) */

	if (
#if defined (USE_SMART_POINTER)
		(variable_handle_address=reinterpret_cast<Variable_handle *>(
		variable_inverse))&&
		(variable_inverse_handle=boost::dynamic_pointer_cast<Variable_inverse,
		Variable>(*variable_handle_address))
#else /* defined (USE_SMART_POINTER) */
		(variable_address=reinterpret_cast<Variable *>(variable_inverse))&&
		(variable_inverse_handle=dynamic_cast<Variable_inverse *>(variable_address))
#endif /* defined (USE_SMART_POINTER) */
		)
	{
		result=reinterpret_cast<Cmiss_variable_new_input_id>(
#if defined (USE_SMART_POINTER)
			new
#if defined (USE_VARIABLE_INPUT)
			Variable_input_handle
#else // defined (USE_VARIABLE_INPUT)
			Variable_io_specifier_handle
#endif // defined (USE_VARIABLE_INPUT)
			(
#endif /* defined (USE_SMART_POINTER) */
			variable_inverse_handle->input_independent()
#if defined (USE_SMART_POINTER)
			)
#endif /* defined (USE_SMART_POINTER) */
			);
	}
	else
	{
		result=0;
	}

	return (result);
}

Cmiss_variable_new_input_id Cmiss_variable_new_input_inverse_step_tolerance(
	Cmiss_variable_new_id variable_inverse)
/*******************************************************************************
LAST MODIFIED : 2 February 2004

DESCRIPTION :
Returns the step tolerance input for the <variable_inverse>.
==============================================================================*/
{
	Cmiss_variable_new_input_id result;
	Variable_inverse_handle variable_inverse_handle;
#if defined (USE_SMART_POINTER)
	Variable_handle *variable_handle_address;
#else /* defined (USE_SMART_POINTER) */
	Variable *variable_address;
#endif /* defined (USE_SMART_POINTER) */

	if (
#if defined (USE_SMART_POINTER)
		(variable_handle_address=reinterpret_cast<Variable_handle *>(
		variable_inverse))&&
		(variable_inverse_handle=boost::dynamic_pointer_cast<Variable_inverse,
		Variable>(*variable_handle_address))
#else /* defined (USE_SMART_POINTER) */
		(variable_address=reinterpret_cast<Variable *>(variable_inverse))&&
		(variable_inverse_handle=dynamic_cast<Variable_inverse *>(variable_address))
#endif /* defined (USE_SMART_POINTER) */
		)
	{
		result=reinterpret_cast<Cmiss_variable_new_input_id>(
#if defined (USE_SMART_POINTER)
			new
#if defined (USE_VARIABLE_INPUT)
			Variable_input_handle
#else // defined (USE_VARIABLE_INPUT)
			Variable_io_specifier_handle
#endif // defined (USE_VARIABLE_INPUT)
			(
#endif /* defined (USE_SMART_POINTER) */
			variable_inverse_handle->input_step_tolerance()
#if defined (USE_SMART_POINTER)
			)
#endif /* defined (USE_SMART_POINTER) */
			);
	}
	else
	{
		result=0;
	}

	return (result);
}

Cmiss_variable_new_input_id Cmiss_variable_new_input_inverse_value_tolerance(
	Cmiss_variable_new_id variable_inverse)
/*******************************************************************************
LAST MODIFIED : 2 February 2004

DESCRIPTION :
Returns the value tolerance input for the <variable_inverse>.
==============================================================================*/
{
	Cmiss_variable_new_input_id result;
	Variable_inverse_handle variable_inverse_handle;
#if defined (USE_SMART_POINTER)
	Variable_handle *variable_handle_address;
#else /* defined (USE_SMART_POINTER) */
	Variable *variable_address;
#endif /* defined (USE_SMART_POINTER) */

	if (
#if defined (USE_SMART_POINTER)
		(variable_handle_address=reinterpret_cast<Variable_handle *>(
		variable_inverse))&&
		(variable_inverse_handle=boost::dynamic_pointer_cast<Variable_inverse,
		Variable>(*variable_handle_address))
#else /* defined (USE_SMART_POINTER) */
		(variable_address=reinterpret_cast<Variable *>(variable_inverse))&&
		(variable_inverse_handle=dynamic_cast<Variable_inverse *>(variable_address))
#endif /* defined (USE_SMART_POINTER) */
		)
	{
		result=reinterpret_cast<Cmiss_variable_new_input_id>(
#if defined (USE_SMART_POINTER)
			new
#if defined (USE_VARIABLE_INPUT)
			Variable_input_handle
#else // defined (USE_VARIABLE_INPUT)
			Variable_io_specifier_handle
#endif // defined (USE_VARIABLE_INPUT)
			(
#endif /* defined (USE_SMART_POINTER) */
			variable_inverse_handle->input_value_tolerance()
#if defined (USE_SMART_POINTER)
			)
#endif /* defined (USE_SMART_POINTER) */
			);
	}
	else
	{
		result=0;
	}

	return (result);
}

Cmiss_variable_new_input_id
	Cmiss_variable_new_input_inverse_maximum_iterations(
	Cmiss_variable_new_id variable_inverse)
/*******************************************************************************
LAST MODIFIED : 2 February 2004

DESCRIPTION :
Returns the maximum_iterations input for the <variable_inverse>.
==============================================================================*/
{
	Cmiss_variable_new_input_id result;
	Variable_inverse_handle variable_inverse_handle;
#if defined (USE_SMART_POINTER)
	Variable_handle *variable_handle_address;
#else /* defined (USE_SMART_POINTER) */
	Variable *variable_address;
#endif /* defined (USE_SMART_POINTER) */

	if (
#if defined (USE_SMART_POINTER)
		(variable_handle_address=reinterpret_cast<Variable_handle *>(
		variable_inverse))&&
		(variable_inverse_handle=boost::dynamic_pointer_cast<Variable_inverse,
		Variable>(*variable_handle_address))
#else /* defined (USE_SMART_POINTER) */
		(variable_address=reinterpret_cast<Variable *>(variable_inverse))&&
		(variable_inverse_handle=dynamic_cast<Variable_inverse *>(variable_address))
#endif /* defined (USE_SMART_POINTER) */
		)
	{
		result=reinterpret_cast<Cmiss_variable_new_input_id>(
#if defined (USE_SMART_POINTER)
			new
#if defined (USE_VARIABLE_INPUT)
			Variable_input_handle
#else // defined (USE_VARIABLE_INPUT)
			Variable_io_specifier_handle
#endif // defined (USE_VARIABLE_INPUT)
			(
#endif /* defined (USE_SMART_POINTER) */
			variable_inverse_handle->input_maximum_iterations()
#if defined (USE_SMART_POINTER)
			)
#endif /* defined (USE_SMART_POINTER) */
			);
	}
	else
	{
		result=0;
	}

	return (result);
}

Cmiss_variable_new_input_id Cmiss_variable_new_input_inverse_dependent_estimate(
	Cmiss_variable_new_id variable_inverse)
/*******************************************************************************
LAST MODIFIED : 2 February 2004

DESCRIPTION :
Returns the dependent_estimate input for the <variable_inverse>.
==============================================================================*/
{
	Cmiss_variable_new_input_id result;
	Variable_inverse_handle variable_inverse_handle;
#if defined (USE_SMART_POINTER)
	Variable_handle *variable_handle_address;
#else /* defined (USE_SMART_POINTER) */
	Variable *variable_address;
#endif /* defined (USE_SMART_POINTER) */

	if (
#if defined (USE_SMART_POINTER)
		(variable_handle_address=reinterpret_cast<Variable_handle *>(
		variable_inverse))&&
		(variable_inverse_handle=boost::dynamic_pointer_cast<Variable_inverse,
		Variable>(*variable_handle_address))
#else /* defined (USE_SMART_POINTER) */
		(variable_address=reinterpret_cast<Variable *>(variable_inverse))&&
		(variable_inverse_handle=dynamic_cast<Variable_inverse *>(variable_address))
#endif /* defined (USE_SMART_POINTER) */
		)
	{
		result=reinterpret_cast<Cmiss_variable_new_input_id>(
#if defined (USE_SMART_POINTER)
			new
#if defined (USE_VARIABLE_INPUT)
			Variable_input_handle
#else // defined (USE_VARIABLE_INPUT)
			Variable_io_specifier_handle
#endif // defined (USE_VARIABLE_INPUT)
			(
#endif /* defined (USE_SMART_POINTER) */
			variable_inverse_handle->input_dependent_estimate()
#if defined (USE_SMART_POINTER)
			)
#endif /* defined (USE_SMART_POINTER) */
			);
	}
	else
	{
		result=0;
	}

	return (result);
}
