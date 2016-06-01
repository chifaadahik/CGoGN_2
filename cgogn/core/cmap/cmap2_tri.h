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

#ifndef CGOGN_CORE_CMAP_CMAP2_TRI_H_
#define CGOGN_CORE_CMAP_CMAP2_TRI_H_

#include <cgogn/core/cmap/cmap1_tri.h>

namespace cgogn
{

// forward declaration of CMap2Builder_T
template <typename MAP_TRAITS> class CMap2TriBuilder_T;

template <typename MAP_TRAITS, typename MAP_TYPE>
class CMap2Tri_T : public CMap1Tri_T<MAP_TRAITS, MAP_TYPE>
{
public:
	static const uint8 DIMENSION = 2;
	static const uint8 PRIM_SIZE = 3;

	using MapTraits = MAP_TRAITS;
	using MapType = MAP_TYPE;
	using Inherit = CMap1Tri_T<MAP_TRAITS, MAP_TYPE>;
	using Self = CMap2Tri_T<MAP_TRAITS, MAP_TYPE>;

	using Builder = CMap2TriBuilder_T<MapTraits>;


	friend class MapBase<MAP_TRAITS, MAP_TYPE>;
	friend class CMap2TriBuilder_T<MapTraits>;
	friend class DartMarker_T<Self>;
	friend class cgogn::DartMarkerStore<Self>;

	using CDart		= typename Inherit::Vertex;
	using Vertex	= Cell<Orbit::PHI21>;
	using Edge		= Cell<Orbit::PHI2>;
	using Face		= typename Inherit::Face;
	using Volume	= Cell<Orbit::PHI1_PHI2>;

	using Boundary  = Face;
	using ConnectedComponent = Volume;

	template <typename T>
	using ChunkArray =  typename Inherit::template ChunkArray<T>;
	template <typename T>
	using ChunkArrayContainer =  typename Inherit::template ChunkArrayContainer<T>;

	template <typename T, Orbit ORBIT>
	using Attribute = typename Inherit::template Attribute<T, ORBIT>;
	template <typename T>
	using VertexAttribute = Attribute<T, Vertex::ORBIT>;
	template <typename T>
	using EdgeAttribute = Attribute<T, Edge::ORBIT>;
	template <typename T>
	using FaceAttribute = Attribute<T, Face::ORBIT>;
	template <typename T>
	using VolumeAttribute = Attribute<T, Volume::ORBIT>;

	using DartMarker = typename cgogn::DartMarker<Self>;
	using DartMarkerStore = typename cgogn::DartMarkerStore<Self>;

	template <Orbit ORBIT>
	using CellMarker = typename cgogn::CellMarker<Self, ORBIT>;

protected:

	ChunkArray<Dart>* phi2_;

	inline void init()
	{
		phi2_ = this->topology_.template add_attribute<Dart>("phi2");
	}

public:

	CMap2Tri_T() : Inherit()
	{
		init();
	}

	CGOGN_NOT_COPYABLE_NOR_MOVABLE(CMap2Tri_T);

	~CMap2Tri_T() override
	{}

	/*!
	 * \brief Check the integrity of embedding information
	 */
	inline bool check_embedding_integrity()
	{
		bool result = true;

		if (this->template is_embedded<CDart>())
			result = result && this->template is_well_embedded<CDart>();

		if (this->template is_embedded<Vertex>())
			result = result && this->template is_well_embedded<Vertex>();

		if (this->template is_embedded<Edge>())
			result = result && this->template is_well_embedded<Edge>();

		if (this->template is_embedded<Face>())
			result = result && this->template is_well_embedded<Face>();

		if (this->template is_embedded<Volume>())
			result = result && this->template is_well_embedded<Volume>();

		return result;
	}

	/*******************************************************************************
	 * Low-level topological operations
	 *******************************************************************************/

protected:

	/**
	 * \brief Init an newly added dart.
	 * The dart is defined as a fixed point for PHI2.
	 */
	inline void init_dart(Dart d)
	{
		Inherit::init_dart(d);
		(*phi2_)[d.index] = d;
		(*phi2_)[d.index+1] = Dart(d.index+1);
		(*phi2_)[d.index+2] = Dart(d.index+2);
	}

	/**
	 * @brief Check the integrity of a dart
	 * @param d the dart to check
	 * @return true if the integrity constraints are locally statisfied
	 * PHI2 should be an involution without fixed point
	 */
	inline bool check_integrity(Dart d) const
	{
		return (Inherit::check_integrity(d) &&
				phi2(phi2(d)) == d &&
				phi2(d) != d);
	}

	/**
	 * @brief Check the integrity of a boundary dart
	 * @param d the dart to check
	 * @return true if the bondary constraints are locally statisfied
	 * The boundary is a 1-manifold: the boundary marker is the same
	 * for all darts of a face and two boundary faces cannot be adjacent.
	 */
	inline bool check_boundary_integrity(Dart d) const
	{
		return ((  this->is_boundary(d) ==  this->is_boundary(this->phi1(d)) ) &&
				( !this->is_boundary(d) || !this->is_boundary(this->phi2(d)) ));
	}

	/**
	 * \brief Link dart d with dart e by the phi2 involution
	 * @param d,e the darts to link
	 *	- Before: d->d and e->e
	 *	- After:  d->e and e->d
	 */
	inline void phi2_sew(Dart d, Dart e)
	{
		cgogn_assert(phi2(d) == d);
		cgogn_assert(phi2(e) == e);
		(*phi2_)[d.index] = e;
		(*phi2_)[e.index] = d;
	}

	inline void phi2_sew_nocheck(Dart d, Dart e)
	{
		(*phi2_)[d.index] = e;
		(*phi2_)[e.index] = d;
	}


	/**
	 * \brief Remove the phi2 link between the current dart and its linked dart
	 * @param d the dart to unlink
	 * - Before: d->e and e->d
	 * - After:  d->d and e->e
	 */
	inline void phi2_unsew(Dart d)
	{
		Dart e = phi2(d);
		(*phi2_)[d.index] = d;
		(*phi2_)[e.index] = e;
	}

	/*******************************************************************************
	 * Basic topological operations
	 *******************************************************************************/

public:


	/**
	 * \brief phi2
	 * @param d
	 * @return phi2(d)
	 */
	inline Dart phi2(Dart d) const
	{
		return (*phi2_)[d.index];
	}


	/**
	 * \brief Composition of PHI calls
	 * @param d
	 * @return The result of successive applications of PHI1 and PHI2 on d.
	 * The template parameter contains a sequence (Base10 encoded) of PHI indeices.
	 * If N=0 the identity is used.
	 */
	template <uint64 N>
	inline Dart phi(Dart d) const
	{
		static_assert((N % 10) <= 2, "Composition of PHI: invalid index");
		switch(N % 10)
		{
			case 1 : return this->phi1(phi<N / 10>(d)) ;
			case 2 : return this->phi2(phi<N / 10>(d)) ;
			default : return d ;
		}
	}

	/*******************************************************************************
	 * High-level embedded and topological operations
	 *******************************************************************************/

protected:

	/**
	 * \brief Add a face in the map.
	 * \param size : the number of darts in the built face
	 * \return A dart of the built face.
	 * Two 1-face are built. The first one is the returned face,
	 * the second is a boundary face that closes the map.
	 */
	Dart add_tri_topo()
	{
		Dart d = Inherit::add_tri_topo();
		Dart e = Inherit::add_tri_topo();

		this->foreach_dart_of_PHI1(d, [&] (Dart it)
		{
			this->set_boundary(e, true);
			phi2_sew(it, e);
			e = this->phi_1(e);
		});

		return d;
	}

public:

	/**
	 * \brief Add a face in the map.
	 * \param size : the number of edges in the built face
	 * \return The built face
	 * If the map has Dart, Vertex, Edge, Face or Volume attributes,
	 * the inserted cells are automatically embedded on new attribute elements.
	 * More precisely :
	 *  - a Face attribute is created, if needed, for the new face.
	 */
	Face add_face(uint32 size)
	{
		CGOGN_CHECK_CONCRETE_TYPE;

		cgogn_message_assert(size == 3u, "Can create only triangles");
		if (size != 3)
			cgogn_log_warning("add_face") << "Attempt to create a face which is not a triangle";

		const Face f(add_tri_topo());

		if (this->template is_embedded<CDart>())
		{
			foreach_dart_of_orbit(f, [this] (Dart d)
			{
				this->new_orbit_embedding(CDart(d));
			});
		}

		if (this->template is_embedded<Vertex>())
		{
			foreach_dart_of_orbit(f, [this] (Dart d)
			{
				this->new_orbit_embedding(Vertex(d));
			});
		}

		if (this->template is_embedded<Edge>())
		{
			foreach_dart_of_orbit(f, [this] (Dart d)
			{
				this->new_orbit_embedding(Edge(d));
			});
		}

		if (this->template is_embedded<Face>())
			this->new_orbit_embedding(f);

		if (this->template is_embedded<Volume>())
			this->new_orbit_embedding(Volume(f.dart));

		return f;
	}

protected:

	inline Dart add_tetra_topo(uint32 size)
	{
		cgogn_message_assert(size > 0u, "The pyramid cannot be empty");

		Dart f1 = this->Inherit::add_tri_topo();
		Dart f2 = this->Inherit::add_tri_topo();
		Dart f3 = this->Inherit::add_tri_topo();
		Dart f4 = this->Inherit::add_tri_topo();

		this->phi2_sew(this->phi_1(f2), f1);
		this->phi2_sew(this->phi_1(f3), f2);
		this->phi2_sew(this->phi_1(f1), f3);

		this->phi2_sew(this->phi1(f1), f4);
		f4 = this->phi1(f4);
		this->phi2_sew(this->phi1(f2), f4);
		f4 = this->phi1(f4);
		this->phi2_sew(this->phi1(f3), f4);

		return f1;
	}


protected:

	/**
	 * @brief Flip an edge
	 * @param d : a dart of the edge to flip
	 * @return true if the edge has been flipped, false otherwise
	 * Each end of the edge is detached from its initial vertex
	 * and inserted in the next vertex within its incident face.
	 * An end of the edge that is a vertex of degree 1 is not moved.
	 * If one of the faces have co-degree 1 then nothing is done.
	 */
	inline bool flip_edge_topo(Dart d)
	{
		Dart e = phi2(d);
		if (!this->is_boundary(d) && !this->is_boundary(e))
		{
			Dart d1  = this->phi1(d);
			Dart d11 = this->phi1(d1);
			Dart e1  = this->phi1(e);
			Dart e11 = this->phi1(e1);

			Dart xd1  = phi2(d1);
			Dart xd11 = phi2(d11);
			Dart xe1  = phi2(e1);
			Dart xe11 = phi2(e11);

			phi2_sew_nocheck(d1,xd11);
			phi2_sew_nocheck(d11,xe1);
			phi2_sew_nocheck(e1,xe11);
			phi2_sew_nocheck(e11,xd1);

			return true;
		}
		return false;
	}

public:

	/**
	 * @brief Flip an edge
	 * @param ed : the edge to flip
	 * The two endpoints of the given edge are moved to the next vertices
	 * of their two adjacent faces
	 */
	inline void flip_edge(Edge e)
	{
		CGOGN_CHECK_CONCRETE_TYPE;

		if (flip_edge_topo(e.dart))
		{
			Dart d = e.dart;
			Dart dd = phi2(d);

			Dart d1  = this->phi1(d);
			Dart d11 = this->phi1(d1);
			Dart dd1  = this->phi1(dd);
			Dart dd11 = this->phi1(dd1);

			if (this->template is_embedded<Vertex>())
			{
				this->template copy_embedding<Vertex>(d1, phi<21>(d1));
				this->template copy_embedding<Vertex>(d11, phi<21>(d11));
				this->template copy_embedding<Vertex>(dd1, phi<21>(dd1));
				this->template copy_embedding<Vertex>(dd11, phi<21>(dd11));
				this->template copy_embedding<Vertex>(d, dd1);
				this->template copy_embedding<Vertex>(dd, d11);
			}

			if (this->template is_embedded<Edge>())
			{
				this->template copy_embedding<Edge>(d1, phi2(d1));
				this->template copy_embedding<Edge>(d11, phi2(d11));
				this->template copy_embedding<Edge>(dd1, phi2(dd1));
				this->template copy_embedding<Edge>(dd11, phi2(dd11));
			}
		}
	}

protected:

	/**
	 * @brief Collapse an edge
	 * @param d : a dart of the edge to collapse
	 * @return a dart of the resulting vertex
	 */
	inline Dart collapse_edge_topo(Dart d)
	{
		Dart res = phi2(this->phi_1(d));
		Dart e = phi2(d);

		phi2_sew_nocheck(phi<12>(d),phi2(this->phi_1(d)));
		phi2_sew_nocheck(phi<12>(e),phi2(this->phi_1(e)));

		return res;
	}

public:

	/**
	 * @brief Collapse an edge
	 * @param e : the edge to collapse
	 * @return the resulting vertex
	 */
	inline Vertex collapse_edge(Edge e)
	{
		CGOGN_CHECK_CONCRETE_TYPE;

		// dart for edge of one side
		Dart d2 = phi2(this->phi_1(phi2(e.dart)));
		// dart for edge of other side, and vertex
		Dart d1 = collapse_edge_topo(e.dart);


		Vertex v(d1);
		if (this->template is_embedded<Vertex>())
		{
			uint32 emb = this->get_embedding(v);
			foreach_dart_of_orbit(v, [this, emb] (Dart d) { this->template set_embedding<Vertex>(d, emb); });
		}

		if (this->template is_embedded<Edge>())
		{
			this->template copy_embedding<Edge>(d1, phi2(d1));
			this->template copy_embedding<Edge>(d2, phi2(d2));
		}

		return v;
	}

protected:

	/*!
	 * \brief Close the topological hole that contains Dart d (a fixed point for PHI2).
	 * \param d : a vertex of the hole
	 * \return a vertex of the face that closes the hole
	 * This method is used to close a CMap2 that has been build through the 2-sewing of 1-faces.
	 * A face is inserted on the boundary that begin at dart d.
	 */
	inline Dart close_hole_topo(Dart d)
	{
		cgogn_message_assert(phi2(d) == d, "CMap2: close hole called on a dart that is not a phi2 fix point");

		Dart first = Inherit::add_tri_topo();	// First edge of the face that will fill the hole
		phi2_sew(d, first);				// 2-sew the new edge to the hole

		Dart prec_tri = first;
		Dart d_next = d;				// Turn around the hole
		Dart d_phi1;					// to complete the face
		do
		{
			do
			{
				d_phi1 = this->phi1(d_next); // Search and put in d_next
				d_next = phi2(d_phi1); // the next dart of the hole
			} while (d_next != d_phi1 && d_phi1 != d);

			if (d_phi1 != d)
			{
				Dart tri = Inherit::add_tri_topo();	// First edge of the face that will fill the hole
				phi2_sew(d_next, tri);
				phi2_sew(this->phi_1(prec_tri), this->phi1(tri));
				prec_tri = tri;
			}
		} while (d_phi1 != d);

		phi2_sew(this->phi_1(prec_tri), this->phi1(first));

		return first;
	}

	/*******************************************************************************
	 * Connectivity information
	 *******************************************************************************/

public:

	inline uint32 degree(Vertex v) const
	{
		return this->nb_darts_of_orbit(v);
	}

	inline uint32 codegree(Edge e) const
	{
		if (this->phi1(e.dart) == e.dart)
			return 1;
		else
			return 2;
	}

	inline uint32 degree(Edge e) const
	{
		if (this->is_boundary(e.dart) || this->is_boundary(phi2(e.dart)))
			return 1;
		else
			return 2;
	}

	inline uint32 codegree(Face f) const
	{
		return Inherit::codegree(f);
	}

	inline uint32 degree(Face) const
	{
		return 1;
	}

	inline uint32 codegree(Volume v) const
	{
		uint32 result = 0;
		foreach_incident_face(v, [&result] (Face) { ++result; });
		return result;
	}

	/*******************************************************************************
	 * Boundary information
	 *******************************************************************************/

	bool is_adjacent_to_boundary(Boundary c)
	{
		CGOGN_CHECK_CONCRETE_TYPE;

		bool result = false;
		foreach_dart_of_orbit_until(c, [this, &result] (Dart d)
		{
			if (this->is_boundary(phi2(d))) { result = true; return false; }
			return true;
		});
		return result;
	}

	/*******************************************************************************
	 * Orbits traversal
	 *******************************************************************************/

protected:

	template <typename FUNC>
	inline void foreach_dart_of_PHI2(Dart d, const FUNC& f) const
	{
		f(d);
		f(phi2(d));
	}

	template <typename FUNC>
	inline void foreach_dart_of_PHI21(Dart d, const FUNC& f) const
	{
		Dart it = d;
		do
		{
			f(it);
			it = phi2(this->phi_1(it));
		} while (it != d);
	}

	template <typename FUNC>
	void foreach_dart_of_PHI1_PHI2(Dart d, const FUNC& f) const
	{
		DartMarkerStore marker(*this);

		std::vector<Dart>* visited_faces = cgogn::get_dart_buffers()->get_buffer();
		visited_faces->push_back(d); // Start with the face of d

		// For every face added to the list
		for(uint32 i = 0; i < visited_faces->size(); ++i)
		{
			Dart e = (*visited_faces)[i];
			if (!marker.is_marked(e))	// Face has not been visited yet
			{
				// mark visited darts (current face)
				// and add non visited adjacent faces to the list of face
				Dart it = e;
				do
				{
					f(it); // apply the function to the darts of the face
					marker.mark(it);				// Mark
					Dart adj = phi2(it);			// Get adjacent face
					if (!marker.is_marked(adj))
						visited_faces->push_back(adj);	// Add it
					it = this->phi1(it);
				} while (it != e);
			}
		}
		cgogn::get_dart_buffers()->release_buffer(visited_faces);
	}

	template <Orbit ORBIT, typename FUNC>
	inline void foreach_dart_of_orbit(Cell<ORBIT> c, const FUNC& f) const
	{
		static_assert(check_func_parameter_type(FUNC, Dart), "Wrong function parameter type");
		static_assert(ORBIT == Orbit::DART || ORBIT == Orbit::PHI1 || ORBIT == Orbit::PHI2 ||
					  ORBIT == Orbit::PHI1_PHI2 || ORBIT == Orbit::PHI21,
					  "Orbit not supported in a CMap2");

		switch (ORBIT)
		{
			case Orbit::DART: f(c.dart); break;
			case Orbit::PHI1: this->foreach_dart_of_PHI1(c.dart, f); break;
			case Orbit::PHI2: foreach_dart_of_PHI2(c.dart, f); break;
			case Orbit::PHI1_PHI2: foreach_dart_of_PHI1_PHI2(c.dart, f); break;
			case Orbit::PHI21: foreach_dart_of_PHI21(c.dart, f); break;
			case Orbit::PHI2_PHI3:
			case Orbit::PHI1_PHI3:
			case Orbit::PHI21_PHI31:
			case Orbit::PHI1_PHI2_PHI3:
			default: cgogn_assert_not_reached("Orbit not supported in a CMap2"); break;
		}
	}

	template <typename FUNC>
	inline void foreach_dart_of_PHI2_until(Dart d, const FUNC& f) const
	{
		if (f(d))
			f(phi2(d));
	}

	template <typename FUNC>
	inline void foreach_dart_of_PHI21_until(Dart d, const FUNC& f) const
	{
		Dart it = d;
		do
		{
			if (!f(it))
				break;
			it = phi2(this->phi_1(it));
		} while (it != d);
	}

	template <typename FUNC>
	void foreach_dart_of_PHI1_PHI2_until(Dart d, const FUNC& f) const
	{
		DartMarkerStore marker(*this);

		std::vector<Dart>* visited_faces = cgogn::get_dart_buffers()->get_buffer();
		visited_faces->push_back(d); // Start with the face of d

		// For every face added to the list
		for(uint32 i = 0; i < visited_faces->size(); ++i)
		{
			Dart e = (*visited_faces)[i];
			if (!marker.is_marked(e))	// Face has not been visited yet
			{
				// mark visited darts (current face)
				// and add non visited adjacent faces to the list of face
				Dart it = e;
				do
				{
					if (!f(it)) // apply the function to the darts of the face
					{
						cgogn::get_dart_buffers()->release_buffer(visited_faces);
						return;
					}
					marker.mark(it);				// Mark
					Dart adj = phi2(it);			// Get adjacent face
					if (!marker.is_marked(adj))
						visited_faces->push_back(adj);	// Add it
					it = this->phi1(it);
				} while (it != e);
			}
		}
		cgogn::get_dart_buffers()->release_buffer(visited_faces);
	}

	template <Orbit ORBIT, typename FUNC>
	inline void foreach_dart_of_orbit_until(Cell<ORBIT> c, const FUNC& f) const
	{
		static_assert(check_func_parameter_type(FUNC, Dart), "Wrong function parameter type");
		static_assert(check_func_return_type(FUNC, bool), "Wrong function return type");

		static_assert(ORBIT == Orbit::DART || ORBIT == Orbit::PHI1 || ORBIT == Orbit::PHI2 ||
					  ORBIT == Orbit::PHI1_PHI2 || ORBIT == Orbit::PHI21,
					  "Orbit not supported in a CMap2");

		switch (ORBIT)
		{
			case Orbit::DART: f(c.dart); break;
			case Orbit::PHI1: this->foreach_dart_of_PHI1_until(c.dart, f); break;
			case Orbit::PHI2: foreach_dart_of_PHI2_until(c.dart, f); break;
			case Orbit::PHI1_PHI2: foreach_dart_of_PHI1_PHI2_until(c.dart, f); break;
			case Orbit::PHI21: foreach_dart_of_PHI21_until(c.dart, f); break;
			case Orbit::PHI2_PHI3:
			case Orbit::PHI1_PHI3:
			case Orbit::PHI21_PHI31:
			case Orbit::PHI1_PHI2_PHI3:
			default: cgogn_assert_not_reached("Orbit not supported in a CMap2"); break;
		}
	}

	/*******************************************************************************
	 * Incidence traversal
	 *******************************************************************************/

public:

	template <typename FUNC>
	inline void foreach_incident_edge(Vertex v, const FUNC& func) const
	{
		static_assert(check_func_parameter_type(FUNC, Edge), "Wrong function cell parameter type");
		foreach_dart_of_orbit(v, [&] (Dart d)
		{
			if (!this->is_boundary(d) && !this->is_boundary(phi2(d)))
				func(Edge(d));
		});
	}

	template <typename FUNC>
	inline void foreach_incident_face(Vertex v, const FUNC& func) const
	{
		static_assert(check_func_parameter_type(FUNC, Face), "Wrong function cell parameter type");
		foreach_dart_of_orbit(v, [this, &func] (Dart d)
		{
			if (!this->is_boundary(d))
				func(Face(d));
		});
	}

	template <typename FUNC>
	inline void foreach_incident_volume(Vertex v, const FUNC& func) const
	{
		static_assert(check_func_parameter_type(FUNC, Volume), "Wrong function cell parameter type");
		func(Volume(v.dart));
	}

	template <typename FUNC>
	inline void foreach_incident_vertex(Edge e, const FUNC& func) const
	{
		static_assert(check_func_parameter_type(FUNC, Vertex), "Wrong function cell parameter type");
		foreach_dart_of_orbit(e, [&func] (Dart d){ func(Vertex(d)); });
	}

	template <typename FUNC>
	inline void foreach_incident_face(Edge e, const FUNC& func) const
	{
		static_assert(check_func_parameter_type(FUNC, Face), "Wrong function cell parameter type");
		foreach_dart_of_orbit(e, [this, &func] (Dart d)
		{
			if (!this->is_boundary(d))
				func(Face(d));
		});
	}

	template <typename FUNC>
	inline void foreach_incident_volume(Edge e, const FUNC& func) const
	{
		static_assert(check_func_parameter_type(FUNC, Volume), "Wrong function cell parameter type");
		func(Volume(e.dart));
	}

	template <typename FUNC>
	inline void foreach_incident_vertex(Face f, const FUNC& func) const
	{
		static_assert(check_func_parameter_type(FUNC, Vertex), "Wrong function cell parameter type");
		foreach_dart_of_orbit(f, [&func] (Dart d) { func(Vertex(d)); });
	}

	template <typename FUNC>
	inline void foreach_incident_edge(Face f, const FUNC& func) const
	{
		static_assert(check_func_parameter_type(FUNC, Edge), "Wrong function cell parameter type");
		foreach_dart_of_orbit(f, [&func] (Dart d) { func(Edge(d)); });
	}

	template <typename FUNC>
	inline void foreach_incident_volume(Face f, const FUNC& func) const
	{
		static_assert(check_func_parameter_type(FUNC, Volume), "Wrong function cell parameter type");
		func(Volume(f.dart));
	}

	template <typename FUNC>
	inline void foreach_incident_vertex(Volume w, const FUNC& func) const
	{
		static_assert(check_func_parameter_type(FUNC, Vertex), "Wrong function cell parameter type");
		DartMarkerStore marker(*this);
		foreach_dart_of_orbit(w, [&] (Dart d)
		{
			if (!marker.is_marked(d))
			{
				marker.mark_orbit(Vertex(d));
				func(Vertex(d));
			}
		});
	}

	template <typename FUNC>
	inline void foreach_incident_edge(Volume w, const FUNC& func) const
	{
		static_assert(check_func_parameter_type(FUNC, Edge), "Wrong function cell parameter type");
		DartMarkerStore marker(*this);
		foreach_dart_of_orbit(w, [&] (Dart d)
		{
			if (!marker.is_marked(d))
			{
				marker.mark_orbit(Edge(d));
				func(Edge(d));
			}
		});
	}

	template <typename FUNC>
	inline void foreach_incident_face(Volume w, const FUNC& func) const
	{
		static_assert(check_func_parameter_type(FUNC, Face), "Wrong function cell parameter type");
		DartMarkerStore marker(*this);
		foreach_dart_of_orbit(w, [&] (Dart d)
		{
			if (!marker.is_marked(d) && !this->is_boundary(d))
			{
				marker.mark_orbit(Face(d));
				func(Face(d));
			}
		});
	}

	/*******************************************************************************
	 * Adjacence traversal
	 *******************************************************************************/

	template <typename FUNC>
	inline void foreach_adjacent_vertex_through_edge(Vertex v, const FUNC& f) const
	{
		static_assert(check_func_parameter_type(FUNC, Vertex), "Wrong function cell parameter type");
		foreach_dart_of_orbit(v, [this, &f] (Dart d)
		{
			if (!this->is_boundary(d) && !this->is_boundary(phi2(d)))
				f(Vertex(this->phi2(d)));
		});
	}

	template <typename FUNC>
	inline void foreach_adjacent_vertex_through_face(Vertex v, const FUNC& f) const
	{
		static_assert(check_func_parameter_type(FUNC, Vertex), "Wrong function cell parameter type");
		foreach_dart_of_orbit(v, [this, &f] (Dart vd)
		{
			if (!this->is_boundary(vd))
			{
				Dart vd1 = this->phi1(vd);
				this->foreach_dart_of_orbit(Face(vd), [&f, vd, vd1] (Dart fd)
				{
					// skip Vertex v itself and its first successor around current face
					if (fd != vd && fd != vd1)
						f(Vertex(fd));
				});
			}
		});
	}

	template <typename FUNC>
	inline void foreach_adjacent_edge_through_vertex(Edge e, const FUNC& f) const
	{
		static_assert(check_func_parameter_type(FUNC, Edge), "Wrong function cell parameter type");
		foreach_dart_of_orbit(e, [&f, this] (Dart ed)
		{
			this->foreach_dart_of_orbit(Vertex(ed), [&, ed] (Dart vd)
			{
				if (!this->is_boundary(vd) && !this->is_boundary(phi2(vd)))
					// skip Edge e itself
					if (vd != ed)
						f(Edge(vd));
			});
		});
	}

	template <typename FUNC>
	inline void foreach_adjacent_edge_through_face(Edge e, const FUNC& f) const
	{
		static_assert(check_func_parameter_type(FUNC, Edge), "Wrong function cell parameter type");
		foreach_dart_of_orbit(e, [&f, this] (Dart ed)
		{
			if (!this->is_boundary(ed))
			{
				this->foreach_dart_of_orbit(Face(ed), [&f, ed] (Dart fd)
				{
					// skip Edge e itself
					if (fd != ed)
						f(Edge(fd));
				});
			}
		});
	}

	template <typename FUNC>
	inline void foreach_adjacent_face_through_vertex(Face f, const FUNC& func) const
	{
		static_assert(check_func_parameter_type(FUNC, Face), "Wrong function cell parameter type");
		foreach_dart_of_orbit(f, [this, &func] (Dart fd)
		{
			Dart fd1 = this->phi2(this->phi_1(fd));
			this->foreach_dart_of_orbit(Vertex(fd), [this, &func, fd, fd1] (Dart vd)
			{
				// skip Face f itself and its first successor around current vertex
				if (vd != fd && vd != fd1 && !this->is_boundary(vd))
					func(Face(vd));
			});
		});
	}

	template <typename FUNC>
	inline void foreach_adjacent_face_through_edge(Face f, const FUNC& func) const
	{
		static_assert(check_func_parameter_type(FUNC, Face), "Wrong function cell parameter type");
		foreach_dart_of_orbit(f, [this, &func] (Dart d)
		{
			const Dart d2 = this->phi2(d);
			if (!this->is_boundary(d2))
				func(Face(d2));
		});
	}

	inline std::pair<Vertex,Vertex> vertices(Edge e) const
	{
		return std::pair<Vertex, Vertex>(Vertex(e.dart), Vertex(this->phi1(e.dart)));
	}

protected:

#define FOR_ALL_ORBITS( CODE) {\
{static const Orbit orbit_const=DART; CODE }\
{static const Orbit orbit_const=PHI1; CODE }\
{static const Orbit orbit_const=PHI2; CODE }\
{static const Orbit orbit_const=PHI1_PHI2; CODE }\
{static const Orbit orbit_const=PHI21; CODE }}


	/**
	 * @brief check if embedding of map is also embedded in this (create if not). Used by merge method
	 * @param map
	 */
	void merge_check_embedding(const Self& map)
	{
		FOR_ALL_ORBITS
		(
			if (!this->template is_embedded<orbit_const>() && map.template is_embedded<orbit_const>())
				this->template create_embedding<orbit_const>();
		)
	}


	/**
	 * @brief ensure all cells (introduced while merging) are embedded.
	 * @param first index of first dart to scan
	 */
	void merge_finish_embedding(uint32 first)
	{
		FOR_ALL_ORBITS
		(
			if (this->template is_embedded<orbit_const>())
			{
				for (uint32 j=first; j!= this->topology_.end(); this->topology_.next(j))
				{
					if ( ((orbit_const != Boundary::ORBIT) && (orbit_const != DART)) || (!this->is_boundary(Dart(j))))
						if ((*this->embeddings_[orbit_const])[j] == INVALID_INDEX)
						{
							this->new_orbit_embedding(Cell<orbit_const>(Dart(j)));
						}
				}
			}
		)
	}

#undef FOR_ALL_ORBITS

};

template <typename MAP_TRAITS>
struct CMap2TriType
{
	using TYPE = CMap2Tri_T<MAP_TRAITS, CMap2TriType<MAP_TRAITS>>;
};

template <typename MAP_TRAITS>
using CMap2Tri = CMap2Tri_T<MAP_TRAITS, CMap2TriType<MAP_TRAITS>>;

#if defined(CGOGN_USE_EXTERNAL_TEMPLATES) && (!defined(CGOGN_CORE_MAP_MAP2_TRI_CPP_))
extern template class CGOGN_CORE_API CMap2Tri_T<DefaultMapTraits, CMap2TriType<DefaultMapTraits>>;
extern template class CGOGN_CORE_API DartMarker<CMap2Tri<DefaultMapTraits>>;
extern template class CGOGN_CORE_API DartMarkerStore<CMap2Tri<DefaultMapTraits>>;
extern template class CGOGN_CORE_API DartMarkerNoUnmark<CMap2Tri<DefaultMapTraits>>;
extern template class CGOGN_CORE_API CellMarker<CMap2Tri<DefaultMapTraits>, CMap2Tri<DefaultMapTraits>::Vertex::ORBIT>;
extern template class CGOGN_CORE_API CellMarker<CMap2Tri<DefaultMapTraits>, CMap2Tri<DefaultMapTraits>::Edge::ORBIT>;
extern template class CGOGN_CORE_API CellMarker<CMap2Tri<DefaultMapTraits>, CMap2Tri<DefaultMapTraits>::Face::ORBIT>;
extern template class CGOGN_CORE_API CellMarker<CMap2Tri<DefaultMapTraits>, CMap2Tri<DefaultMapTraits>::Volume::ORBIT>;
extern template class CGOGN_CORE_API CellMarkerStore<CMap2Tri<DefaultMapTraits>, CMap2Tri<DefaultMapTraits>::Vertex::ORBIT>;
extern template class CGOGN_CORE_API CellMarkerStore<CMap2Tri<DefaultMapTraits>, CMap2Tri<DefaultMapTraits>::Edge::ORBIT>;
extern template class CGOGN_CORE_API CellMarkerStore<CMap2Tri<DefaultMapTraits>, CMap2Tri<DefaultMapTraits>::Face::ORBIT>;
extern template class CGOGN_CORE_API CellMarkerStore<CMap2Tri<DefaultMapTraits>, CMap2Tri<DefaultMapTraits>::Volume::ORBIT>;
#endif // defined(CGOGN_USE_EXTERNAL_TEMPLATES) && (!defined(CGOGN_CORE_MAP_MAP2_CPP_))

} // namespace cgogn

#include <cgogn/core/cmap/cmap2_tri_builder.h>

#endif // CGOGN_CORE_CMAP_CMAP2_H_
