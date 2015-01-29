/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#include "dataBlock.hpp"
#include <cstring>
#include <stdexcept>
#include <map>
#include <iostream>
#include <algorithm>

using namespace strus;

#define BLOCK_ALLOC_SIZE(minNofBytes)  (((((minNofBytes) + 1024-1) / 1024)) * 1024)

DataBlock::~DataBlock()
{
	if (m_allocsize) std::free( m_ptr);
}

void DataBlock::init( const Index& id_, const void* ptr_, std::size_t size_, std::size_t allocsize_)
{
	if (allocsize_)
	{
		std::size_t mm = BLOCK_ALLOC_SIZE( size_);
		void* newptr = std::malloc( mm);
		if (!newptr) throw std::bad_alloc();
		std::memcpy( newptr, ptr_, size_);
		if (m_allocsize) std::free( m_ptr);

		m_ptr = (char*)newptr;
		m_allocsize = mm;
	}
	else
	{
		if (m_allocsize) std::free( m_ptr);

		m_ptr = static_cast<char*>(const_cast<void*>( ptr_));
		m_allocsize = 0;
	}
	m_size = size_;
	m_id = id_;
}

void DataBlock::initcopy( const DataBlock& o)
{
	if (m_type != o.m_type) throw std::logic_error( "block type mismatch in initcopy");
	init( o.m_id, o.m_ptr, o.m_size, o.m_size /*force copy*/);
}

void DataBlock::swap( DataBlock& o)
{
	std::swap( m_type, o.m_type);
	std::swap( m_id, o.m_id);
	std::swap( m_ptr, o.m_ptr);
	std::swap( m_size, o.m_size);
	std::swap( m_allocsize, o.m_allocsize);
}

void DataBlock::expand( std::size_t datasize)
{
	std::size_t aa = datasize + m_size;
	std::size_t mm = 1024;
	while (mm <= aa && mm != 0) mm *= 2;
	if (!mm) throw std::bad_alloc();
	char* mp;

	if (m_allocsize)
	{
		mp = (char*)std::realloc( m_ptr, mm);
		if (!mp) throw std::bad_alloc();
	}
	else
	{
		mp = (char*)std::calloc( mm, 1);
		if (!mp) throw std::bad_alloc();
		std::memcpy( mp, m_ptr, m_size);
	}
	m_allocsize = mm;
	m_ptr = mp;
}

void DataBlock::append( const void* data, std::size_t datasize)
{
	if (datasize + m_size > m_allocsize)
	{
		expand( datasize);
	}
	std::memcpy( m_ptr+m_size, data, datasize);
	m_size += datasize;
}

void DataBlock::fill( char ch, std::size_t datasize)
{
	if (datasize + m_size > m_allocsize)
	{
		expand( datasize);
	}
	std::memset( m_ptr+m_size, ch, datasize);
	m_size += datasize;
}

