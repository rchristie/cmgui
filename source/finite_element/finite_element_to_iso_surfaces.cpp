/***************************************************************************//**
 * FILE : finite_element_to_iso_surfaces.cpp
 * 
 * Functions for creating graphical iso-surfaces from finite element fields.
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
 * Portions created by the Initial Developer are Copyright (C) 2009
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
#include <list>
#include <map>
extern "C" {
#include "computed_field/computed_field.h"
#include "computed_field/computed_field_wrappers.h"
#include "finite_element/finite_element.h"
#include "finite_element/finite_element_to_iso_surfaces.h"
#include "general/debug.h"
#include "general/matrix_vector.h"
}
extern "C" {
#include "user_interface/message.h"
}

/*
Global types
------------
*/

struct Iso_surface_specification
{
public:
	double first_iso_value, *iso_values, iso_value_range, last_iso_value;
	int number_of_iso_values;
	struct Computed_field *coordinate_field, *data_field, *scalar_field,
		*texture_coordinate_field;

public:
	double get_iso_value(int iso_value_number) const;
};

/*
Module types and functions
--------------------------
*/

inline double Iso_surface_specification::get_iso_value(int iso_value_number) const
{
	if (NULL != iso_values)
	{
		return iso_values[iso_value_number];
	}
	return (first_iso_value + (double)iso_value_number * iso_value_range);
}

namespace {

class Point_index
{
public:
	bool exact_xi;
	int i, j, k;
	FE_value xi1, xi2, xi3;
	double scalar;

	Point_index() :
		exact_xi(false), i(0), j(0), k(0)
	{
	}

	Point_index(int i, int j, int k) :
		exact_xi(false), i(i), j(j), k(k)
	{
	}

	Point_index(FE_value xi[3], double scalar) :
		exact_xi(true), xi1(xi[0]), xi2(xi[1]), xi3(xi[2]), scalar(scalar)
	{
	}
	
	void set(int in_i, int in_j, int in_k)
	{
		exact_xi = false;
		i = in_i;
		j = in_j;
		k = in_k;
	}

	bool operator<(const Point_index& other) const
	{
		// index < exact_xi
		if (exact_xi)
		{
			if (other.exact_xi)
			{
				return ((xi1 < other.xi1) || ((xi1 == other.xi1) &&
					((xi2 < other.xi2) || ((xi2 == other.xi2) && (xi3 < other.xi3)))));
			}
			return false;
		}
		else if (other.exact_xi)
		{
			return true;
		}
		return ((i < other.i) || ((i == other.i) &&
			((j < other.j) || ((j == other.j) && (k < other.k)))));
	}
};

class Point_index_pair
{
public:
	Point_index pa;
	Point_index pb;

	Point_index_pair(const Point_index& in_pa, const Point_index& in_pb)
	{
		// ensure pa < pb
		if (in_pa < in_pb)
		{
			pa = in_pa;
			pb = in_pb;
		}
		else
		{
			pa = in_pb;
			pb = in_pa;
		}
	}
};

class Point_index_pair_compare
{
public:
	bool operator() (const Point_index_pair &pp1, const Point_index_pair &pp2) const
	{
		return ((pp1.pa < pp2.pa) || ((!(pp2.pa < pp1.pa)) && (pp1.pb < pp2.pb)));
	}
};

class Iso_vertex
{
public:
	FE_value xi[3];
	FE_value coordinates[3];
	FE_value normal[3];
	FE_value texture_coordinates[3];
	int number_of_data_components; // GRC point to shared object for this?
	GTDATA *data;

	Iso_vertex() :
		number_of_data_components(0),
		data(NULL)
	{
		xi[0] = 0.0;
		xi[1] = 0.0;
		xi[2] = 0.0;
		coordinates[0] = 0.0;
		coordinates[1] = 0.0;
		coordinates[2] = 0.0;
		normal[0] = 0.0;
		normal[1] = 0.0;
		normal[2] = 0.0;
		texture_coordinates[0] = 0.0;
		texture_coordinates[1] = 0.0;
		texture_coordinates[2] = 0.0;
	}

	Iso_vertex(const Iso_vertex& in_vertex) :
		data(NULL)
	{
		operator=(in_vertex);
	}
	
	~Iso_vertex()
	{
		delete[] data;
	}
	
	void operator=(const Iso_vertex& in_vertex)
	{
		xi[0] = in_vertex.xi[0];
		xi[1] = in_vertex.xi[1];
		xi[2] = in_vertex.xi[2];
		coordinates[0] = in_vertex.coordinates[0];
		coordinates[1] = in_vertex.coordinates[1];
		coordinates[2] = in_vertex.coordinates[2];
		normal[0] = in_vertex.normal[0];
		normal[1] = in_vertex.normal[1];
		normal[2] = in_vertex.normal[2];
		texture_coordinates[0] = in_vertex.texture_coordinates[0];
		texture_coordinates[1] = in_vertex.texture_coordinates[1];
		texture_coordinates[2] = in_vertex.texture_coordinates[2];
		number_of_data_components = in_vertex.number_of_data_components;
		delete data;
		if (NULL != in_vertex.data)
		{
			data = new GTDATA[number_of_data_components];
			for (int i = 0; i < number_of_data_components; i++)
			{
				data[i] = in_vertex.data[i];
			}
		}
		else
		{
			data = NULL;
		}
	}
};

inline FE_value square_distance3(const FE_value *c1, const FE_value *c2)
{
	FE_value offset0 = c2[0] - c1[0];
	FE_value offset1 = c2[1] - c1[1];
	FE_value offset2 = c2[2] - c1[2];
	return (offset0*offset0 + offset1*offset1 + offset2*offset2);
}

typedef std::map<Point_index_pair, Iso_vertex*, Point_index_pair_compare> Iso_vertex_map;
typedef std::map<Point_index_pair, Iso_vertex*, Point_index_pair_compare>::iterator Iso_vertex_map_iterator;
typedef std::map<Point_index_pair, Iso_vertex*, Point_index_pair_compare>::const_iterator Iso_vertex_map_const_iterator;

class Iso_triangle
{
public:
	const Iso_vertex *v1, *v2, *v3;
	
	Iso_triangle(const Iso_vertex *v1, const Iso_vertex *v2, const Iso_vertex *v3) :
		v1(v1), v2(v2), v3(v3)
	{
	}
};

typedef std::list<Iso_triangle*> Iso_triangle_list;
typedef std::list<Iso_triangle*>::iterator Iso_triangle_list_iterator;
typedef std::list<Iso_triangle*>::const_iterator Iso_triangle_list_const_iterator;

class Iso_mesh
{
public:
	Iso_vertex_map vertex_map;
	Iso_triangle_list triangle_list;
	
	Iso_mesh() :
		vertex_map(Point_index_pair_compare()),
		triangle_list()
	{
	}

	~Iso_mesh()
	{
		for (Iso_triangle_list_iterator triangle_iter = triangle_list.begin(); triangle_iter != triangle_list.end(); triangle_iter++)
		{
			delete (*triangle_iter);
		}
		for (Iso_vertex_map_iterator vertex_iter = vertex_map.begin(); vertex_iter != vertex_map.end(); vertex_iter++)
		{
			Iso_vertex *vertex = vertex_iter->second;
			delete (vertex);
		}
	}
	
	const Iso_vertex *get_vertex(const Point_index_pair& pp) const
	{
		Iso_vertex_map_const_iterator iter = vertex_map.find(pp);
		if (iter != vertex_map.end())
		{
			return (iter->second);
		}
		return NULL;
	}

	const Iso_vertex *add_vertex(const Point_index_pair& pp, const Iso_vertex& vertex)
	{
		Iso_vertex *new_vertex = new Iso_vertex(vertex);
		vertex_map[pp] = new_vertex;
		return new_vertex;
	}
	
	/**
	 *      3
	 *     / \
	 *    /   \
	 *   1-----2
	 */
	void add_triangle(const Iso_vertex *v1,	const Iso_vertex *v2,
		const Iso_vertex *v3)
	{
		// ignore degenerate triangles
		if ((0.0 == square_distance3(v1->coordinates, v2->coordinates)) ||
			(0.0 == square_distance3(v2->coordinates, v3->coordinates)) ||
			(0.0 == square_distance3(v3->coordinates, v1->coordinates)))
		{
			return;
		}
		triangle_list.push_back(new Iso_triangle(v1, v2, v3));
	}

	void add_triangle(const Iso_vertex *v1,	const Iso_vertex *v2,
		const Iso_vertex *v3, bool reverse_winding)
	{
		if (reverse_winding)
		{
			add_triangle(v3, v2, v1);
		}
		else
		{
			add_triangle(v1, v2, v3);
		}
	}

	/**
	 *   4-----3
	 *   |     |
	 *   |     |
	 *   1-----2
	 */
	void add_quadrilateral(const Iso_vertex *v1,	const Iso_vertex *v2,
		const Iso_vertex *v3, const Iso_vertex *v4)
	{
		// cut quadrilateral across the shortest diagonal
		if (square_distance3(v1->coordinates, v3->coordinates) <
			square_distance3(v2->coordinates, v4->coordinates))
		{
			add_triangle(v1, v2, v3);
			add_triangle(v1, v3, v4);
		}
		else
		{
			add_triangle(v1, v2, v4);
			add_triangle(v2, v3, v4);
		}
	}

	void add_quadrilateral(const Iso_vertex *v1,	const Iso_vertex *v2,
		const Iso_vertex *v3, const Iso_vertex *v4, bool reverse_winding)
	{
		if (reverse_winding)
		{
			add_quadrilateral(v4, v3, v2, v1);
		}
		else
		{
			add_quadrilateral(v1, v2, v3, v4);
		}
	}

	/**
	 *   5----4
	 *   |     \
	 *   |      3
	 *   |     /
	 *   1----2
	 */
	void add_pentagon(const Iso_vertex *v1,	const Iso_vertex *v2,
		const Iso_vertex *v3, const Iso_vertex *v4, const Iso_vertex *v5)
	{
		// cut pentagon across the shortest diagonal
		FE_value distance13 = square_distance3(v1->coordinates, v3->coordinates);
		FE_value distance24 = square_distance3(v2->coordinates, v4->coordinates);
		FE_value distance35 = square_distance3(v3->coordinates, v5->coordinates);
		FE_value distance41 = square_distance3(v4->coordinates, v1->coordinates);
		FE_value distance52 = square_distance3(v5->coordinates, v2->coordinates);
		if ((distance13 < distance24) && (distance13 < distance35) &&
			(distance13 < distance41) && (distance13 < distance52))
		{
			add_triangle(v1, v2, v3);
			add_quadrilateral(v1, v3, v4, v5);
		}
		else if ((distance24 < distance35) && (distance24 < distance41) &&
			(distance24 < distance52))
		{
			add_triangle(v2, v3, v4);
			add_quadrilateral(v1, v2, v4, v5);
		}
		else if ((distance35 < distance41) && (distance35 < distance52))
		{
			add_triangle(v3, v4, v5);
			add_quadrilateral(v1, v2, v3, v5);
		}
		else if (distance41 < distance52)
		{
			add_triangle(v1, v4, v5);
			add_quadrilateral(v1, v2, v3, v4);
		}
		else
		{
			add_triangle(v1, v2, v5);
			add_quadrilateral(v2, v3, v4, v5);
		}
	}

	void add_pentagon(const Iso_vertex *v1,	const Iso_vertex *v2,
		const Iso_vertex *v3, const Iso_vertex *v4, const Iso_vertex *v5,
		bool reverse_winding)
	{
		if (reverse_winding)
		{
			add_pentagon(v5, v4, v3, v2, v1);
		}
		else
		{
			add_pentagon(v1, v2, v3, v4, v5);
		}
	}

	/**
	 *    5----4
	 *   /      \
	 *  6        3
	 *   \      /
	 *    1----2
	 */
	void add_hexagon(const Iso_vertex *v1,	const Iso_vertex *v2, const Iso_vertex *v3,
		const Iso_vertex *v4, const Iso_vertex *v5, const Iso_vertex *v6)
	{
		// cut hexagon into two quads across the shortest opposite diagonal
		FE_value distance14 = square_distance3(v1->coordinates, v4->coordinates);
		FE_value distance25 = square_distance3(v2->coordinates, v5->coordinates);
		FE_value distance36 = square_distance3(v3->coordinates, v6->coordinates);
		if ((distance14 < distance25) && (distance14 < distance36))
		{
			add_quadrilateral(v1, v2, v3, v4);
			add_quadrilateral(v1, v4, v5, v6);
		}
		else if (distance25 < distance36)
		{
			add_quadrilateral(v1, v2, v5, v6);
			add_quadrilateral(v2, v3, v4, v5);
		}
		else
		{
			add_quadrilateral(v1, v2, v3, v6);
			add_quadrilateral(v3, v4, v5, v6);
		}
	}

private:
	// declared but not defined so illegal:
	Iso_mesh(const Iso_mesh& in_Iso_mesh);

	// declared but not defined so illegal:
	void operator=(const Iso_mesh& in_Iso_mesh);
};

class Marching_cube
{
	unsigned char cases[256][10];

public:
	Marching_cube();
	
	unsigned char final_case(unsigned char unrotated_case)
	{
		return cases[unrotated_case][0];
	}
	
	unsigned char rotated_vertex(unsigned char unrotated_case, unsigned char unrotated_vertex)
	{
		return cases[unrotated_case][unrotated_vertex + 1];
	}
	
	bool inverse(unsigned char unrotated_case)
	{
		return (0 != cases[unrotated_case][9]);
	}
};

Marching_cube::Marching_cube()
{
	// all cube orientations = 4 rotations of 6 cube faces
	static const unsigned char cube_orientation[24][8] =
	{
			{ 0, 1, 2, 3, 4, 5, 6, 7 },
			{ 2, 3, 6, 7, 0, 1, 4, 5 },
			{ 6, 7, 4, 5, 2, 3, 0, 1 },
			{ 4, 5, 0, 1, 6, 7, 2, 3 },
			
			{ 3, 2, 1, 0, 7, 6, 5, 4 },
			{ 1, 0, 5, 4, 3, 2, 7, 6 },
			{ 5, 4, 7, 6, 1, 0, 3, 2 },
			{ 7, 6, 3, 2, 5, 4, 1, 0 },
			
			{ 1, 3, 0, 2, 5, 7, 4, 6 },
			{ 0, 2, 4, 6, 1, 3, 5, 7 },
			{ 4, 6, 5, 7, 0, 2, 1, 3 },
			{ 5, 7, 1, 3, 4, 6, 0, 2 },
			
			{ 2, 0, 3, 1, 6, 4, 7, 5 },
			{ 3, 1, 7, 5, 2, 0, 6, 4 },
			{ 7, 5, 6, 4, 3, 1, 2, 0 },
			{ 6, 4, 2, 0, 7, 5, 3, 1 },
			
			{ 0, 4, 1, 5, 2, 6, 3, 7 },
			{ 1, 5, 3, 7, 0, 4, 2, 6 },
			{ 3, 7, 2, 6, 1, 5, 0, 4 },
			{ 2, 6, 0, 4, 3, 7, 1, 5 },
			
			{ 5, 1, 4, 0, 7, 3, 6, 2 },
			{ 4, 0, 6, 2, 5, 1, 7, 3 },
			{ 6, 2, 7, 3, 4, 0, 5, 1 },
			{ 7, 3, 5, 1, 6, 2, 4, 0 }
	};
	// final crossing cases; second byte = 1 means also handle inverse
	static const unsigned char final_cases[14][2] =
	{
			{   1, 1 },
			{   3, 1 },
			{   7, 1 },
			{   9, 1 },
			{  15, 0 },
			{  23, 0 },
			{  39, 0 },
			{  41, 1 },
			{  67, 1 },
			{  71, 0 },
			{ 105, 0 },
			{ 129, 1 },
			{ 135, 0 },
			{ 195, 0 }
	};
	static const unsigned char power2[8] =	{ 1, 2, 4, 8, 16, 32, 64, 128 };
	
	ENTER(Marching_cube::Marching_cube);
	memset(cases, 0, sizeof(cases));
	for (int f = 0; f < 14; f++)
	{
		unsigned char final_case = final_cases[f][0];
		for (unsigned char inverse = 0; inverse <= final_cases[f][1]; inverse++)
		{
			unsigned char test_case = inverse ? ( final_case ^ 0xFF) : final_case;
			for (unsigned char orient = 0; orient < 24; orient++)
			{
				unsigned char unrotated_case = 0;
				unsigned char bit = 1;
				unsigned char point;
				for (point = 0; point < 8; point++)
				{
					if (test_case & bit)
					{
						unrotated_case += power2[cube_orientation[orient][point]];
					}
					bit <<= 1;
				}
				if (0 == cases[unrotated_case][0])
				{
					cases[unrotated_case][0] = final_case;
					for (point = 0; point < 8; point++)
					{
						cases[unrotated_case][point+1] = cube_orientation[orient][point];
					}
					cases[unrotated_case][9] = inverse;
				}
			}
		}
	}
#if defined (TEST_CODE)
	display_message(INFORMATION_MESSAGE, "In Marching_cube::Marching_cube\n");
	unsigned char final_case_count[14][2];
	memset(final_case_count, 0, sizeof(final_case_count));
	for (int i = 0; i < 256; i++)
	{
		for (int j = 0; j < 14; j++)
		{
			if (cases[i][0] == final_cases[j][0])
			{
				++(final_case_count[j][ cases[i][9] ]);
			}
		}
		display_message(INFORMATION_MESSAGE, "case[%d] -> %d  : %d %d %d %d %d %d %d %d : %d\n",
			i,
			cases[i][0], cases[i][1], cases[i][2], cases[i][3], cases[i][4],
			cases[i][5], cases[i][6], cases[i][7], cases[i][8], cases[i][9]);
	}
	display_message(INFORMATION_MESSAGE, "\nCube case summary:\n");
	int total_cases = 2;
	for (int j = 0; j < 14; j++)
	{
		display_message(INFORMATION_MESSAGE, "case[%3d] : %2d , %2d\n",
			final_cases[j][0], final_case_count[j][0], final_case_count[j][1]);
		total_cases += final_case_count[j][0] + final_case_count[j][1];
	}
	display_message(INFORMATION_MESSAGE, "Total cases = %d\n", total_cases);
#endif /* defined (TEST_CODE) */
	LEAVE;
}

typedef std::map<int, Iso_mesh*> Iso_mesh_map;
typedef std::map<int, Iso_mesh*>::iterator Iso_mesh_map_iterator;
typedef std::map<int, Iso_mesh*>::const_iterator Iso_mesh_map_const_iterator;

class Isosurface_builder
{
private:
	FE_element *element;
	FE_value time;
	int number_in_xi1, number_in_xi2, number_in_xi3, number_of_polygon_vertices;
	const Iso_surface_specification& specification;
	FE_value delta_xi1, delta_xi2, delta_xi3;
	const int number_of_iso_values;
	int current_iso_value_number;
	double current_iso_value;
	Computed_field *coordinate_field, *data_field, *scalar_field,
		*texture_coordinate_field;
	int plane_size;
	double *plane_scalars;
	bool cube, polygon12, polygon13, polygon23, simplex12, simplex13, simplex23,
		tetrahedron;
	Iso_mesh_map mesh_map;
	int last_mesh_number;
	Iso_mesh *last_mesh;

public:
	Isosurface_builder(FE_element *element, FE_value time,
		int requested_number_in_xi1, int requested_number_in_xi2, int requested_number_in_xi3,
		const Iso_surface_specification& specification);

	~Isosurface_builder();

	int sweep();

	int fill_graphics(struct GT_object *graphics_object,
		enum Render_type render_type);

private:

	Iso_mesh& get_mesh()
	{
		if (current_iso_value_number != last_mesh_number)
		{
			last_mesh_number = current_iso_value_number;
			Iso_mesh_map_iterator iter = mesh_map.find(current_iso_value_number);
			if (iter != mesh_map.end())
			{
				last_mesh = iter->second;				
			}
			else
			{
				last_mesh = new Iso_mesh();
				mesh_map[last_mesh_number] = last_mesh;
			}
		}
		return *last_mesh;
	}
	
	const Iso_vertex *compute_line_crossing(const Point_index_pair& pp);

	const Iso_vertex *get_line_crossing(const Point_index_pair& pp)
	{
		const Iso_vertex *vertex = get_mesh().get_vertex(pp);
		if (NULL == vertex)
		{
			vertex = compute_line_crossing(pp);
		}
		return (vertex);
	}

	double get_scalar(const Point_index& p) const
	{
		if (p.exact_xi)
		{
			return p.scalar;
		}
		return plane_scalars[(p.k & 1)*plane_size + p.j*(number_in_xi1 + 1) + p.i];
	}

	void set_scalar(int i, int j, int k, double scalar_value)
	{
		plane_scalars[(k & 1)*plane_size + j*(number_in_xi1 + 1) + i] = scalar_value;
	}
	
	/***************************************************************************//**
	 * Get the mean scalar value of four points, used to get the interpolated scalar
	 * value at the centre of a quadrilateral. Adds scalars from maximum to minimum
	 * so result is identical for different ordering of the same points, needed to
	 * ensure same ambiguity resolution case used for neighbouring faces, where even
	 * small numerical rounding differences can leave holes in surface.
	 * @return  true if the mean scalar is greater than the iso_value; 
	 */
	bool ambiguous_quad_resolves_over(const Point_index& p1, const Point_index& p2,
		const Point_index& p3, const Point_index& p4)
	{
		double s1 = get_scalar(p1);
		double s2 = get_scalar(p2);
		double s3 = get_scalar(p3);
		double s4 = get_scalar(p4);
		if (s4 > s3)
		{
			double temp = s3;
			s3 = s4;
			s4 = temp;
		}
		if (s3 > s2)
		{
			double temp = s2;
			s2 = s3;
			s3 = temp;
		}
		if (s2 > s1)
		{
			double temp = s1;
			s1 = s2;
			s2 = temp;
		}
		if (s4 > s3)
		{
			double temp = s3;
			s3 = s4;
			s4 = temp;
		}
		if (s3 > s2)
		{
			double temp = s2;
			s2 = s3;
			s3 = temp;
		}
		if (s4 > s3)
		{
			double temp = s3;
			s3 = s4;
			s4 = temp;
		}
		return ((s1 + s2 + s3 + s4)*0.25 > current_iso_value);
	}

	void get_xi(const Point_index& p, FE_value *xi) const
	{
		if (p.exact_xi)
		{
			xi[0] = p.xi1;
			xi[1] = p.xi2;
			xi[2] = p.xi3;
		}
		else
		{
			xi[0] = p.i * delta_xi1;
			xi[1] = p.j * delta_xi2;
			xi[2] = p.k * delta_xi3;
		}
	}

	void get_cell_centre_xi(int i, int j, int k, FE_value *xi) const
	{
		xi[0] = (i + 0.5) * delta_xi1;
		xi[1] = (j + 0.5) * delta_xi2;
		xi[2] = (k + 0.5) * delta_xi3;
	}

	void cross_tetrahedron(const Point_index& p1,
		const Point_index& p2, const Point_index& p3, const Point_index& p4);

	int cross_cube(int i, int j, int k);

	int cross_octahedron(int i, int j, int k);

	int cross_pyramid(
		const Point_index& p0, const Point_index& p1, const Point_index& p2,
		const Point_index& p3, const Point_index& p4);

	int cross_triangle_wedge(
		const Point_index& p0, const Point_index& p1, const Point_index& p2,
		const Point_index& p3, const Point_index& p4, const Point_index& p5);

};

Isosurface_builder::Isosurface_builder(FE_element *element, FE_value time,
	int requested_number_in_xi1, int requested_number_in_xi2, int requested_number_in_xi3,
	const Iso_surface_specification& specification) :
		element(element), time(time),
		number_in_xi1(requested_number_in_xi1),
		number_in_xi2(requested_number_in_xi2),
		number_in_xi3(requested_number_in_xi3),
		number_of_polygon_vertices(1),
		specification(specification),
		number_of_iso_values(specification.number_of_iso_values),
		coordinate_field(specification.coordinate_field),
		data_field(specification.data_field),
		scalar_field(specification.scalar_field),
		texture_coordinate_field(specification.texture_coordinate_field),
		last_mesh_number(-1)
{
	enum FE_element_shape_type shape_type1, shape_type2, shape_type3;
	FE_element_shape *element_shape = NULL;
	get_FE_element_shape(element, &element_shape);
	get_FE_element_shape_xi_shape_type(element_shape, /*xi_number*/0, &shape_type1);
	get_FE_element_shape_xi_shape_type(element_shape, /*xi_number*/1, &shape_type2);
	get_FE_element_shape_xi_shape_type(element_shape, /*xi_number*/2, &shape_type3);
	
	simplex12 = (SIMPLEX_SHAPE == shape_type1) && (SIMPLEX_SHAPE == shape_type2);
	simplex13 = (SIMPLEX_SHAPE == shape_type3) && (SIMPLEX_SHAPE == shape_type1);
	simplex23 = (SIMPLEX_SHAPE == shape_type2) && (SIMPLEX_SHAPE == shape_type3);
	tetrahedron = simplex12 && simplex23;
	polygon12 = (POLYGON_SHAPE == shape_type1) && (POLYGON_SHAPE == shape_type2);
	polygon13 = (POLYGON_SHAPE == shape_type1) && (POLYGON_SHAPE == shape_type3);
	polygon23 = (POLYGON_SHAPE == shape_type2) && (POLYGON_SHAPE == shape_type3);
	cube = (!simplex12) && (!simplex13) && (!simplex23);

	if (simplex12)
	{
		number_in_xi2 = number_in_xi1;
	}
	if (simplex13)
	{
		number_in_xi3 = number_in_xi1;
	}
	if (simplex23)
	{
		number_in_xi3 = number_in_xi2;
	}

	if (polygon12 || polygon13 || polygon23)
	{
		int linked_xi1 = (polygon12 || polygon13) ? 0 : 1;
		int linked_xi2 = polygon12 ? 1 : 2;
		get_FE_element_shape_xi_linkage_number(element_shape,
			linked_xi1, linked_xi2, &number_of_polygon_vertices);
		if (0 == linked_xi1)
		{
			number_in_xi1 = (1 + (number_in_xi1 - 1)/number_of_polygon_vertices)
				* number_of_polygon_vertices;
			// better: number_in_xi1 *= number_of_polygon_vertices;
		}
		else
		{
			number_in_xi2 = (1 + (number_in_xi2 - 1)/number_of_polygon_vertices)
				* number_of_polygon_vertices;
			// better: number_in_xi2 *= number_of_polygon_vertices;
		}
	}
	
	// precalculate scalings from index to xi
	delta_xi1 = 1.0 / number_in_xi1;
	delta_xi2 = 1.0 / number_in_xi2;
	delta_xi3 = 1.0 / number_in_xi3;
	plane_size = (number_in_xi1 + 1)*(number_in_xi2 + 1);
	plane_scalars = new double[2*plane_size];
}

Isosurface_builder::~Isosurface_builder()
{
	ENTER(Isosurface_builder::~Isosurface_builder);
	for (Iso_mesh_map_iterator mesh_iter = mesh_map.begin(); mesh_iter != mesh_map.end(); mesh_iter++)
	{
		Iso_mesh *mesh = mesh_iter->second;
		delete mesh;
	}
	delete[] plane_scalars;
	Computed_field_clear_cache(coordinate_field);
	Computed_field_clear_cache(scalar_field);
	if (data_field)
	{
		Computed_field_clear_cache(data_field);
	}
	if (texture_coordinate_field)
	{
		Computed_field_clear_cache(texture_coordinate_field);
	}
	LEAVE;
}

const Iso_vertex *Isosurface_builder::compute_line_crossing(
	const Point_index_pair& pp)
{
	ENTER(Isosurface_builder::compute_line_crossing);
	Iso_vertex vertex;
	double scalar_a = get_scalar(pp.pa);
	double scalar_b = get_scalar(pp.pb);
	double r = (current_iso_value - scalar_a) / (scalar_b - scalar_a);
	double inverse_r = 1.0 - r;
	FE_value xi_a[3], xi_b[3];
	get_xi(pp.pa, xi_a);
	get_xi(pp.pb, xi_b);
	vertex.xi[0] = xi_a[0]*inverse_r + xi_b[0]*r;
	vertex.xi[1] = xi_a[1]*inverse_r + xi_b[1]*r;
	vertex.xi[2] = xi_a[2]*inverse_r + xi_b[2]*r;
	// future: option to iterate to get exact xi crossing for non-linear fields
	Computed_field_evaluate_in_element(coordinate_field, element, vertex.xi,
		time, /*top_level_element*/(struct FE_element *)NULL, vertex.coordinates,
		/*derivatives*/(FE_value *)NULL);
	if (NULL != texture_coordinate_field)
	{
		Computed_field_evaluate_in_element(texture_coordinate_field, element, vertex.xi,
			time, /*top_level_element*/(struct FE_element *)NULL, vertex.texture_coordinates,
			/*derivatives*/(FE_value *)NULL);
	}
	const Iso_vertex* new_vertex = get_mesh().add_vertex(pp, vertex);
	LEAVE;
	
	return (new_vertex);
}

/***************************************************************************//**
 *
 * @param p0..p3  Point index locations of tetrahedral vertices with winding: 
 * 
 *       4---3
 *      / \.'|
 *     / .'\ |
 *    /.'   \|
 *   1-------2
 * 
 * @return  1 on success, 0 on failure
 */
void Isosurface_builder::cross_tetrahedron(const Point_index& p1,
	const Point_index& p2, const Point_index& p3, const Point_index& p4)
{
	// first column is final case = number of points > iso_value, or 0 for no crossings
	// columns 1-4 are indexes of initial vertices in order to rotate to final case:
	// 0 : ignore
	// 1 : p1 > iso_value
	// 2 : p1, p2 > iso_value
	// 3 : p1, p2, p3 > iso_value
	static const int tet_case[16][5] =
	{
		{ 0, 0, 0, 0, 0 }, // [nothing]   > iso_value
		{ 1, 0, 1, 2, 3 }, // p1          > iso_value
		{ 1, 1, 0, 3, 2 }, //    p2       > iso_value
		{ 3, 0, 1, 2, 3 }, // p1 p2       > iso_value
		{ 1, 2, 0, 1, 3 }, //       p3    > iso_value
		{ 3, 0, 2, 3, 1 }, // p1    p3    > iso_value
		{ 3, 1, 2, 0, 3 }, //    p2 p3    > iso_value
		{ 7, 0, 1, 2, 3 }, // p1 p2 p3    > iso_value
		{ 1, 3, 0, 2, 1 }, //          p4 > iso_value
		{ 3, 0, 3, 1, 2 }, // p1       p4 > iso_value
		{ 3, 1, 3, 2, 0 }, //    p2    p4 > iso_value
		{ 7, 0, 3, 1, 2 }, // p1 p2    p4 > iso_value
		{ 3, 2, 3, 0, 1 }, //       p3 p4 > iso_value
		{ 7, 0, 2, 3, 1 }, // p1    p3 p4 > iso_value
		{ 7, 1, 3, 2, 0 }, //    p2 p3 p4 > iso_value
		{ 0, 0, 0, 0, 0 }, // p1 p2 p3 p4 > iso_value
	}
	
	ENTER(Isosurface_builder::cross_tetrahedron);
	int unrotated_case =	
		((get_scalar(p1) > current_iso_value) ? 1 : 0) +
		((get_scalar(p2) > current_iso_value) ? 2 : 0) +
		((get_scalar(p3) > current_iso_value) ? 4 : 0) +
		((get_scalar(p4) > current_iso_value) ? 8 : 0);
	int final_case = tet_case[unrotated_case][0];
	if (0 < final_case)
	{
		Point_index mp[4] = { p1, p2, p3, p4 };
		Point_index mp1 = mp[tet_case[unrotated_case][1]];
		Point_index mp2 = mp[tet_case[unrotated_case][2]];
		Point_index mp3 = mp[tet_case[unrotated_case][3]];
		Point_index mp4 = mp[tet_case[unrotated_case][4]];
		const Iso_vertex *v1, *v2, *v3, *v4;
		Iso_mesh& mesh = get_mesh();
		switch (final_case)
		{
		case 1:
			v1 = get_line_crossing(Point_index_pair(mp1, mp2));
			v2 = get_line_crossing(Point_index_pair(mp1, mp3));
			v3 = get_line_crossing(Point_index_pair(mp1, mp4));
			mesh.add_triangle(v1,v3,v2);
			break;
		case 3:
			// 1, 2 > iso_value 
			v1 = get_line_crossing(Point_index_pair(mp1, mp4));
			v2 = get_line_crossing(Point_index_pair(mp1, mp3));
			v3 = get_line_crossing(Point_index_pair(mp2, mp3));
			v4 = get_line_crossing(Point_index_pair(mp2, mp4));
			mesh.add_quadrilateral(v1, v2, v3, v4);
			break;
		case 7:
			v1 = get_line_crossing(Point_index_pair(mp1, mp4));
			v2 = get_line_crossing(Point_index_pair(mp2, mp4));
			v3 = get_line_crossing(Point_index_pair(mp3, mp4));
			mesh.add_triangle(v1,v3,v2);
			break;
		}
	}
	LEAVE;
}

static Marching_cube mcube;

inline int Isosurface_builder::cross_cube(int i, int j, int k)
{
	ENTER(Isosurface_builder::cross_cube);
	int return_code = 1;
	Point_index p[8];
	p[0].set(i    , j    , k    );
	p[1].set(i + 1, j    , k    );
	p[2].set(i    , j + 1, k    );
	p[3].set(i + 1, j + 1, k    );
	p[4].set(i    , j    , k + 1);
	p[5].set(i + 1, j    , k + 1);
	p[6].set(i    , j + 1, k + 1);
	p[7].set(i + 1, j + 1, k + 1);
	int unrotated_case =	
		((get_scalar(p[0]) > current_iso_value) ?   1 : 0) +
		((get_scalar(p[1]) > current_iso_value) ?   2 : 0) +
		((get_scalar(p[2]) > current_iso_value) ?   4 : 0) +
		((get_scalar(p[3]) > current_iso_value) ?   8 : 0) +
		((get_scalar(p[4]) > current_iso_value) ?  16 : 0) +
		((get_scalar(p[5]) > current_iso_value) ?  32 : 0) +
		((get_scalar(p[6]) > current_iso_value) ?  64 : 0) +
		((get_scalar(p[7]) > current_iso_value) ? 128 : 0);
	int final_case = mcube.final_case(unrotated_case);
	if (0 < final_case)
	{
		bool inverse = mcube.inverse(unrotated_case);
		Point_index mp0 = p[mcube.rotated_vertex(unrotated_case, 0)];
		Point_index mp1 = p[mcube.rotated_vertex(unrotated_case, 1)];
		Point_index mp2 = p[mcube.rotated_vertex(unrotated_case, 2)];
		Point_index mp3 = p[mcube.rotated_vertex(unrotated_case, 3)];
		Point_index mp4 = p[mcube.rotated_vertex(unrotated_case, 4)];
		Point_index mp5 = p[mcube.rotated_vertex(unrotated_case, 5)];
		Point_index mp6 = p[mcube.rotated_vertex(unrotated_case, 6)];
		Point_index mp7 = p[mcube.rotated_vertex(unrotated_case, 7)];
		const Iso_vertex *v1, *v2, *v3, *v4, *v5, *v6, *v7, *v8;
		Iso_mesh& mesh = get_mesh();
		switch (final_case)
		{
		case 1:
			// 0 > iso_value, with inverse
			v1 = get_line_crossing(Point_index_pair(mp0, mp1));
			v2 = get_line_crossing(Point_index_pair(mp0, mp4));
			v3 = get_line_crossing(Point_index_pair(mp0, mp2));
			mesh.add_triangle(v1, v2, v3, inverse);
			break;
		case 3:
			// 0,1 > iso_value, with inverse
			v1 = get_line_crossing(Point_index_pair(mp0, mp4));
			v2 = get_line_crossing(Point_index_pair(mp0, mp2));
			v3 = get_line_crossing(Point_index_pair(mp1, mp3));
			v4 = get_line_crossing(Point_index_pair(mp1, mp5));
			mesh.add_quadrilateral(v1, v2, v3, v4, inverse);
			break;
		case 7:
			// 0,1,2 > iso_value, with inverse
			v1 = get_line_crossing(Point_index_pair(mp0, mp4));
			v2 = get_line_crossing(Point_index_pair(mp2, mp6));
			v3 = get_line_crossing(Point_index_pair(mp2, mp3));
			v4 = get_line_crossing(Point_index_pair(mp1, mp3));
			v5 = get_line_crossing(Point_index_pair(mp1, mp5));
			mesh.add_pentagon(v1, v2, v3, v4, v5, inverse);
			break;
		case 9: // 0,3 > iso_value, with inverse
			v1 = get_line_crossing(Point_index_pair(mp0, mp1));
			v2 = get_line_crossing(Point_index_pair(mp0, mp4));
			v3 = get_line_crossing(Point_index_pair(mp0, mp2));
			v4 = get_line_crossing(Point_index_pair(mp3, mp1));
			v5 = get_line_crossing(Point_index_pair(mp3, mp2));
			v6 = get_line_crossing(Point_index_pair(mp3, mp7));
			if (ambiguous_quad_resolves_over(mp0, mp1, mp2, mp3) != inverse)
			{
				mesh.add_quadrilateral(v1, v2, v6, v4, inverse);
				mesh.add_quadrilateral(v2, v3, v5, v6, inverse);
			}
			else
			{
				mesh.add_triangle(v1, v2, v3, inverse);
				mesh.add_triangle(v4, v5, v6, inverse);
			}
			break;
		case 15: // 0,1,2,3 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp0, mp4));
			v2 = get_line_crossing(Point_index_pair(mp2, mp6));
			v3 = get_line_crossing(Point_index_pair(mp3, mp7));
			v4 = get_line_crossing(Point_index_pair(mp1, mp5));
			mesh.add_quadrilateral(v1, v2, v3, v4);
			break;
		case 23: // 0,1,2,4 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp1, mp3));
			v2 = get_line_crossing(Point_index_pair(mp1, mp5));
			v3 = get_line_crossing(Point_index_pair(mp4, mp5));
			v4 = get_line_crossing(Point_index_pair(mp4, mp6));
			v5 = get_line_crossing(Point_index_pair(mp2, mp6));
			v6 = get_line_crossing(Point_index_pair(mp2, mp3));
			mesh.add_hexagon(v1, v2, v3, v4, v5, v6);
			break;
		case 39: // 0,1,2,5 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp0, mp4));
			v2 = get_line_crossing(Point_index_pair(mp2, mp6));
			v3 = get_line_crossing(Point_index_pair(mp2, mp3));
			v4 = get_line_crossing(Point_index_pair(mp1, mp3));
			v5 = get_line_crossing(Point_index_pair(mp5, mp7));
			v6 = get_line_crossing(Point_index_pair(mp5, mp4));
			mesh.add_hexagon(v1, v2, v3, v4, v5, v6);
			break;
		case 67: // 0,1,6 > iso_value, with inverse
			v1 = get_line_crossing(Point_index_pair(mp0, mp4));
			v2 = get_line_crossing(Point_index_pair(mp0, mp2));
			v3 = get_line_crossing(Point_index_pair(mp1, mp3));
			v4 = get_line_crossing(Point_index_pair(mp1, mp5));
			v5 = get_line_crossing(Point_index_pair(mp6, mp2));
			v6 = get_line_crossing(Point_index_pair(mp6, mp4));
			v7 = get_line_crossing(Point_index_pair(mp6, mp7));
			if (ambiguous_quad_resolves_over(mp0, mp2, mp4, mp6) != inverse)
			{
				// extra interior point:
				v8 = get_line_crossing(Point_index_pair(mp0, mp7));
				mesh.add_triangle(v8, v4, v1, inverse);
				mesh.add_triangle(v8, v3, v4, inverse);
				mesh.add_triangle(v8, v2, v3, inverse);
				mesh.add_quadrilateral(v8, v1, v6, v7, inverse);
				mesh.add_quadrilateral(v8, v7, v5, v2, inverse);
			}
			else
			{
				mesh.add_quadrilateral(v1, v2, v3, v4, inverse);
				mesh.add_triangle(v5, v6, v7, inverse);
			}
			break;
		case 71: // 0,1,2,6 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp0, mp4));
			v2 = get_line_crossing(Point_index_pair(mp6, mp4));
			v3 = get_line_crossing(Point_index_pair(mp6, mp7));
			v4 = get_line_crossing(Point_index_pair(mp2, mp3));
			v5 = get_line_crossing(Point_index_pair(mp1, mp3));
			v6 = get_line_crossing(Point_index_pair(mp1, mp5));
			mesh.add_hexagon(v1, v2, v3, v4, v5, v6);
			break;
		case 129: // 0,7 > iso_value, with inverse
			v1 = get_line_crossing(Point_index_pair(mp0, mp1));
			v2 = get_line_crossing(Point_index_pair(mp0, mp4));
			v3 = get_line_crossing(Point_index_pair(mp0, mp2));
			v4 = get_line_crossing(Point_index_pair(mp7, mp3));
			v5 = get_line_crossing(Point_index_pair(mp7, mp6));
			v6 = get_line_crossing(Point_index_pair(mp7, mp5));
			mesh.add_triangle(v1, v2, v3, inverse);
			mesh.add_triangle(v4, v5, v6, inverse);
			break;
		case 41: // 0,3,5 > iso_value
		case 105: // 0,3,5,6 > iso_value
		case 135: // 0,1,2,7 > iso_value
		case 195: // 0,1,6,7 > iso_value
		{
			// Cases with multiple ambiguous double contour faces:
			// divide into 6 pyramids with new centre point sample
			FE_value xi_c[3], scalar_FE_value;
			get_cell_centre_xi(i, j, k, xi_c);
			Computed_field_evaluate_in_element(scalar_field, element, xi_c,
				time, /*top_level_element*/(struct FE_element *)NULL, &scalar_FE_value,
				/*derivatives*/(FE_value *)NULL);
			Point_index pc(xi_c,  static_cast<double>(scalar_FE_value));
			cross_pyramid(p[0], p[2], p[4], p[6], pc);
			cross_pyramid(p[3], p[1], p[7], p[5], pc);
			cross_pyramid(p[1], p[0], p[5], p[4], pc);
			cross_pyramid(p[2], p[3], p[6], p[7], pc);
			cross_pyramid(p[0], p[1], p[2], p[3], pc);
			cross_pyramid(p[5], p[4], p[7], p[6], pc);
			break;
		}
		default:
			display_message(ERROR_MESSAGE,
				"Isosurface_builder::cross_cube.  Unknown case %d (unrotated %d)", final_case, unrotated_case);
			return_code = 0;
			break;
		}
	}
	LEAVE;

	return (return_code);
}

int Isosurface_builder::cross_octahedron(int i, int j, int k)
{
	// first column is final case in standard orientation
	// columns 1-6 are indexes of initial vertices in order to rotate to final case
	static const int oct_case[64][7] =
	{
		{  0, 0, 0, 0, 0, 0, 0 }, //  0
		{  1, 0, 1, 2, 3, 4, 5 }, //  1 : 0 over = standard case
		{  1, 1, 2, 0, 5, 3, 4 }, //  2
		{  3, 0, 1, 2, 3, 4, 5 }, //  3 : 0,1 over (adj) = standard case
		{  1, 2, 0, 1, 4, 5, 3 }, //  4
		{  3, 0, 2, 4, 1, 3, 5 }, //  5 
		{  3, 1, 2, 0, 5, 3, 4 }, //  6
		{  7, 0, 1, 2, 3, 4, 5 }, //  7 : 0,1,2 over (adj) = standard case
		{  1, 3, 1, 0, 5, 4, 2 }, //  8
		{  3, 0, 3, 1, 4, 2, 5 }, //  9
		{  3, 1, 3, 5, 0, 2, 4 }, // 10
		{  7, 0, 3, 1, 4, 2, 5 }, // 11
		{ 33, 2, 0, 1, 4, 5, 3 }, // 12
		{ 35, 2, 0, 1, 4, 5, 3 }, // 13
		{ 35, 2, 1, 5, 0, 4, 3 }, // 14
		{ 15, 0, 1, 2, 3, 4, 5 }, // 15 : 0,1,2,3 over (adj) = standard case
		{  1, 4, 0, 2, 3, 5, 1 }, // 16
		{  3, 0, 4, 3, 2, 1, 5 }, // 17
		{ 33, 1, 2, 0, 5, 3, 4 }, // 18
		{ 35, 1, 0, 3, 2, 5, 4 }, // 19
		{  3, 2, 4, 0, 5, 1, 3 }, // 20
		{  7, 0, 2, 4, 1, 3, 5 }, // 21
		{ 35, 1, 2, 0, 5, 3, 4 }, // 22
		{ 15, 2, 0, 1, 4, 5, 3 }, // 23
		{  3, 3, 4, 5, 0, 1, 2 }, // 24
		{  7, 0, 4, 3, 2, 1, 5 }, // 25
		{ 35, 1, 3, 5, 0, 2, 4 }, // 26
		{ 15, 0, 3, 1, 4, 2, 5 }, // 27
		{ 35, 2, 4, 0, 5, 1, 3 }, // 28
		{ 15, 4, 0, 2, 3, 5, 1 }, // 29
		{ 51, 1, 2, 0, 5, 3, 4 }, // 30 
		{ 31, 0, 1, 2, 3, 4, 5 }, // 31 : 0,1,2,3,4 over = standard case
		{  1, 5, 2, 1, 4, 3, 0 }, // 32
		{ 33, 0, 1, 2, 3, 4, 5 }, // 33 : 0,5 over (opp) = standard case
		{  3, 1, 5, 2, 3, 0, 4 }, // 34
		{ 35, 0, 1, 2, 3, 4, 5 }, // 35 : 0,1,5 over (opp) = standard case
		{  3, 2, 5, 4, 1, 0, 3 }, // 36
		{ 35, 0, 2, 4, 1, 3, 5 }, // 37
		{  7, 1, 5, 2, 3, 0, 4 }, // 38
		{ 15, 2, 1, 5, 0, 4, 3 }, // 39
		{  3, 3, 5, 1, 4, 0, 2 }, // 40
		{ 35, 0, 3, 1, 4, 2, 5 }, // 41
		{  7, 1, 3, 5, 0, 2, 4 }, // 42
		{ 15, 3, 1, 0, 5, 4, 2 }, // 43
		{ 35, 2, 5, 4, 1, 0, 3 }, // 44
		{ 51, 0, 2, 4, 1, 3, 5 }, // 45  
		{ 15, 1, 5, 2, 3, 0, 4 }, // 46
		{ 31, 1, 2, 0, 5, 3, 4 }, // 47
		{  3, 4, 5, 3, 2, 0, 1 }, // 48
		{ 35, 0, 4, 3, 2, 1, 5 }, // 49
		{ 35, 1, 5, 2, 3, 0, 4 }, // 50
		{ 51, 0, 1, 2, 3, 4, 5 }, // 51 : 0,1,4,5 over (opp) = standard case
		{  7, 2, 5, 4, 1, 0, 3 }, // 52
		{ 15, 2, 4, 0, 5, 1, 3 }, // 53
		{ 15, 2, 5, 4, 1, 0, 3 }, // 54
		{ 31, 2, 0, 1, 4, 5, 3 }, // 55
		{  7, 3, 4, 5, 0, 1, 2 }, // 56
		{ 15, 4, 3, 0, 5, 2, 1 }, // 57
		{ 15, 5, 3, 4, 1, 2, 0 }, // 58
		{ 31, 3, 1, 0, 5, 4, 2 }, // 59
		{ 15, 4, 5, 3, 2, 0, 1 }, // 60
		{ 31, 4, 0, 2, 3, 5, 1 }, // 61
		{ 31, 5, 1, 3, 2, 4, 0 }, // 62
		{  0, 0, 0, 0, 0, 0, 0 }  // 63
	};

	ENTER(Isosurface_builder::cross_octahedron);
	int return_code = 1;
	Point_index p[6];
	p[0].set(i + 1, j    , k    );
	p[1].set(i    , j + 1, k    );
	p[2].set(i + 1, j + 1, k    );
	p[3].set(i    , j    , k + 1);
	p[4].set(i + 1, j    , k + 1);
	p[5].set(i    , j + 1, k + 1);
	int unrotated_case =	
		((get_scalar(p[0]) > current_iso_value) ?  1 : 0) +
		((get_scalar(p[1]) > current_iso_value) ?  2 : 0) +
		((get_scalar(p[2]) > current_iso_value) ?  4 : 0) +
		((get_scalar(p[3]) > current_iso_value) ?  8 : 0) +
		((get_scalar(p[4]) > current_iso_value) ? 16 : 0) +
		((get_scalar(p[5]) > current_iso_value) ? 32 : 0);
	int final_case = oct_case[unrotated_case][0];
	if (0 < final_case)
	{
		Point_index mp0 = p[oct_case[unrotated_case][1]];
		Point_index mp1 = p[oct_case[unrotated_case][2]];
		Point_index mp2 = p[oct_case[unrotated_case][3]];
		Point_index mp3 = p[oct_case[unrotated_case][4]];
		Point_index mp4 = p[oct_case[unrotated_case][5]];
		Point_index mp5 = p[oct_case[unrotated_case][6]];
		const Iso_vertex *v1, *v2, *v3, *v4, *v5, *v6;
		Iso_mesh& mesh = get_mesh();
		switch (final_case)
		{
		case 1:
			// 0 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp0, mp1));
			v2 = get_line_crossing(Point_index_pair(mp0, mp2));
			v3 = get_line_crossing(Point_index_pair(mp0, mp4));
			v4 = get_line_crossing(Point_index_pair(mp0, mp3));
			mesh.add_quadrilateral(v1, v2, v3, v4);
			break;
		case 3:
			// 0,1 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp0, mp2));
			v2 = get_line_crossing(Point_index_pair(mp0, mp4));
			v3 = get_line_crossing(Point_index_pair(mp0, mp3));
			v4 = get_line_crossing(Point_index_pair(mp1, mp3));
			v5 = get_line_crossing(Point_index_pair(mp1, mp5));
			v6 = get_line_crossing(Point_index_pair(mp1, mp2));
			mesh.add_hexagon(v1, v2, v3, v4, v5, v6);
			break;
		case 7:
			// 0,1,2 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp0, mp4));
			v2 = get_line_crossing(Point_index_pair(mp0, mp3));
			v3 = get_line_crossing(Point_index_pair(mp1, mp3));
			v4 = get_line_crossing(Point_index_pair(mp1, mp5));
			v5 = get_line_crossing(Point_index_pair(mp2, mp5));
			v6 = get_line_crossing(Point_index_pair(mp2, mp4));
			mesh.add_hexagon(v1, v2, v3, v4, v5, v6);
			break;
		case 15:
			// 0,1,2,3 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp0, mp4));
			v2 = get_line_crossing(Point_index_pair(mp3, mp4));
			v3 = get_line_crossing(Point_index_pair(mp3, mp5));
			v4 = get_line_crossing(Point_index_pair(mp1, mp5));
			v5 = get_line_crossing(Point_index_pair(mp2, mp5));
			v6 = get_line_crossing(Point_index_pair(mp2, mp4));
			mesh.add_hexagon(v1, v2, v3, v4, v5, v6);
			break;
		case 31:
			// 0,1,2,3,4 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp5, mp1));
			v2 = get_line_crossing(Point_index_pair(mp5, mp2));
			v3 = get_line_crossing(Point_index_pair(mp5, mp4));
			v4 = get_line_crossing(Point_index_pair(mp5, mp3));
			mesh.add_quadrilateral(v1, v2, v3, v4);
			break;
		case 33:
			// 0,5 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp0, mp1));
			v2 = get_line_crossing(Point_index_pair(mp0, mp2));
			v3 = get_line_crossing(Point_index_pair(mp0, mp4));
			v4 = get_line_crossing(Point_index_pair(mp0, mp3));
			mesh.add_quadrilateral(v1, v2, v3, v4);
			v1 = get_line_crossing(Point_index_pair(mp5, mp1));
			v2 = get_line_crossing(Point_index_pair(mp5, mp3));
			v3 = get_line_crossing(Point_index_pair(mp5, mp4));
			v4 = get_line_crossing(Point_index_pair(mp5, mp2));
			mesh.add_quadrilateral(v1, v2, v3, v4);
			break;
		case 35:
			// 0,1,5 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp0, mp2));
			v2 = get_line_crossing(Point_index_pair(mp0, mp4));
			v3 = get_line_crossing(Point_index_pair(mp0, mp3));
			v4 = get_line_crossing(Point_index_pair(mp1, mp3));
			v5 = get_line_crossing(Point_index_pair(mp1, mp2));
			mesh.add_pentagon(v1, v2, v3, v4, v5);
			v1 = v5;
			v2 = v4;
			v3 = get_line_crossing(Point_index_pair(mp5, mp3));
			v4 = get_line_crossing(Point_index_pair(mp5, mp4));
			v5 = get_line_crossing(Point_index_pair(mp5, mp2));
			mesh.add_pentagon(v1, v2, v3, v4, v5);
			break;
		case 51:
			// 0,1,4,5 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp2, mp0));
			v2 = get_line_crossing(Point_index_pair(mp2, mp4));
			v3 = get_line_crossing(Point_index_pair(mp2, mp5));
			v4 = get_line_crossing(Point_index_pair(mp2, mp1));
			mesh.add_quadrilateral(v1, v2, v3, v4);
			v1 = get_line_crossing(Point_index_pair(mp3, mp0));
			v2 = get_line_crossing(Point_index_pair(mp3, mp1));
			v3 = get_line_crossing(Point_index_pair(mp3, mp5));
			v4 = get_line_crossing(Point_index_pair(mp3, mp4));
			mesh.add_quadrilateral(v1, v2, v3, v4);
			break;
		default:
			display_message(ERROR_MESSAGE,
				"Isosurface_builder::cross_octahedron.  Unknown case %d", final_case);
			return_code = 0;
			break;
		}
	}
	LEAVE;

	return (return_code);
}

/**
 *        4\ 
 *       /.\ \
 *      /.  \  \
 *     /2 . .\. 3
 *    /.      \/
 *   0--------1
 */
int Isosurface_builder::cross_pyramid(
	const Point_index& p0, const Point_index& p1, const Point_index& p2,
	const Point_index& p3, const Point_index& p4)
{
	// first column is final case in standard orientation
	// columns 1-5 are indexes of initial vertices in order to rotate to final case
	// column 6 is 1 for inverse case: reverse polygon winding
	static const unsigned char pyr_case[32][7] =
	{
		{  0, 0, 0, 0, 0, 0, 0 }, //  0
		{  1, 0, 1, 2, 3, 4, 0 }, //  1 : 0 over = standard case
		{  1, 1, 3, 0, 2, 4, 0 }, //  2
		{  3, 0, 1, 2, 3, 4, 0 }, //  3 : 0,1 over = standard case
		{  1, 2, 0, 3, 1, 4, 0 }, //  4 
		{  3, 2, 0, 3, 1, 4, 0 }, //  5
		{  9, 1, 3, 0, 2, 4, 0 }, //  6
		{  7, 0, 1, 2, 3, 4, 0 }, //  7 : 0,1,2 over = standard case
		{  1, 3, 2, 1, 0, 4, 0 }, //  8
		{  9, 0, 1, 2, 3, 4, 0 }, //  9 : 0,3 over = standard case
		{  3, 1, 3, 0, 2, 4, 0 }, // 10
		{  7, 1, 3, 0, 2, 4, 0 }, // 11
		{  3, 3, 2, 1, 0, 4, 0 }, // 12
		{  7, 2, 0, 3, 1, 4, 0 }, // 13
		{  7, 3, 2, 1, 0, 4, 0 }, // 14
		{ 15, 0, 1, 2, 3, 4, 0 }, // 15 : 0,1,2,3 over = standard case
		{ 15, 0, 1, 2, 3, 4, 1 }, // 16 : 4 over = case 15 inverse
		{  7, 3, 2, 1, 0, 4, 1 }, // 17
		{  7, 2, 0, 3, 1, 4, 1 }, // 18
		{  3, 3, 2, 1, 0, 4, 1 }, // 19
		{  7, 1, 3, 0, 2, 4, 1 }, // 20
		{  3, 1, 3, 0, 2, 4, 1 }, // 21
		{  9, 0, 1, 2, 3, 4, 1 }, // 22 : 1,2,4 over = case 9 inverse
		{  1, 3, 2, 1, 0, 4, 1 }, // 23
		{  7, 0, 1, 2, 3, 4, 1 }, // 24 : 3,4 over = case 7 inverse
		{  9, 1, 3, 0, 2, 4, 1 }, // 25
		{  3, 2, 0, 3, 1, 4, 1 }, // 26
		{  1, 2, 0, 3, 1, 4, 1 }, // 27
		{  3, 0, 1, 2, 3, 4, 1 }, // 28 : 2,3,4 over = case 3 inverse
		{  1, 1, 3, 0, 2, 4, 1 }, // 29 
		{  1, 0, 1, 2, 3, 4, 1 }, // 30 : 1,2,3,4 over = case 1 inverse
		{  0, 0, 0, 0, 0, 0, 0 }  // 31
	};
	
	ENTER(Isosurface_builder::cross_pyramid);
	int return_code = 1;
	int unrotated_case =	
		((get_scalar(p0) > current_iso_value) ?  1 : 0) +
		((get_scalar(p1) > current_iso_value) ?  2 : 0) +
		((get_scalar(p2) > current_iso_value) ?  4 : 0) +
		((get_scalar(p3) > current_iso_value) ?  8 : 0) +
		((get_scalar(p4) > current_iso_value) ? 16 : 0);
	int final_case = pyr_case[unrotated_case][0];
	if (0 < final_case)
	{
		bool inverse = (0 != pyr_case[unrotated_case][6]);
		Point_index mp[5] = { p0, p1, p2, p3, p4 };
		Point_index mp0 = mp[pyr_case[unrotated_case][1]];
		Point_index mp1 = mp[pyr_case[unrotated_case][2]];
		Point_index mp2 = mp[pyr_case[unrotated_case][3]];
		Point_index mp3 = mp[pyr_case[unrotated_case][4]];
		Point_index mp4 = mp[pyr_case[unrotated_case][5]];
		const Iso_vertex *v1, *v2, *v3, *v4, *v5, *v6;
		Iso_mesh& mesh = get_mesh();
		switch (final_case)
		{
		case 1: // 0 > iso_value, with inverse
			v1 = get_line_crossing(Point_index_pair(mp0, mp1));
			v2 = get_line_crossing(Point_index_pair(mp0, mp4));
			v3 = get_line_crossing(Point_index_pair(mp0, mp2));
			mesh.add_triangle(v1, v2, v3, inverse);
			break;
		case 3: // 0,1 > iso_value, with inverse
			v1 = get_line_crossing(Point_index_pair(mp0, mp4));
			v2 = get_line_crossing(Point_index_pair(mp0, mp2));
			v3 = get_line_crossing(Point_index_pair(mp1, mp3));
			v4 = get_line_crossing(Point_index_pair(mp1, mp4));
			mesh.add_quadrilateral(v1, v2, v3, v4, inverse);
			break;
		case 7: // 0,1,2 > iso_value, with inverse
			v1 = get_line_crossing(Point_index_pair(mp0, mp4));
			v2 = get_line_crossing(Point_index_pair(mp2, mp4));
			v3 = get_line_crossing(Point_index_pair(mp2, mp3));
			v4 = get_line_crossing(Point_index_pair(mp1, mp3));
			v5 = get_line_crossing(Point_index_pair(mp1, mp4));
			mesh.add_pentagon(v1, v2, v3, v4, v5, inverse);
			break;
		case 9: // 0,3 > iso_value, with inverse
			v1 = get_line_crossing(Point_index_pair(mp0, mp1));
			v2 = get_line_crossing(Point_index_pair(mp0, mp4));
			v3 = get_line_crossing(Point_index_pair(mp0, mp2));
			v4 = get_line_crossing(Point_index_pair(mp3, mp2));
			v5 = get_line_crossing(Point_index_pair(mp3, mp4));
			v6 = get_line_crossing(Point_index_pair(mp3, mp1));
			if (ambiguous_quad_resolves_over(p0, p1, p2, p3) != inverse)
			{
				mesh.add_quadrilateral(v1, v2, v5, v6, inverse);
				mesh.add_quadrilateral(v2, v3, v4, v5, inverse);
			}
			else
			{
				mesh.add_triangle(v1, v2, v3, inverse);
				mesh.add_triangle(v4, v5, v6, inverse);
			}
			break;
		case 15: // 0,1,2,3 > iso_value, with inverse
			v1 = get_line_crossing(Point_index_pair(mp0, mp4));
			v2 = get_line_crossing(Point_index_pair(mp2, mp4));
			v3 = get_line_crossing(Point_index_pair(mp3, mp4));
			v4 = get_line_crossing(Point_index_pair(mp1, mp4));
			mesh.add_quadrilateral(v1, v2, v3, v4, inverse);
			break;
		default:
			display_message(ERROR_MESSAGE,
				"Isosurface_builder::cross_pyramid.  Unknown case %d", final_case);
			return_code = 0;
			break;
		}
	}
	LEAVE;

	return (return_code);
}

/**
 *      3_____5
 *      |\   /|
 *      | \ / |
 *      |  4  |
 *      0..|..2
 *       \ | /
 *        \|/
 *         1
 */
int Isosurface_builder::cross_triangle_wedge(
	const Point_index& p0, const Point_index& p1, const Point_index& p2,
	const Point_index& p3, const Point_index& p4, const Point_index& p5)
{
	// first column is final case in standard orientation
	// columns 1-6 are indexes of initial vertices in order to rotate to final case
	static const int tri_case[64][7] =
	{
		{  0, 0, 0, 0, 0, 0, 0 }, //  0
		{  1, 0, 1, 2, 3, 4, 5 }, //  1 : 0 over = standard case
		{  1, 1, 2, 0, 4, 5, 3 }, //  2
		{  3, 0, 1, 2, 3, 4, 5 }, //  3 : 0,1 over = standard case
		{  1, 2, 0, 1, 5, 3, 4 }, //  4
		{  3, 2, 0, 1, 5, 3, 4 }, //  5
		{  3, 1, 2, 0, 4, 5, 3 }, //  6
		{  7, 0, 1, 2, 3, 4, 5 }, //  7 : 0,1,2 over = standard case
		{  1, 3, 5, 4, 0, 2, 1 }, //  8
		{  9, 0, 1, 2, 3, 4, 5 }, //  9 : 0,3 over = standard case
		{ 33, 1, 2, 0, 4, 5, 3 }, // 10
		{ 11, 0, 1, 2, 3, 4, 5 }, // 11 : 0,1,3 over = standard case
		{ 17, 2, 0, 1, 5, 3, 4 }, // 12
		{ 19, 2, 0, 1, 5, 3, 4 }, // 13
		{ 35, 1, 2, 0, 4, 5, 3 }, // 14
		{ 15, 0, 1, 2, 3, 4, 5 }, // 15 : 0,1,2,3 over = standard case
		{  1, 4, 3, 5, 1, 0, 2 }, // 16
		{ 17, 0, 1, 2, 3, 4, 5 }, // 17 : 0,4 over = standard case
		{  9, 1, 2, 0, 4, 5, 3 }, // 18
		{ 19, 0, 1, 2, 3, 4, 5 }, // 19 : 0,1,4 over = standard case
		{ 33, 2, 0, 1, 5, 3, 4 }, // 20
		{ 35, 2, 0, 1, 5, 3, 4 }, // 21
		{ 11, 1, 2, 0, 4, 5, 3 }, // 22
		{ 15, 1, 2, 0, 4, 5, 3 }, // 23
		{  3, 4, 3, 5, 1, 0, 2 }, // 24
		{ 19, 4, 3, 5, 1, 0, 2 }, // 25
		{ 11, 4, 3, 5, 1, 0, 2 }, // 26
		{ 27, 0, 1, 2, 3, 4, 5 }, // 27 : 0,1,3,4 over = standard case
		{ 35, 4, 3, 5, 1, 0, 2 }, // 28
		{ 51, 2, 0, 1, 5, 3, 4 }, // 29
		{ 43, 1, 2, 0, 4, 5, 3 }, // 30 
		{ 31, 0, 1, 2, 3, 4, 5 }, // 31 : 0,1,2,3,4 over = standard case
		{  1, 5, 4, 3, 2, 1, 0 }, // 32
		{ 33, 0, 1, 2, 3, 4, 5 }, // 33 : 0,5 over = standard case
		{ 17, 1, 2, 0, 4, 5, 3 }, // 34
		{ 35, 0, 1, 2, 3, 4, 5 }, // 35 : 0,1,5 over = standard case
		{  9, 2, 0, 1, 5, 3, 4 }, // 36
		{ 11, 2, 0, 1, 5, 3, 4 }, // 37
		{ 19, 1, 2, 0, 4, 5, 3 }, // 38
		{ 15, 2, 0, 1, 5, 3, 4 }, // 39
		{  3, 3, 5, 4, 0, 2, 1 }, // 40
		{ 11, 3, 5, 4, 0, 2, 1 }, // 41
		{ 35, 3, 5, 4, 0, 2, 1 }, // 42
		{ 43, 0, 1, 2, 3, 4, 5 }, // 43 : 0,1,3,5 over = standard case
		{ 19, 3, 5, 4, 0, 2, 1 }, // 44
		{ 27, 2, 0, 1, 5, 3, 4 }, // 45
		{ 51, 1, 2, 0, 4, 5, 3 }, // 46
		{ 31, 2, 0, 1, 5, 3, 4 }, // 47
		{  3, 5, 4, 3, 2, 1, 0 }, // 48
		{ 35, 5, 4, 3, 2, 1, 0 }, // 49
		{ 19, 5, 4, 3, 2, 1, 0 }, // 50
		{ 51, 0, 1, 2, 3, 4, 5 }, // 51 : 0,1,4,5 over = standard case
		{ 11, 5, 4, 3, 2, 1, 0 }, // 52
		{ 43, 2, 0, 1, 5, 3, 4 }, // 53
		{ 27, 1, 2, 0, 4, 5, 3 }, // 54
		{ 31, 1, 2, 0, 4, 5, 3 }, // 55
		{  7, 5, 4, 3, 2, 1, 0 }, // 56
		{ 15, 3, 5, 4, 0, 2, 1 }, // 57
		{ 15, 4, 3, 5, 1, 0, 2 }, // 58
		{ 31, 4, 3, 5, 1, 0, 2 }, // 59
		{ 15, 5, 4, 3, 2, 1, 0 }, // 60
		{ 31, 3, 5, 4, 0, 2, 1 }, // 61
		{ 31, 5, 4, 3, 2, 1, 0 }, // 62
		{  0, 0, 0, 0, 0, 0, 0 }  // 63
	};
	
	ENTER(Isosurface_builder::cross_triangle_wedge);
	int return_code = 1;
	int unrotated_case =	
		((get_scalar(p0) > current_iso_value) ?  1 : 0) +
		((get_scalar(p1) > current_iso_value) ?  2 : 0) +
		((get_scalar(p2) > current_iso_value) ?  4 : 0) +
		((get_scalar(p3) > current_iso_value) ?  8 : 0) +
		((get_scalar(p4) > current_iso_value) ? 16 : 0) +
		((get_scalar(p5) > current_iso_value) ? 32 : 0);
	int final_case = tri_case[unrotated_case][0];
	Iso_mesh& mesh = get_mesh();
	if (0 < final_case)
	{
		Point_index mp[6] = { p0, p1, p2, p3, p4, p5 };
		Point_index mp0 = mp[tri_case[unrotated_case][1]];
		Point_index mp1 = mp[tri_case[unrotated_case][2]];
		Point_index mp2 = mp[tri_case[unrotated_case][3]];
		Point_index mp3 = mp[tri_case[unrotated_case][4]];
		Point_index mp4 = mp[tri_case[unrotated_case][5]];
		Point_index mp5 = mp[tri_case[unrotated_case][6]];
		const Iso_vertex *v1, *v2, *v3, *v4, *v5, *v6, *v7;
		switch (final_case)
		{
		case 1: // 0 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp0, mp1));
			v2 = get_line_crossing(Point_index_pair(mp0, mp3));
			v3 = get_line_crossing(Point_index_pair(mp0, mp2));
			mesh.add_triangle(v1, v2, v3);
			break;
		case 3: // 0,1 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp0, mp2));
			v2 = get_line_crossing(Point_index_pair(mp1, mp2));
			v3 = get_line_crossing(Point_index_pair(mp1, mp4));
			v4 = get_line_crossing(Point_index_pair(mp0, mp3));
			mesh.add_quadrilateral(v1, v2, v3, v4);
			break;
		case 7: // 0,1,2 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp0, mp3));
			v2 = get_line_crossing(Point_index_pair(mp2, mp5));
			v3 = get_line_crossing(Point_index_pair(mp1, mp4));
			mesh.add_triangle(v1, v2, v3);
			break;
		case 9: // 0,3 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp0, mp1));
			v2 = get_line_crossing(Point_index_pair(mp3, mp4));
			v3 = get_line_crossing(Point_index_pair(mp3, mp5));
			v4 = get_line_crossing(Point_index_pair(mp0, mp2));
			mesh.add_quadrilateral(v1, v2, v3, v4);
			break;
		case 11: // 0,1,3 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp0, mp2));
			v2 = get_line_crossing(Point_index_pair(mp1, mp2));
			v3 = get_line_crossing(Point_index_pair(mp1, mp4));
			v4 = get_line_crossing(Point_index_pair(mp3, mp4));
			v5 = get_line_crossing(Point_index_pair(mp3, mp5));
			mesh.add_pentagon(v1, v2, v3, v4, v5);
			break;
		case 15: // 0,1,2,3 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp1, mp4));
			v2 = get_line_crossing(Point_index_pair(mp3, mp4));
			v3 = get_line_crossing(Point_index_pair(mp3, mp5));
			v4 = get_line_crossing(Point_index_pair(mp2, mp5));
			mesh.add_quadrilateral(v1, v2, v3, v4);
			break;
		case 17: // 0,4 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp0, mp3));
			v2 = get_line_crossing(Point_index_pair(mp0, mp2));
			v3 = get_line_crossing(Point_index_pair(mp0, mp1));
			v4 = get_line_crossing(Point_index_pair(mp4, mp1));
			v5 = get_line_crossing(Point_index_pair(mp4, mp5));
			v6 = get_line_crossing(Point_index_pair(mp4, mp3));
			if (ambiguous_quad_resolves_over(p0, p1, p3, p4))
			{
				mesh.add_hexagon(v1, v2, v3, v4, v5, v6);
			}
			else
			{
				mesh.add_triangle(v1, v2, v3);
				mesh.add_triangle(v4, v5, v6);
			}
			break;
		case 19: // 0,1,4 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp0, mp2));
			v2 = get_line_crossing(Point_index_pair(mp1, mp2));
			v3 = get_line_crossing(Point_index_pair(mp4, mp5));
			v4 = get_line_crossing(Point_index_pair(mp4, mp3));
			v5 = get_line_crossing(Point_index_pair(mp0, mp3));
			mesh.add_pentagon(v1, v2, v3, v4, v5);
			break;
		case 27: // 0,1,3,4 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp0, mp2));
			v2 = get_line_crossing(Point_index_pair(mp1, mp2));
			v3 = get_line_crossing(Point_index_pair(mp4, mp5));
			v4 = get_line_crossing(Point_index_pair(mp3, mp5));
			mesh.add_quadrilateral(v1, v2, v3, v4);
			break;
		case 31: // 0,1,2,3,4 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp5, mp2));
			v2 = get_line_crossing(Point_index_pair(mp5, mp4));
			v3 = get_line_crossing(Point_index_pair(mp5, mp3));
			mesh.add_triangle(v1, v2, v3);
			break;
		case 33: // 0,5 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp0, mp2));
			v2 = get_line_crossing(Point_index_pair(mp0, mp1));
			v3 = get_line_crossing(Point_index_pair(mp0, mp3));
			v4 = get_line_crossing(Point_index_pair(mp5, mp3));
			v5 = get_line_crossing(Point_index_pair(mp5, mp4));
			v6 = get_line_crossing(Point_index_pair(mp5, mp2));
			if (ambiguous_quad_resolves_over(p0, p2, p3, p5))
			{
				mesh.add_hexagon(v1, v2, v3, v4, v5, v6);
			}
			else
			{
				mesh.add_triangle(v1, v2, v3);
				mesh.add_triangle(v4, v5, v6);
			}
			break;
		case 35: // 0,1,5 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp0, mp3));
			v2 = get_line_crossing(Point_index_pair(mp0, mp2));
			v3 = get_line_crossing(Point_index_pair(mp1, mp2));
			v4 = get_line_crossing(Point_index_pair(mp1, mp4));
			v5 = get_line_crossing(Point_index_pair(mp5, mp2));
			v6 = get_line_crossing(Point_index_pair(mp5, mp3));
			v7 = get_line_crossing(Point_index_pair(mp5, mp4));
			{
				bool mean1245_over = ambiguous_quad_resolves_over(p1, p2, p4, p5);
				bool mean0235_over = ambiguous_quad_resolves_over(p0, p2, p3, p5);
				if (mean1245_over && mean0235_over)
				{
					mesh.add_quadrilateral(v1, v6, v7, v4);
					mesh.add_triangle(v2, v3, v5);
				}
				else // both faces non-positive crossing
				{
					mesh.add_quadrilateral(v1, v2, v3, v4);
					if (mean1245_over)
					{
						mesh.add_quadrilateral(v3, v5, v7, v4);
					}
					else if (mean0235_over)
					{
						mesh.add_quadrilateral(v2, v1, v6, v5);
					}
					mesh.add_triangle(v5, v6, v7);
				}
			}
			break;
		case 43: // 0,1,3,5 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp5, mp2));
			v2 = get_line_crossing(Point_index_pair(mp0, mp2));
			v3 = get_line_crossing(Point_index_pair(mp1, mp2));
			v4 = get_line_crossing(Point_index_pair(mp1, mp4));
			v5 = get_line_crossing(Point_index_pair(mp3, mp4));
			v6 = get_line_crossing(Point_index_pair(mp5, mp4));
			// fails on polygon slice:
			if (ambiguous_quad_resolves_over(p1, p2, p4, p5))
			{
				mesh.add_triangle(v1, v2, v3);
				mesh.add_triangle(v4, v5, v6);
			}
			else
			{
				mesh.add_hexagon(v1, v2, v3, v4, v5, v6);
			}
			break;
		case 51: // 0,1,4,5 > iso_value
			v1 = get_line_crossing(Point_index_pair(mp0, mp2));
			v2 = get_line_crossing(Point_index_pair(mp1, mp2));
			v3 = get_line_crossing(Point_index_pair(mp5, mp2));
			v4 = get_line_crossing(Point_index_pair(mp5, mp3));
			v5 = get_line_crossing(Point_index_pair(mp4, mp3));
			v6 = get_line_crossing(Point_index_pair(mp0, mp3));
			if (ambiguous_quad_resolves_over(p0, p2, p3, p5))
			{
				mesh.add_triangle(v1, v2, v3);
				mesh.add_triangle(v4, v5, v6);
			}
			else
			{
				mesh.add_hexagon(v1, v2, v3, v4, v5, v6);
			}
			break;
		default:
			display_message(ERROR_MESSAGE,
				"Isosurface_builder::cross_triangle_wedge.  Unknown case %d", final_case);
			return_code = 0;
			break;
		}
	}
	LEAVE;

	return (return_code);
}

int Isosurface_builder::sweep()
{
	ENTER(Isosurface_builder::sweep);
	int return_code = 1;
	double scalar_value;
	FE_value scalar_FE_value, xi[3];

	for (int k = number_in_xi3; (k >= 0) && return_code; k--)
	{
		xi[2] = k*delta_xi3;
		int mod_number_in_xi2 = number_in_xi2;
		if (simplex23)
		{
			mod_number_in_xi2 -= k; // assume number_in_xi equal on linked axes
		}

		for (int j = mod_number_in_xi2; (j >= 0) && return_code; j--)
		{
			xi[1] = j*delta_xi2;
			int mod_number_in_xi1 = number_in_xi1;
			if (simplex12)
			{
				mod_number_in_xi1 -= j;
			}
			if (simplex13)
			{
				mod_number_in_xi1 -= k;
			}

			for (int i = mod_number_in_xi1; i >= 0; i--)
			{
				xi[0] = i*delta_xi1; 
				if (Computed_field_evaluate_in_element(scalar_field, element, xi,
					time, /*top_level_element*/(struct FE_element *)NULL, &scalar_FE_value,
					/*derivatives*/(FE_value *)NULL))
				{
					scalar_value = static_cast<double>(scalar_FE_value);
					set_scalar(i, j, k, scalar_value);
					
					for (int v = 0; v < number_of_iso_values; v++)
					{
						current_iso_value_number = v;
						current_iso_value = specification.get_iso_value(v);

						if ((i < mod_number_in_xi1) && (j < mod_number_in_xi2) &&
							(k < number_in_xi3))
						{
							if (cube)
							{
								cross_cube(i, j, k);
							}
							else if (tetrahedron)
							{
								if (i < (mod_number_in_xi1 - 1))
								{
									if (i < (mod_number_in_xi1 - 2))
									{
										cross_tetrahedron(
											Point_index(i + 1, j + 1, k    ),
											Point_index(i    , j + 1, k + 1),
											Point_index(i + 1, j    , k + 1),
											Point_index(i + 1, j + 1, k + 1));
									}
									cross_octahedron(i, j, k);
								}
								cross_tetrahedron(
									Point_index(i, j, k),
									Point_index(i + 1, j, k),
									Point_index(i, j + 1, k),
									Point_index(i, j, k + 1));
							}
							else if (simplex12)
							{
								if (i < (mod_number_in_xi1 - 1))
								{
									cross_triangle_wedge(
										Point_index(i + 1, j, k    ), Point_index(i + 1, j + 1, k    ), Point_index(i, j + 1, k    ),
										Point_index(i + 1, j, k + 1), Point_index(i + 1, j + 1, k + 1), Point_index(i, j + 1, k + 1));
								}
								cross_triangle_wedge(
									Point_index(i, j, k    ), Point_index(i + 1, j, k    ), Point_index(i, j + 1, k    ),
									Point_index(i, j, k + 1), Point_index(i + 1, j, k + 1), Point_index(i, j + 1, k + 1));
							}
							else if (simplex23)
							{
								if (j < (mod_number_in_xi2 - 1))
								{
									cross_triangle_wedge(
										Point_index(i    , j + 1, k), Point_index(i    , j + 1, k + 1), Point_index(i    , j, k + 1),
										Point_index(i + 1, j + 1, k), Point_index(i + 1, j + 1, k + 1), Point_index(i + 1, j, k + 1));
								}
								cross_triangle_wedge(
									Point_index(i    , j, k), Point_index(i    , j + 1, k), Point_index(i    , j, k + 1),
									Point_index(i + 1, j, k), Point_index(i + 1, j + 1, k), Point_index(i + 1, j, k + 1));
							}
							else if (simplex13)
							{
								if (i < (mod_number_in_xi1 - 1))
								{
									cross_triangle_wedge(
										Point_index(i + 1, j    , k), Point_index(i, j    , k + 1), Point_index(i + 1, j    , k + 1),
										Point_index(i + 1, j + 1, k), Point_index(i, j + 1, k + 1), Point_index(i + 1, j + 1, k + 1));
								}
								cross_triangle_wedge(
									Point_index(i, j    , k), Point_index(i, j    , k + 1), Point_index(i + 1, j    , k),
									Point_index(i, j + 1, k), Point_index(i, j + 1, k + 1), Point_index(i + 1, j + 1, k));
							}
						}
					}
				}
				else
				{
					return_code = 0;
					break;
				}
			}
		}
	}
	LEAVE;
	
	return (return_code);
}

int Isosurface_builder::fill_graphics(struct GT_object *graphics_object,
	enum Render_type render_type)
{
	int return_code = 1;
	for (Iso_mesh_map_const_iterator mesh_iter = mesh_map.begin();
		mesh_iter != mesh_map.end(); mesh_iter++)
	{
		Iso_mesh& mesh = *(mesh_iter->second);
		const Iso_triangle_list& triangle_list = mesh.triangle_list;
		const int number_of_triangles = triangle_list.size();
		if (0 < number_of_triangles)
		{
			Triple *points = NULL;
			Triple *normalpoints = NULL;
			Triple *texturepoints = NULL;
			Triple *tangentpoints = NULL;
			int n_data_components = 0;
			GTDATA *data = NULL;
			ALLOCATE(points, Triple, number_of_triangles*3);
			ALLOCATE(normalpoints, Triple, number_of_triangles*3);
			if (NULL != texture_coordinate_field)
			{
				ALLOCATE(texturepoints, Triple, number_of_triangles*3);
			}
			if ((NULL != points) && (NULL != normalpoints))
			{
				Triple *point = points;
				Triple *normal = normalpoints;
				Triple *texture = texturepoints;
				for (Iso_triangle_list_const_iterator triangle_iter = triangle_list.begin(); triangle_iter != triangle_list.end(); triangle_iter++)
				{
					const Iso_triangle *triangle = *triangle_iter;
					const Iso_vertex* v1 = triangle->v1;
					const Iso_vertex* v2 = triangle->v2;
					const Iso_vertex* v3 = triangle->v3;
					(*point)[0] = (float)v1->coordinates[0]; 
					(*point)[1] = (float)v1->coordinates[1]; 
					(*point)[2] = (float)v1->coordinates[2];
					point++;
					(*point)[0] = (float)v2->coordinates[0]; 
					(*point)[1] = (float)v2->coordinates[1]; 
					(*point)[2] = (float)v2->coordinates[2];
					point++;
					(*point)[0] = (float)v3->coordinates[0]; 
					(*point)[1] = (float)v3->coordinates[1]; 
					(*point)[2] = (float)v3->coordinates[2];
					point++;
					// calculate facet normal:
					FE_value axis1[3], axis2[3], facet_normal[3];
					axis1[0] = v2->coordinates[0] - v1->coordinates[0];
					axis1[1] = v2->coordinates[1] - v1->coordinates[1];
					axis1[2] = v2->coordinates[2] - v1->coordinates[2];
					axis2[0] = v3->coordinates[0] - v1->coordinates[0];
					axis2[1] = v3->coordinates[1] - v1->coordinates[1];
					axis2[2] = v3->coordinates[2] - v1->coordinates[2];
					cross_product_FE_value_vector3(axis1, axis2, facet_normal);
					normalize_FE_value3(facet_normal);
					for (int i = 0; i < 3; i++)
					{
						(*normal)[0] = (float)facet_normal[0];
						(*normal)[1] = (float)facet_normal[1];
						(*normal)[2] = (float)facet_normal[2];
						normal++;
					}
					if (NULL != texture)
					{
						(*texture)[0] = (float)v1->texture_coordinates[0]; 
						(*texture)[1] = (float)v1->texture_coordinates[1]; 
						(*texture)[2] = (float)v1->texture_coordinates[2];
						texture++;
						(*texture)[0] = (float)v2->texture_coordinates[0]; 
						(*texture)[1] = (float)v2->texture_coordinates[1]; 
						(*texture)[2] = (float)v2->texture_coordinates[2];
						texture++;
						(*texture)[0] = (float)v3->texture_coordinates[0]; 
						(*texture)[1] = (float)v3->texture_coordinates[1]; 
						(*texture)[2] = (float)v3->texture_coordinates[2];
						texture++;
					}
				}
			}
			else
			{
				return_code = 0;
			}
			if (return_code)
			{
				GT_surface *surface = CREATE(GT_surface)(g_SH_DISCONTINUOUS_TEXMAP, render_type, g_TRIANGLE,
					/*number_of_points_in_xi1*/number_of_triangles, /*number_of_points_in_xi2*/3, points,
					normalpoints, tangentpoints, texturepoints, n_data_components, data);
				if (!GT_OBJECT_ADD(GT_surface)(graphics_object, time, surface))
				{
					DESTROY(GT_surface)(&surface);
					return_code = 0;
				}
			}
			else
			{
				DEALLOCATE(points);
				DEALLOCATE(normalpoints);
			}
		}
	}
	return (return_code);
}

} // anonymous namespace

/*
Global functions
----------------
*/

struct Iso_surface_specification *Iso_surface_specification_create(
	int number_of_iso_values, const double *iso_values,
	double first_iso_value, double last_iso_value,
	struct Computed_field *coordinate_field, struct Computed_field *data_field,
	struct Computed_field *scalar_field,
	struct Computed_field *texture_coordinate_field)
{
	ENTER(Iso_surface_specification_create);
	struct Iso_surface_specification *specification = NULL;
	if (Computed_field_has_3_components(coordinate_field, NULL) &&
		(0 <= number_of_iso_values) && 
		(1 == Computed_field_get_number_of_components(scalar_field)) &
		((NULL == texture_coordinate_field) ||
			(3 >= Computed_field_get_number_of_components(texture_coordinate_field))))
	{
		ALLOCATE(specification, struct Iso_surface_specification, 1);
		if (NULL != specification)
		{
			specification->number_of_iso_values = number_of_iso_values;
			specification->coordinate_field = ACCESS(Computed_field)(coordinate_field);
			specification->data_field =
				(NULL != data_field) ? ACCESS(Computed_field)(data_field) : NULL;
			specification->scalar_field = ACCESS(Computed_field)(scalar_field);
			specification->texture_coordinate_field =
				(NULL != texture_coordinate_field) ? ACCESS(Computed_field)(texture_coordinate_field) : NULL;
			specification->iso_values = NULL;
			specification->first_iso_value = first_iso_value;
			specification->last_iso_value = last_iso_value;
			specification->iso_value_range = 0;
			if (NULL != iso_values)
			{
				if (0 < number_of_iso_values)
				{
					specification->iso_values = new double[number_of_iso_values];
					memcpy(specification->iso_values, iso_values, number_of_iso_values*sizeof(double));
				}
			}
			else if (1 < number_of_iso_values)
			{
				specification->iso_value_range = (last_iso_value - first_iso_value)
					/ (double)(number_of_iso_values - 1);
			}
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Iso_surface_specification_create.  Invalid argument(s)");
	}
	LEAVE;

	return (specification);
}

int Iso_surface_specification_destroy(
	struct Iso_surface_specification **specification_address)
{
	int return_code;
	struct Iso_surface_specification *specification;

	ENTER(Iso_surface_specification_destroy);
	if ((NULL != specification_address) &&
		(NULL != (specification = *specification_address)))
	{
		delete[] specification->iso_values;
		DEACCESS(Computed_field)(&specification->coordinate_field);
		REACCESS(Computed_field)(&specification->data_field, NULL);
		DEACCESS(Computed_field)(&specification->scalar_field);
		REACCESS(Computed_field)(&specification->texture_coordinate_field, NULL);
		DEALLOCATE(*specification_address);
		return_code = 1;
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"Iso_surface_specification_destroy.  Invalid argument(s)");
		return_code = 0;
	}
	LEAVE;

	return (return_code);
}

int create_iso_surfaces_from_FE_element_new(struct FE_element *element,
	FE_value time, int *number_in_xi,
	struct Iso_surface_specification *specification,
	struct GT_object *graphics_object, enum Render_type render_type)
{
	ENTER(create_iso_surfaces_from_FE_element_new);
	int return_code = 0;
	if ((NULL != element) && (3 == get_FE_element_dimension(element)) &&
		(NULL != number_in_xi) &&
		(0 < number_in_xi[0]) && (0 < number_in_xi[1]) && (0 < number_in_xi[2]) &&
		(NULL != specification) && (NULL != graphics_object))
	{
		Isosurface_builder iso_builder(element, time,
			number_in_xi[0], number_in_xi[1], number_in_xi[2], *specification);
		return_code = iso_builder.sweep();
		if (return_code)
		{
			return_code = iso_builder.fill_graphics(graphics_object, render_type);
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"create_iso_surfaces_from_FE_element_new.  Invalid argument(s)");
	}
	LEAVE;

	return (return_code);
} /* create_iso_surfaces_from_FE_element_new */
