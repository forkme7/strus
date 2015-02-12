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
#ifndef _STRUS_LVDB_DOCUMENT_FREQUENCY_CACHE_HPP_INCLUDED
#define _STRUS_LVDB_DOCUMENT_FREQUENCY_CACHE_HPP_INCLUDED
#include "strus/index.hpp"
#include <cstring>
#include <stdexcept>
#include <vector>
#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>

namespace strus {

class DocumentFrequencyCache
{
public:
	class Batch;

	DocumentFrequencyCache()
		:m_locked(false)
	{}
	~DocumentFrequencyCache()
	{}

	void writeBatch( const Batch& batch);

	GlobalCounter getValue( const Index& typeno, const Index& termno) const;

	void clear();

public:
	class Batch
	{
	public:
		class Increment;

		void put( const Index& typeno, const Index termno, int incr)
		{
			m_ar.push_back( Increment( typeno, termno, incr));
		}
		void put( const Increment& o)
		{
			m_ar.push_back( o);
		}
		typedef std::vector<Increment>::const_iterator const_iterator;

		const_iterator begin() const	{return m_ar.begin();}
		const_iterator end() const	{return m_ar.begin();}

	public:
		class Increment
		{
		public:
			Increment( const Index& typeno_, const Index termno_, int value_)
				:typeno(typeno_),termno(termno_),value(value_){}
			Increment( const Increment& o)
				:typeno(o.typeno),termno(o.termno),value(o.value){}

			Index typeno;
			Index termno;
			int value;
		};

	private:
		std::vector<Increment> m_ar;
	};

private:
	enum {
		MaxNofTermTypes=256,		///< maximum number of term types
		InitNodeSize=1024		///< initial node size of the cache
	};

	class CounterArray
	{
	public:
		~CounterArray()
		{
			if (m_ar) std::free( m_ar);
		}

		explicit CounterArray( std::size_t size_)
			:m_ar((GlobalCounter*)std::calloc( size_, sizeof(GlobalCounter)))
			,m_size(size_)
		{
			if (!m_ar) throw std::bad_alloc();
		}

		explicit CounterArray( const CounterArray& o, std::size_t size_)
			:m_ar(0),m_size(size_)
		{
			std::size_t copy_size = (size_ < o.m_size)?size_:o.m_size;
			if (m_size * sizeof(GlobalCounter) < m_size) throw std::bad_alloc();
			m_ar = (GlobalCounter*)std::malloc( m_size * sizeof(GlobalCounter));
			if (!m_ar) throw std::bad_alloc();

			std::memcpy( m_ar, o.m_ar, copy_size * sizeof(GlobalCounter));
			std::memset( m_ar + copy_size, 0, m_size - copy_size);
		}
		const GlobalCounter& operator[]( std::size_t idx) const		{if (idx >= m_size) throw std::runtime_error("internal: array bound read (document frequency cache)"); return m_ar[idx];}
		GlobalCounter& operator[]( std::size_t idx)			{if (idx >= m_size) throw std::runtime_error("internal: array bound write (document frequency cache)"); return m_ar[idx];}

		std::size_t size() const					{return m_size;}

	private:
		GlobalCounter* m_ar;
		std::size_t m_size;
	};

	typedef boost::shared_ptr<CounterArray> CounterArrayRef;

	void doIncrement( const Batch::Increment& incr);
	void doRevertIncrement( const Batch::Increment& incr);

private:
	boost::mutex m_mutex;
	bool m_locked;
	CounterArrayRef m_ar[ MaxNofTermTypes];
};

}//namespace
#endif

