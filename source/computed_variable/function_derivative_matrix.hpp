//******************************************************************************
// FILE : function_derivative_matrix.hpp
//
// LAST MODIFIED : 5 March 2004
//
// DESCRIPTION :
//==============================================================================
#if !defined (__FUNCTION_DERIVATIVE_MATRIX_HPP__)
#define __FUNCTION_DERIVATIVE_MATRIX_HPP__

#include <list>

#include "computed_variable/function.hpp"
#include "computed_variable/function_variable.hpp"

class Function_derivative_matrix;

typedef boost::intrusive_ptr<Function_derivative_matrix>
	Function_derivative_matrix_handle;

class Function_derivative_matrix : public Function
//******************************************************************************
// LAST MODIFIED : 5 March 2004
//
// DESCRIPTION :
//==============================================================================
{
	friend class Function_variable_derivative_matrix;
	friend class
		Function_variable_iterator_representation_atomic_derivative_matrix;
	friend Function_handle Function_variable::evaluate_derivative(
		std::list<Function_variable_handle>& independent_variables);
	// inherited
	public:
		string_handle get_string_representation();
		Function_variable_handle input();
		Function_variable_handle output();
	// additional
	public:
		// get the specified partial derivative
		Function_handle matrix(
			std::list<Function_variable_handle>& partial_independent_variables);
		// implement the chain rule for differentiation
		friend Function_derivative_matrix_handle
			Function_derivative_matrix_compose(
			const Function_variable_handle& dependent_variable,
			const Function_derivative_matrix_handle& derivative_f,
			const Function_derivative_matrix_handle& derivative_g);
		// calculate the composition inverse
		Function_handle inverse();
	private:
		Function_handle evaluate(Function_variable_handle atomic_variable);
		bool evaluate_derivative(Scalar& derivative,
			Function_variable_handle atomic_variable,
			std::list<Function_variable_handle>& atomic_independent_variables);
		bool set_value(Function_variable_handle atomic_variable,
			Function_variable_handle atomic_value);
	private:
		Function_derivative_matrix(
			const Function_variable_handle& dependent_variable,
			const std::list<Function_variable_handle>& independent_variables,
			const std::list<Matrix>& matrices);
		// calls (dependent_variable->function())->evaluate_derivative to fill in
		//   the matrices.  Used by Function_variable::evaluate_derivative
		Function_derivative_matrix(
			const Function_variable_handle& dependent_variable,
			const std::list<Function_variable_handle>& independent_variables);
		// copy constructor
		Function_derivative_matrix(const Function_derivative_matrix&);
		// assignment
		Function_derivative_matrix& operator=(const Function_derivative_matrix&);
		// destructor
		virtual ~Function_derivative_matrix();
	private:
		//???DB.  Should be Function_variable rather Function_variable_handle
		//  because when Function_variable changes derivative matrix doesn't
		//  automatically change?
		Function_variable_handle dependent_variable;
		std::list<Function_variable_handle> independent_variables;
		std::list<Matrix> matrices;
};

#endif /* !defined (__FUNCTION_DERIVATIVE_MATRIX_HPP__) */
