
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

#ifndef RENDERING_SHADERS_VBO_H_
#define RENDERING_SHADERS_VBO_H_

#include <QOpenGLBuffer>

#include <core/cmap/attribute_handler.h>
#include <geometry/types/geometry_traits.h>

namespace cgogn
{

namespace rendering
{

class VBO
{
protected:

	unsigned int nb_vectors_;
	unsigned int vector_dimension_;
	QOpenGLBuffer buffer_;

public:

	inline VBO(unsigned int vec_dim) :
		nb_vectors_(),
		vector_dimension_(vec_dim)
	{
		buffer_.create();
		buffer_.bind();
		buffer_.setUsagePattern(QOpenGLBuffer::StreamDraw);
	}

	inline ~VBO()
	{
		buffer_.destroy();
	}

	inline void bind()
	{
		buffer_.bind();
	}

	inline void release()
	{
		buffer_.release();
	}

	/**
	 * @brief allocate VBO memory
	 * @param nb_vectors number of vectors
	 * @param vector_dimension_ number of component of each vector
	 */
	inline void allocate(int nb_vectors, unsigned int vector_dimension)
	{
		buffer_.bind();
		unsigned int total = nb_vectors * vector_dimension;
		if (total != nb_vectors_ * vector_dimension_) // only allocate when > ?
			buffer_.allocate(total * sizeof(float));
		nb_vectors_ = nb_vectors;
		if (vector_dimension != vector_dimension_)
		{
			vector_dimension_ = vector_dimension;
			std::cout << " Warning, changing the VBO vector_dimension"<< std::endl;
		}
		buffer_.release();
	}

	/**
	 * @brief get and lock pointer on buffer memory
	 * @return  the pointer
	 */
	inline float* lock_pointer()
	{
		buffer_.bind();
		return reinterpret_cast<float*>(buffer_.map(QOpenGLBuffer::ReadWrite));
	}

	/**
	 * @brief release_pointer
	 */
	inline void release_pointer()
	{
		buffer_.unmap();
	}

	/**
	 * @brief copy data
	 * @param offset offset in bytes in the bufffer
	 * @param nb number of bytes to copy
	 * @param src source pointer
	 */
	inline void copy_data(unsigned int offset, unsigned int nb, void* src)
	{
		buffer_.write(offset, src, nb);
	}

	/**
	 * @brief dimension of vectors stored in buffer
	 */
	inline int vector_dimension() const
	{
		return vector_dimension_;
	}

	unsigned int size() const
	{
		return nb_vectors_;
	}
};

/**
 * @brief update vbo from one AttributeHandler
 * @param attr AttributeHandler (must contain float or vec<float>
 * @param vbo vbo to update
 * @param convert conversion lambda
 */
template <typename ATTR>
void update_vbo(const ATTR& attr, VBO& vbo)
{
	static_assert(std::is_same<typename geometry::vector_traits<typename ATTR::value_type>::Scalar, float>::value || std::is_same<typename geometry::vector_traits<typename ATTR::value_type>::Scalar, double>::value, "only float or double allowed for vbo");

	const typename ATTR::TChunkArray* ca = attr.get_data();

	std::vector<void*> chunk_addr;
	unsigned int byte_chunk_size;
	unsigned int nb_chunks = ca->get_chunks_pointers(chunk_addr, byte_chunk_size);
	const unsigned int vec_dim = geometry::nb_components_traits<typename ATTR::value_type>::value;

	vbo.allocate(nb_chunks * ATTR::CHUNKSIZE, vec_dim);

	const unsigned int vbo_blk_bytes =  ATTR::CHUNKSIZE * vec_dim * sizeof(float);

	if (std::is_same<typename geometry::vector_traits<typename ATTR::value_type>::Scalar, float>::value)
	{
		// copy
		vbo.bind();
		for (unsigned int i = 0; i < nb_chunks; ++i)
			vbo.copy_data(i* vbo_blk_bytes, vbo_blk_bytes, chunk_addr[i]);
		vbo.release();
	}
	else if (std::is_same<typename geometry::vector_traits<typename ATTR::value_type>::Scalar, double>::value)
	{
		// copy (after conversion to float)
		float* float_buffer = new float[ATTR::CHUNKSIZE * vec_dim];
		for (unsigned int i = 0; i < nb_chunks; ++i)
		{
			// transform double into float
			float* fit = float_buffer;
			double* src = reinterpret_cast<double*>(chunk_addr[i]);
			for (unsigned int j = 0; j < ATTR::CHUNKSIZE * vec_dim; ++j)
				*fit++ = *src++;
			vbo.bind();
			vbo.copy_data(i* ATTR::CHUNKSIZE * vec_dim * sizeof(float), ATTR::CHUNKSIZE * vec_dim * sizeof(float),float_buffer);
			vbo.release();
		}
		delete[] float_buffer;
	}
}


/**
 * @brief update vbo from one AttributeHandler with conversion lambda
 * @param attr AttributeHandler
 * @param vbo vbo to update
 * @param convert conversion lambda -> float or std::array<float,2/3/4>
 */
template <typename ATTR, typename FUNC>
void update_vbo(const ATTR& attr, VBO& vbo, const FUNC& convert)
{
	// check that convert has 1 param
	static_assert(function_traits<FUNC>::arity == 1, "convert lambda function must have only one arg");

	// check that convert param  is compatible with attr
	using InputConvert = typename std::remove_cv< typename std::remove_reference<typename function_traits<FUNC>::template arg<0>::type>::type >::type;
	static_assert(std::is_same<InputConvert,inside_type(ATTR) >::value, "wrong parameter 1");

	// get chunk data pointers
	const typename ATTR::TChunkArray* ca = attr.get_data();
	std::vector<void*> chunk_addr;
	unsigned int byte_chunk_size;
	unsigned int nb_chunks = ca->get_chunks_pointers(chunk_addr, byte_chunk_size);

	// check that out of convert is float or std::array<float,2/3/4>
	using Vec2f = std::array<float,2>;
	using Vec3f = std::array<float,3>;
	using Vec4f = std::array<float,4>;
	static_assert(check_func_return_type(FUNC,float) || check_func_return_type(FUNC,Vec2f) || check_func_return_type(FUNC,Vec3f) ||check_func_return_type(FUNC,Vec4f), "convert output must be float or std::array<float,2/3/4>" );

	// set vec dimension
	unsigned int vec_dim = 0;
	if (check_func_return_type(FUNC,float) )
		vec_dim = 1;
	else if (check_func_return_type(FUNC,Vec2f) )
		vec_dim = 2;
	else if (check_func_return_type(FUNC,Vec3f) )
		vec_dim = 3;
	else if (check_func_return_type(FUNC,Vec4f) )
		vec_dim = 4;

	vbo.allocate(nb_chunks * ATTR::CHUNKSIZE, vec_dim);

	// copy (after conversion)
	using OutputConvert = typename function_traits<FUNC>::result_type;
	OutputConvert* dst = reinterpret_cast<OutputConvert*>(vbo.lock_pointer());
	for (unsigned int i = 0; i < nb_chunks; ++i)
	{
		InputConvert* typed_chunk = static_cast<InputConvert*>(chunk_addr[i]);
		for (unsigned int j = 0; j < ATTR::CHUNKSIZE; ++j)
			*dst++ = convert(typed_chunk[j]);
	}
	vbo.release_pointer();
}


/**
 * @brief update vbo from two AttributeHandlers with conversion lambda
 * @param attr first AttributeHandler
 * @param attr2 second AttributeHandler
 * @param vbo vbo to update
 * @param convert conversion lambda -> float or std::array<float,2/3/4>
 */
template <typename ATTR, typename ATTR2, typename FUNC>
void update_vbo(const ATTR& attr, const ATTR2& attr2, VBO& vbo, const FUNC& convert)
{
	// check that convert has 2 param
	static_assert(function_traits<FUNC>::arity == 2, "convert lambda function must have two arg");

	//check that attr & attr2 are on same orbit
	static_assert(ATTR::orbit_value == ATTR2::orbit_value, "attributes must be on same orbit");

	// check that convert param 1 is compatible with attr
	using InputConvert = typename std::remove_cv< typename std::remove_reference<typename function_traits<FUNC>::template arg<0>::type>::type >::type;
	static_assert(std::is_same<InputConvert,inside_type(ATTR) >::value, "wrong parameter 1");

	// check that convert param 2 is compatible with attr2
	using InputConvert2 = typename std::remove_cv< typename std::remove_reference<typename function_traits<FUNC>::template arg<1>::type>::type >::type;
	static_assert(std::is_same<InputConvert,inside_type(ATTR2) >::value, "wrong parameter 2");

	// get chunk data pointers
	const typename ATTR::TChunkArray* ca = attr.get_data();
	std::vector<void*> chunk_addr;
	unsigned int byte_chunk_size;
	unsigned int nb_chunks = ca->get_chunks_pointers(chunk_addr, byte_chunk_size);

	const typename ATTR::TChunkArray* ca2 = attr2.get_data();
	std::vector<void*> chunk_addr2;
	ca2->get_chunks_pointers(chunk_addr2, byte_chunk_size);

	// check that out of convert is float or std::array<float,2/3/4>
	using Vec2f = std::array<float,2>;
	using Vec3f = std::array<float,3>;
	using Vec4f = std::array<float,4>;
	static_assert(check_func_return_type(FUNC,float) || check_func_return_type(FUNC,Vec2f) || check_func_return_type(FUNC,Vec3f) ||check_func_return_type(FUNC,Vec4f), "convert output must be float or std::array<float,2/3/4>" );

	// set vec dimension
	unsigned int vec_dim = 0;
	if (check_func_return_type(FUNC,float) )
		vec_dim = 1;
	else if (check_func_return_type(FUNC,Vec2f) )
		vec_dim = 2;
	else if (check_func_return_type(FUNC,Vec3f) )
		vec_dim = 3;
	else if (check_func_return_type(FUNC,Vec4f) )
		vec_dim = 4;

	// allocate vbo
	vbo.allocate(nb_chunks * ATTR::CHUNKSIZE, vec_dim);

	// copy (after conversion)
	// out type conversion
	using OutputConvert = typename function_traits<FUNC>::result_type;
	OutputConvert* dst = reinterpret_cast<OutputConvert*>(vbo.lock_pointer());
	for (unsigned int i = 0; i < nb_chunks; ++i)
	{
		InputConvert* typed_chunk = static_cast<InputConvert*>(chunk_addr[i]);
		InputConvert2* typed_chunk2 = static_cast<InputConvert2*>(chunk_addr2[i]);
		for (unsigned int j = 0; j < ATTR::CHUNKSIZE; ++j)
			*dst++ = convert(typed_chunk[j],typed_chunk2[j]);
	}
	vbo.release_pointer();
}



/**
 * @brief generate a vbo from an attribute and it's indices
 * @param attr the AttributeHandler
 * @param indices indices in the AttributeHandler
 * @param vbo the vbo to generate
 * @param convert conversion lambda  -> float or std::array<float,2/3/4>
 */
template <typename ATTR, typename FUNC>
void generate_vbo(const ATTR& attr, const std::vector<unsigned int>& indices, VBO& vbo, const FUNC& convert)
{
	// check that convert has 1 param
	static_assert(function_traits<FUNC>::arity == 1, "convert lambda function must have only one arg");

	// check that convert param  is compatible with attr
	using InputConvert = typename std::remove_cv< typename std::remove_reference<typename function_traits<FUNC>::template arg<0>::type>::type >::type;
	static_assert(std::is_same<InputConvert,inside_type(ATTR) >::value, "wrong parameter 1");

	// check that out of convert is float or std::array<float,2/3/4>
	using Vec2f = std::array<float,2>;
	using Vec3f = std::array<float,3>;
	using Vec4f = std::array<float,4>;
	static_assert(check_func_return_type(FUNC,float) || check_func_return_type(FUNC,Vec2f) || check_func_return_type(FUNC,Vec3f) ||check_func_return_type(FUNC,Vec4f), "convert output must be float or std::array<float,2/3/4>" );

	// set vec dimension
	unsigned int vec_dim = 0;
	if (check_func_return_type(FUNC,float) )
		vec_dim = 1;
	else if (check_func_return_type(FUNC,Vec2f) )
		vec_dim = 2;
	else if (check_func_return_type(FUNC,Vec3f) )
		vec_dim = 3;
	else if (check_func_return_type(FUNC,Vec4f) )
		vec_dim = 4;

	// allocate vbo
	vbo.allocate(static_cast<unsigned int>(indices.size()), vec_dim);

	// copy (after conversion)
	using OutputConvert = typename function_traits<FUNC>::result_type;
	OutputConvert* dst = reinterpret_cast<OutputConvert*>(vbo.lock_pointer());

	for (unsigned int i: indices)
		 *dst++ = convert(attr[i]);

	vbo.release_pointer();
}






} // namespace rendering

} // namespace cgogn

#endif // RENDERING_SHADERS_VBO_H_