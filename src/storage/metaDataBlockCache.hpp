/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_METADATA_BLOCK_CACHE_HPP_INCLUDED
#define _STRUS_STORAGE_METADATA_BLOCK_CACHE_HPP_INCLUDED
#include "strus/index.hpp"
#include "metaDataBlock.hpp"
#include "metaDataRecord.hpp"
#include "databaseAdapter.hpp"
#include <utility>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include "private/utils.hpp"

namespace strus {

/// \brief Forward declaration
class DatabaseClientInterface;

class MetaDataBlockCache
{
public:
	MetaDataBlockCache( DatabaseClientInterface* database, const MetaDataDescription& descr_);

	~MetaDataBlockCache(){}

	const MetaDataRecord get( const Index& docno);

	void declareVoid( const Index& blockno);
	void refresh();

private:
	void resetBlock( const Index& blockno);

private:
	enum {
		CacheSize=(1024*1024),				///< size of the cache in blocks
		MaxDocno=(CacheSize*MetaDataBlock::BlockSize)	///< hardcode limit of maximum document number
	};

private:
	DatabaseClientInterface* m_database;
	MetaDataDescription m_descr;
	DatabaseAdapter_DocMetaData m_dbadapter;
	utils::SharedPtr<MetaDataBlock> m_ar[ CacheSize];
	std::vector<unsigned int> m_voidar;
};

}
#endif

