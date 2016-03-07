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

#ifndef CORE_CONTAINER_CHUNK_ARRAY_FACTORY_H_
#define CORE_CONTAINER_CHUNK_ARRAY_FACTORY_H_

#include <core/utils/unique_ptr.h>
#include <core/utils/name_types.h>
#include <core/container/chunk_array.h>

#include <iostream>
#include <map>
#include <memory>
#include <array>

namespace cgogn
{

template <unsigned int CHUNKSIZE>
class ChunkArrayFactory
{
	static_assert(CHUNKSIZE >= 1u,"ChunkSize must be at least 1");
	static_assert(!(CHUNKSIZE & (CHUNKSIZE - 1)),"CHUNKSIZE must be a power of 2");

public:
	using ChunkArrayGenPtr = std::unique_ptr< ChunkArrayGen<CHUNKSIZE> >;
	using NamePtrMap = std::map<std::string, ChunkArrayGenPtr>;
	using UniqueNamePtrMap = std::unique_ptr<NamePtrMap>;

	static UniqueNamePtrMap map_CA_;

	/**
	 * @brief register a type
	 * @param keyType name of type
	 * @param obj a ptr on object (new ChunkArray<32,int> for example) ptr will be deleted by clean method
	 */
	template <typename T>
	static void register_CA()
	{
		// could be moved in register_known_types (dangerous) ??
		if (!map_CA_)
			reset();

		std::string&& keyType(name_of_type(T()));
		if(map_CA_->find(keyType) == map_CA_->end())
			(*map_CA_)[std::move(keyType)] = make_unique<ChunkArray<CHUNKSIZE, T>>();
	}

	static void register_known_types()
	{
		static bool known_types_initialized_ = false;

		if (known_types_initialized_)
			return;

		register_CA<bool>();
		register_CA<char>();
		register_CA<short>();
		register_CA<int>();
		register_CA<long>();
		register_CA<long long>();
		register_CA<signed char>();
		register_CA<unsigned char>();
		register_CA<unsigned short>();
		register_CA<unsigned int>();
		register_CA<unsigned long>();
		register_CA<unsigned long long>();
		register_CA<float>();
		register_CA<double>();
		register_CA<std::string>();
		register_CA<std::array<double,3>>();
		register_CA<std::array<float,3>>();
		// NOT TODO : add Eigen.

		known_types_initialized_ = true;
	}

	/**
	 * @brief create a ChunkArray from a typename
	 * @param keyType typename of type store in ChunkArray
	 * @return ptr on created ChunkArray
	 */
	static ChunkArrayGen<CHUNKSIZE>* create(const std::string& keyType)
	{
		ChunkArrayGen<CHUNKSIZE>* tmp = nullptr;
		typename NamePtrMap::const_iterator it = map_CA_->find(keyType);

		if(it != map_CA_->end())
		{
			tmp = (it->second)->clone();
		}
		else
			std::cerr << "type " << keyType << " not registred in ChunkArrayFactory" << std::endl;

		return tmp;
	}

	static void reset()
	{
		ChunkArrayFactory<CHUNKSIZE>::map_CA_ = make_unique<NamePtrMap>();
	}
};

template <unsigned int CHUNKSIZE>
typename ChunkArrayFactory<CHUNKSIZE>::UniqueNamePtrMap ChunkArrayFactory<CHUNKSIZE>::map_CA_ = nullptr;


#if defined(CGOGN_USE_EXTERNAL_TEMPLATES) && (!defined(CORE_CONTAINER_CHUNK_ARRAY_FACTORY_CPP_))
extern template class CGOGN_CORE_API ChunkArrayFactory<DEFAULT_CHUNK_SIZE>;
#endif // defined(CGOGN_USE_EXTERNAL_TEMPLATES) && (!defined(CORE_CONTAINER_CHUNK_ARRAY_FACTORY_CPP_))

} // namespace cgogn

#endif // CORE_CONTAINER_CHUNK_ARRAY_FACTORY_H_
