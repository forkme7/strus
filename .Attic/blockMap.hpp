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
#ifndef _STRUS_LVDB_BLOCK_MAP_TEMPLATE_HPP_INCLUDED
#define _STRUS_LVDB_BLOCK_MAP_TEMPLATE_HPP_INCLUDED
#include "strus/index.hpp"
#include "dataBlock.hpp"
#include "blockStorage.hpp"
#include "databaseKey.hpp"
#include "blockKey.hpp"
#include <cstdlib>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>

namespace strus {

template <class BlockType, class BlockElementMap>
class BlockMap
{
public:
	BlockMap( leveldb::DB* db_)
		:m_db(db_){}
	BlockMap( const BlockMap& o)
		:m_db(o.m_db),m_map(o.m_map){}

	void defineElement(
		const BlockKey& dbkey,
		const Index& elemid,
		const typename BlockElementMap::mapped_type& elem)
	{
		typename Map::iterator mi = m_map.find( dbkey.index());
		if (mi == m_map.end())
		{
			BlockElementMap em;
			em.define( elemid, elem);
			m_map[ dbkey.index()] = em;
		}
		else
		{
			mi->second.define( elemid, elem);
		}
	}

	void deleteElement(
		const BlockKey& dbkey,
		const Index& elemid)
	{
		defineElement( dbkey, elemid, typename BlockElementMap::mapped_type());
	}

	void getWriteBatchMerge( leveldb::WriteBatch& batch)
	{
		typename Map::const_iterator mi = m_map.begin(), me = m_map.end();
		for (; mi != me; ++mi)
		{
			typename BlockElementMap::const_iterator
				ei = mi->second.begin(),
				ee = mi->second.end();

			if (ei == ee) continue;
			Index lastInsertBlockId = mi->second.lastInsertBlockId();

			BlockStorage<BlockType> blkstorage( m_db, BlockKey(mi->first), false);
			BlockType newblk;

			// [1] Merge new elements with existing upper bound blocks:
			mergeNewElements( blkstorage, ei, ee, newblk, batch);

			// [2] Write the new blocks that could not be merged into existing ones:
			insertNewElements( blkstorage, ei, ee, newblk, lastInsertBlockId, batch);
		}
		m_map.clear();
	}

	void getWriteBatchReplace( leveldb::WriteBatch& batch)
	{
		typename Map::const_iterator mi = m_map.begin(), me = m_map.end();
		for (; mi != me; ++mi)
		{
			typename BlockElementMap::const_iterator
				ei = mi->second.begin(),
				ee = mi->second.end();

			if (ei == ee) continue;
			Index lastInsertBlockId = mi->second.lastInsertBlockId();

			BlockStorage<BlockType> blkstorage( m_db, BlockKey(mi->first), false);
			const BlockType* blk;
			BlockType newblk;

			// [1] Delete all old blocks with the database key as prefix address:
			for (blk = blkstorage.load( 0);
				blk != 0; blk = blkstorage.loadNext())
			{
				blkstorage.dispose( blk->id(), batch);
			}
			// [2] Write the new blocks:
			insertNewElements( blkstorage, ei, ee, newblk, lastInsertBlockId, batch);
		}
		m_map.clear();
	}

	template <class ElementMap, class Element>
	void getElementOccurrencies(
		std::map<Element,Index>& result,
		const BlockKey& dbkey,
		ElementMap& map)
	{
		BlockStorage<BlockType> blkstorage( m_db, dbkey, false);

		Index blkidx = 0;
		const BlockType* blk = blkstorage.load( blkidx);

		for (; blk; blk = blkstorage.load( blkidx))
		{
			blkidx = blk->id()+1;
			typename BlockType::const_iterator bi = blk->begin(), be = blk->end();
			for (; bi != be; ++bi)
			{
				result[ map( *bi)] += 1;
			}
		}
	}

	template <class Renamer>
	void renameKeys( Renamer& renamer)
	{
		typename Map::iterator mi = m_map.begin(), me = m_map.end();
		while (mi != me)
		{
			BlockKey dbkey( mi->first);

			if (renamer.isCandidate( mi->first))
			{
				BlockKeyIndex newkey = renamer.map( mi->first);
				BlockElementMap& newelem = m_map[ newkey];
				newelem.swap( mi->second);
				m_map.erase( mi++);
			}
			else
			{
				++mi;
			}
		}
	}

private:
	typedef std::map<BlockKeyIndex,BlockElementMap> Map;

private:
	void insertNewElements( BlockStorage<BlockType>& blkstorage,
				typename BlockElementMap::const_iterator& ei,
				const typename BlockElementMap::const_iterator& ee,
				BlockType& newblk,
				const Index& lastInsertBlockId,
				leveldb::WriteBatch& batch)
	{
		if (newblk.id() < lastInsertBlockId)
		{
			newblk.setId( lastInsertBlockId);
		}
		Index blkid = newblk.id();
		for (; ei != ee; ++ei)
		{
			if (newblk.full())
			{
				newblk.setId( blkid);
				blkstorage.store( newblk, batch);
				newblk.clear();
				newblk.setId( lastInsertBlockId);
			}
			newblk.append( ei->key(), ei->value());
			blkid = ei->key();
		}
		if (!newblk.empty())
		{
			newblk.setId( blkid);
			blkstorage.store( newblk, batch);
		}
	}

	void mergeNewElements( BlockStorage<BlockType>& blkstorage,
				typename BlockElementMap::const_iterator& ei,
				const typename BlockElementMap::const_iterator& ee,
				BlockType& newblk,
				leveldb::WriteBatch& batch)
	{
		const BlockType* blk;
		while (ei != ee && 0!=(blk=blkstorage.load( ei->key())))
		{
			BlockType elemblk;
			elemblk.setId( blk->id());

			for (; ei != ee && ei->key() <= blk->id(); ++ei)
			{
				elemblk.append( ei->key(), ei->value());
			}
			newblk = BlockType::merge( elemblk, *blk);
			if (blkstorage.loadNext())
			{
				// ... is not the last block, so we store it
				blkstorage.store( newblk, batch);
				newblk.clear();
			}
			else
			{
				if (newblk.full())
				{
					// ... it is the last block, but full
					blkstorage.store( newblk, batch);
					newblk.clear();
				}
				else
				{
					blkstorage.dispose( elemblk.id(), batch);
				}
				break;
			}
		}
		if (newblk.empty())
		{
			// Fill first new block with elements of last 
			// block and dispose the last block:
			if (ei != ee &&  0!=(blk=blkstorage.loadLast()))
			{
				newblk.initcopy( *blk);
				blkstorage.dispose( blk->id(), batch);
			}
		}
	}

private:
	leveldb::DB* m_db;
	Map m_map;
};

}
#endif

