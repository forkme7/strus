/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#ifndef _STRUS_LVDB_DOCUMENT_FREQUENCY_MAP_HPP_INCLUDED
#define _STRUS_LVDB_DOCUMENT_FREQUENCY_MAP_HPP_INCLUDED
#include "strus/index.hpp"
#include "private/localStructAllocator.hpp"
#include "documentFrequencyCache.hpp"
#include <cstdlib>
#include <map>

namespace strus {

/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class DatabaseTransactionInterface;
/// \brief Forward declaration
class StatisticsBuilderInterface;
/// \brief Forward declaration
class KeyMapInv;

class DocumentFrequencyMap
{
public:
	DocumentFrequencyMap( DatabaseClientInterface* database_)
		:m_database(database_){}

	void increment( Index typeno, Index termno, Index count=1);
	void decrement( Index typeno, Index termno, Index count=1);

	void renameNewTermNumbers( const std::map<Index,Index>& renamemap);

	void getWriteBatch(
			DatabaseTransactionInterface* transaction,
			StatisticsBuilderInterface* statisticsBuilder,
			DocumentFrequencyCache::Batch* dfbatch,
			const KeyMapInv& termTypeMapInv,
			const KeyMapInv& termValueMapInv);

	void clear();

private:
	typedef std::pair<Index,Index> Key;
	typedef LocalStructAllocator<std::pair<Key,int> > MapAllocator;
	typedef std::less<Key> MapCompare;
	typedef std::map<Key,int,MapCompare, MapAllocator> Map;

private:
	DatabaseClientInterface* m_database;
	Map m_map;
};

}//namespace
#endif


