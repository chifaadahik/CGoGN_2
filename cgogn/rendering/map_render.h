
/*******************************************************************************
* CGoGN: Combinatorial and Geometric modeling with Generic N-dimensional Maps  *
* Copyright (C) 2015, IGG Group, ICube, University of Strasbourg, France       *
*                                                                              *
* This library is free software; you can redistribute it and/or modify it      *
* under the terms of the GNU Lesser General Public License as published by the *
* Free Software Foundation; either version 2.1 of the License, or (at your     *
* option) any later version.                                                   *
*                                                                              *
* This library is distributed in the hope that it will be useful, but WITHOUT  *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or        *
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License  *
* for more details.                                                            *
*                                                                              *
* You should have received a copy of the GNU Lesser General Public License     *
* along with this library; if not, write to the Free Software Foundation,      *
* Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.           *
*                                                                              *
* Web site: http://cgogn.unistra.fr/                                           *
* Contact information: cgogn@unistra.fr                                        *
*                                                                              *
*******************************************************************************/

#ifndef RENDERING_MAP_RENDER_H_
#define RENDERING_MAP_RENDER_H_

#include <QOpenGLBuffer>
#include <QOpenGLFunctions>

#include <core/cmap/map_base.h> // impossible to include directly attribute_handler.h !
#include <geometry/algos/ear_triangulation.h>
#include <rendering/drawer.h>

namespace cgogn
{

namespace rendering
{

enum DrawingType
{
	POINTS = 0,
	LINES,
	TRIANGLES,
	SIZE_BUFFER
};

class MapRender
{
protected:

	QOpenGLBuffer* indices_buffers_[SIZE_BUFFER];
	bool indices_buffers_uptodate_[SIZE_BUFFER];
	unsigned int nb_indices_[SIZE_BUFFER];

public:

	inline MapRender()
	{
		for (unsigned int i = 0u; i < SIZE_BUFFER; ++i)
		{
			indices_buffers_[i] = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
			indices_buffers_[i]->setUsagePattern(QOpenGLBuffer::StaticDraw);
		}
	}

	inline ~MapRender()
	{
		for (unsigned int i = 0u; i < SIZE_BUFFER; ++i)
			delete indices_buffers_[i];
	}

	inline bool is_primitive_uptodate(DrawingType prim)  { return indices_buffers_uptodate_[prim]; }

	template <typename MAP>
	void init_points(MAP& m, std::vector<unsigned int>& table_indices)
	{
		//		table_indices.reserve(m.get_nb_darts()/6);
		m.foreach_cell([&] (typename MAP::Vertex v)
		{
			table_indices.push_back(m.get_embedding(v));
		});
	}

	template <typename MAP>
	void init_lines(MAP& m, std::vector<unsigned int>& table_indices)
	{
		using Vertex = typename MAP::Vertex;
		using Edge = typename MAP::Edge;
		//		table_indices.reserve(m.get_nb_darts()/2);
		m.foreach_cell([&] (Edge e)
		{
			table_indices.push_back(m.get_embedding(Vertex(e.dart)));
			table_indices.push_back(m.get_embedding(Vertex(m.phi1(e.dart))));
		});
	}

	template <typename VEC3, typename MAP>
	void init_triangles(MAP& m, std::vector<unsigned int>& table_indices, const typename MAP::template VertexAttributeHandler<VEC3>& position)
	{
		using Vertex = typename MAP::Vertex;
		using Face = typename MAP::Face;
		// reserve more ?
		//		table_indices.reserve(m.get_nb_darts()/3);
		m.foreach_cell([&] (Face f)
		{
			if (m.has_degree(f,3))
			{
				table_indices.push_back(m.get_embedding(Vertex(f.dart)));
				table_indices.push_back(m.get_embedding(Vertex(m.phi1(f.dart))));
				table_indices.push_back(m.get_embedding(Vertex(m.phi1(m.phi1(f.dart)))));
			}
			else
			{
				cgogn::geometry::compute_ear_triangulation<VEC3>(m,f,position,table_indices);
			}
		});
	}

	template <typename VEC3, typename MAP>
	void init_primitives(MAP& m, DrawingType prim, const typename MAP::template VertexAttributeHandler<VEC3>& position)
	{
		std::vector<unsigned int> table_indices;

		switch (prim)
		{
			case POINTS:
				init_points(m, table_indices);
				break;
			case LINES:
				init_lines(m, table_indices);
				break;
			case TRIANGLES:
				init_triangles<VEC3>(m, table_indices, position);
				break;
			default:
				break;
		}

		if (!indices_buffers_[prim]->isCreated())
			indices_buffers_[prim]->create();

		indices_buffers_uptodate_[prim] = true;
		nb_indices_[prim] = static_cast<unsigned int>(table_indices.size());
		indices_buffers_[prim]->bind();
		indices_buffers_[prim]->allocate(&(table_indices[0]), nb_indices_[prim] * sizeof(unsigned int));
		indices_buffers_[prim]->release();
	}

	inline void draw(DrawingType prim)
	{
		QOpenGLFunctions *ogl = QOpenGLContext::currentContext()->functions();

		indices_buffers_[prim]->bind();
		switch (prim)
		{
			case POINTS:
				ogl->glDrawElements(GL_POINTS, nb_indices_[POINTS], GL_UNSIGNED_INT, 0);
				break;
			case LINES:
				ogl->glDrawElements(GL_LINES, nb_indices_[LINES], GL_UNSIGNED_INT, 0);
				break;
			case TRIANGLES:
				ogl->glDrawElements(GL_TRIANGLES, nb_indices_[TRIANGLES], GL_UNSIGNED_INT, 0);
				break;
			default:
				break;
		}

		indices_buffers_[prim]->release();
	}
};



/**
 * @brief create embedding indices of vertices and faces for arch vertx of each face
 * @param m
 * @param position vertex positions use for ear triangulation)
 * @param indices1 embedding indices of vertices
 * @param indices2 embedding indices of faces
 */
template <typename VEC3, typename MAP>
void create_indices_vertices_faces(MAP& m, const typename MAP::template VertexAttributeHandler<VEC3>& position, std::vector<unsigned int>& indices1, std::vector<unsigned int>& indices2)
{
	using Vertex = typename MAP::Vertex;
	using Face = typename MAP::Face;

	indices1.reserve(m.nb_darts());
	indices2.reserve(m.nb_darts());
	indices1.clear();
	indices2.clear();

	//local vector for ear triangulation
	std::vector<unsigned int> local_vert_indices;
	local_vert_indices.reserve(256);

	m.foreach_cell([&] (Face f)
	{
		unsigned int ef = m.get_embedding(Face(f.dart));
		if (m.has_degree(f,3))
		{
			indices1.push_back(m.get_embedding(Vertex(f.dart)));
			indices1.push_back(m.get_embedding(Vertex(m.phi1(f.dart))));
			indices1.push_back(m.get_embedding(Vertex(m.phi1(m.phi1(f.dart)))));
			indices2.push_back(ef);
			indices2.push_back(ef);
			indices2.push_back(ef);

		}
		else
		{
			cgogn::geometry::compute_ear_triangulation<VEC3>(m,f,position,local_vert_indices);
			for (unsigned int i : local_vert_indices)
			{
				indices1.push_back(i);
				indices2.push_back(ef);
			}
		}
	});
}

/**
 * @brief create topologie rendering in a Drawer
 * @param m the map
 * @param position vertices positions
 * @param dr output drawer
 */
template <typename VEC3, typename MAP>
void create_drawer_topo2(MAP& m, const typename MAP::template VertexAttributeHandler<VEC3>& position,
				 Drawer& dr)
{
	using Vertex = typename MAP::Vertex;
	using Face = typename MAP::Face;
	using Scalar = typename VEC3::Scalar;

	const Scalar expl1 = 0.95;
	const Scalar expl2 = 0.85;

	const Scalar opp_expl1 = 1.0 -expl1;
	const Scalar opp_expl2 = 1.0 - expl2;


	std::vector<VEC3> local_vertices;
	local_vertices.reserve(256);

	dr.new_list();

	m.foreach_cell([&] (Face f)
	{
		local_vertices.clear();
		VEC3 center;
		center.setZero();
		unsigned int count = 0u;
		m.foreach_incident_vertex(f, [&] (Vertex v)
		{
			local_vertices.push_back(position[v]);
			center += position[v];
			count++;
		});
		center /= Scalar(count);

		// phi2 mid-edge: N -> 2N-1
		for (unsigned int i=0; i<count; ++i)
			local_vertices.push_back((local_vertices[i]+local_vertices[(i+1)%count])/Scalar(2.0));

		// dart round point: 0 -> N-1
		for (unsigned int i=0; i<count; ++i)
			local_vertices[i] = local_vertices[i] * expl2 + center * (opp_expl2);

		//dart other extremety: 2N -> 3N-1
		for (unsigned int i=0; i<count; ++i)
			local_vertices.push_back(local_vertices[i]*(opp_expl1) + local_vertices[(i+1)%count]*expl1);

		//phi2 mid-dart: 3N -> 4N-1
		for (unsigned int i=0; i<count; ++i)
			local_vertices.push_back((local_vertices[i]+local_vertices[(2*count+i+1)%count])/Scalar(2.0));

		//  TODO phi1

		dr.point_size(4.0);
		dr.begin(GL_POINTS);
		dr.color3f(1.0f,1.0f,1.0f);
		for (unsigned int i=0; i<count; ++i)
			dr.vertex3fv(local_vertices[i]);
		dr.end();
		dr.line_width(1.0);
		dr.begin(GL_LINES);
		for (unsigned int i=0; i<count; ++i)
		{
			dr.color3f(1.0f,1.0f,1.0f);
			dr.vertex3fv(local_vertices[i]);
			dr.vertex3fv(local_vertices[2*count+i]);
			dr.color3f(1.0f,0.0f,0.0f);
			dr.vertex3fv(local_vertices[count+i]);
			dr.vertex3fv(local_vertices[3*count+i]);
		}
		dr.end();
	});
	dr.end_list();
}



} // namespace rendering

} // namespace cgogn

#endif // RENDERING_MAP_RENDER_H_