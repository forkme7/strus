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
#ifndef _STRUS_LVDB_METADATA_BLOCK_MAP_HPP_INCLUDED
#define _STRUS_LVDB_METADATA_BLOCK_MAP_HPP_INCLUDED
#include "strus/index.hpp"
#include "metaDataBlock.hpp"
#include "metaDataRecord.hpp"
#include "localStructAllocator.hpp"
#include "strus/arithmeticVariant.hpp"
#include <cstdlib>
#include <vector>
#include <boost/shared_ptr.hpp>

namespace strus {
/// \brief Forward declaration
class MetaDataBlockCache;
/// \brief Forward declaration
class DatabaseInterface;
/// \brief Forward declaration
class DatabaseTransactionInterface;


class MetaDataMap
{
public:
	MetaDataMap( DatabaseInterface* database_, const MetaDataDescription* descr_)
		:m_database(database_),m_descr(descr_){}
	~MetaDataMap();

	void defineMetaData( Index docno, const std::string& varname, const ArithmeticVariant& value);
	void deleteMetaData( Index docno);
	void deleteMetaData( Index docno, const std::string& varname);

	void getWriteBatch( DatabaseTransactionInterface* transaction, std::vector<Index>& cacheRefreshList);
	void rewriteMetaData(
			const MetaDataDescription::TranslationMap& trmap,
			const MetaDataDescription& newDescr,
			DatabaseTransactionInterface* transaction);

private:
	MetaDataRecord getRecord( Index docno);

private:
	typedef std::pair<Index,Index> MetaDataKey;
	typedef LocalStructAllocator<std::pair<MetaDataKey,ArithmeticVariant> > MapAllocator;
	typedef std::less<MetaDataKey> MapCompare;
	typedef std::map<MetaDataKey,ArithmeticVariant,MapCompare,MapAllocator> Map;

private:
	DatabaseInterface* m_database;
	const MetaDataDescription* m_descr;
	Map m_map;
};

}
#endif
