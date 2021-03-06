/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "invertedIndexMap.hpp"
#include "booleanBlockBatchWrite.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "private/internationalization.hpp"
#include "keyMap.hpp"
#include "databaseAdapter.hpp"
#include "indexPacker.hpp"
#include <sstream>
#include <limits>

using namespace strus;

InvertedIndexMap::InvertedIndexMap( DatabaseClientInterface* database_)
	:m_dfmap(database_),m_database(database_),m_docno(0)
{
	m_posinfo.push_back( 0);
}

void InvertedIndexMap::clear()
{
	m_dfmap.clear();
	m_map.clear();
	m_posinfo.clear();
	m_posinfo.push_back( 0);
	m_invtermmap.clear();
	m_invterms.clear();
	m_docno = 0;
	m_docno_deletes.clear();
	m_docno_typeno_deletes.clear();
}

void InvertedIndexMap::definePosinfoPosting(
	const Index& termtype,
	const Index& termvalue,
	const Index& docno,
	const std::vector<Index>& pos)
{
	if (pos.empty()) return;

	if (termtype == 0 || termvalue == 0)
	{
		throw strus::runtime_error( _TXT( "calling definePosinfoPosting with illegal arguments: (%u,%u,%u)"), termtype, termvalue, docno);
	}
	MapKey key( termtype, termvalue, docno);
	Map::const_iterator mi = m_map.find( key);

	if (mi == m_map.end() || mi->second == 0)
	{
		if (pos.size())
		{
			if (pos.size() > std::numeric_limits<PosinfoBlock::PositionType>::max())
			{
				throw strus::runtime_error( _TXT( "size of document out of range (max %u)"), 65535);
			}
			m_map[ key] = m_posinfo.size();

			m_posinfo.push_back( (PosinfoBlock::PositionType)pos.size());	//... ff
			std::vector<Index>::const_iterator pi = pos.begin(), pe = pos.end();
			for (; pi != pe; ++pi)
			{
				if (*pi > std::numeric_limits<PosinfoBlock::PositionType>::max())
				{
					throw strus::runtime_error( _TXT( "token position out of range (max %u)"), 65535);
				}
				else
				{
					m_posinfo.push_back( (PosinfoBlock::PositionType)*pi);
				}
			}
		}
		else
		{
			m_map[ key] = 0;
		}
	}
	else
	{
		// create exception message:
		std::ostringstream posmsg;
		std::vector<Index>::const_iterator pi = pos.begin(), pe = pos.end();
		for (int pidx=0;pi != pe; ++pi,++pidx)
		{
			if (pidx) posmsg << ",";
			posmsg << *pi;
		}
		std::size_t posidx = mi->second;
		unsigned int fi=0,fe=m_posinfo[posidx++];
		posmsg << " | ";
		for (; fi != fe; ++fi)
		{
			if (!fi) posmsg << ",";
			posmsg << m_posinfo[posidx+fi];
		}
		std::string posstr = posmsg.str();
		// throw exception message:
		throw strus::runtime_error( _TXT( "document feature defined twice [termtype=%u,termvalue=%u,docno=%u] %s"), termtype, termvalue, docno, posstr.c_str());
	}
	InvTermMap::const_iterator vi = m_invtermmap.find( docno);
	if (vi == m_invtermmap.end())
	{
		if (docno == 0) throw strus::runtime_error( "%s", _TXT( "illegal document number for insert (posinfo)"));
		if (m_invterms.size()) m_invterms.push_back( InvTerm());

		m_invtermmap[ m_docno = docno] = m_invterms.size();
		m_invterms.push_back( InvTerm( termtype, termvalue, pos.size(), pos.empty()?0:pos[0]));
	}
	else
	{
		if (m_docno && m_docno != docno)
		{
			throw strus::runtime_error( "%s", _TXT( "inverted index operations not grouped by document"));
		}
		m_invterms.push_back( InvTerm( termtype, termvalue, pos.size(), pos.empty()?0:pos[0]));
	}
}

void InvertedIndexMap::deleteIndex( const Index& docno)
{
	InvTermMap::iterator vi = m_invtermmap.find( docno);
	if (vi != m_invtermmap.end())
	{
		InvTermList::const_iterator li = m_invterms.begin() + vi->second, le = m_invterms.end();
		for (; li != le && li->typeno; ++li)
		{
			MapKey key( li->typeno, li->termno, docno);
			Map::iterator mi = m_map.find( key);
			if (mi != m_map.end())
			{
				m_map.erase( mi);
			}
		}
		m_invtermmap.erase( vi);
	}
	m_docno_deletes.insert( docno);
}

void InvertedIndexMap::deleteIndex( const Index& docno, const Index& typeno)
{
	InvTermMap::iterator vi = m_invtermmap.find( docno);
	if (vi != m_invtermmap.end())
	{
		InvTermList::const_iterator li = m_invterms.begin() + vi->second, le = m_invterms.end();
		InvTermList::iterator write_li = m_invterms.begin();
		for (; li != le && li->typeno; ++li)
		{
			if (typeno == li->typeno)
			{
				MapKey key( li->typeno, li->termno, docno);
				Map::iterator mi = m_map.find( key);
				if (mi != m_map.end())
				{
					m_map.erase( mi);
				}
			}
			else
			{
				*write_li++ = *li;
			}
		}
		if (write_li != m_invterms.end())
		{
			*write_li = InvTerm();
		}
	}
	m_docno_typeno_deletes[ docno].insert( typeno);
}

void InvertedIndexMap::renameNewNumbers(
		const std::map<Index,Index>& docnoUnknownMap,
		const std::map<Index,Index>& termUnknownMap)
{
	// Rename terms:
	Map::iterator mi = m_map.begin(), me = m_map.end();
	while (mi != me)
	{
		Index termno = BlockKey(mi->first.termkey).elem(2);
		Index docno = mi->first.docno;
		if (KeyMap::isUnknown( termno) || KeyMap::isUnknown( docno))
		{
			Index typeno = BlockKey( mi->first.termkey).elem(1);
			Index new_termno;
			if (KeyMap::isUnknown( termno))
			{
				std::map<Index,Index>::const_iterator ri = termUnknownMap.find( termno);
				if (ri == termUnknownMap.end())
				{
					throw strus::runtime_error( _TXT( "%s value undefined (%s)"), "term", "posinfo map");
				}
				new_termno = ri->second;
			}
			else
			{
				new_termno = termno;
			}
			Index new_docno;
			if (KeyMap::isUnknown( docno))
			{
				std::map<Index,Index>::const_iterator ri = docnoUnknownMap.find( docno);
				if (ri == docnoUnknownMap.end())
				{
					throw strus::runtime_error( _TXT( "%s value undefined (%s)"), "docno", "posinfo map");
				}
				new_docno = ri->second;
			}
			else
			{
				new_docno = docno;
			}
			MapKey newkey( typeno, new_termno, new_docno);
			m_map[ newkey] = mi->second;
			m_map.erase( mi++);
		}
		else
		{
			++mi;
		}
	}
	// Rename inv:
	InvTermList::iterator li = m_invterms.begin(), le = m_invterms.end();
	for (; li != le; ++li)
	{
		if (KeyMap::isUnknown( li->termno))
		{
			std::map<Index,Index>::const_iterator ri = termUnknownMap.find( li->termno);
			if (ri == termUnknownMap.end())
			{
				throw strus::runtime_error( _TXT( "%s value undefined (%s)"), "term", "inv posinfo map");
			}
			li->termno = ri->second;
		}
	}
	InvTermMap::iterator di = m_invtermmap.begin(), de = m_invtermmap.end();
	while (di != de)
	{
		if (KeyMap::isUnknown( di->first))
		{
			std::map<Index,Index>::const_iterator ri = docnoUnknownMap.find( di->first);
			if (ri == docnoUnknownMap.end())
			{
				throw strus::runtime_error( _TXT( "%s value undefined (%s)"), "docno", "inv posinfo map");
			}
			m_invtermmap[ ri->second] = di->second;
			m_invtermmap.erase( di++);
		}
		else
		{
			++di;
		}
	}
	// Rename df:
	m_dfmap.renameNewTermNumbers( termUnknownMap);
}

void InvertedIndexMap::getWriteBatch(
			DatabaseTransactionInterface* transaction,
			StatisticsBuilderInterface* statisticsBuilder,
			DocumentFrequencyCache::Batch* dfbatch,
			const KeyMapInv& termTypeMapInv,
			const KeyMapInv& termValueMapInv)
{
	DatabaseAdapter_InverseTerm::ReadWriter dbadapter_inv( m_database);
	// [1] Get deletes:
	{
		std::set<Index>::const_iterator di = m_docno_deletes.begin(), de = m_docno_deletes.end();
		for (; di != de; ++di)
		{
			InvTermBlock invblk;
			if (dbadapter_inv.load( *di, invblk))
			{
				char const* ei = invblk.begin();
				const char* ee = invblk.end();
				for (;ei != ee; ei = invblk.next( ei))
				{
					InvTerm it = invblk.element_at( ei);
	
					// ensure old search index elements are deleted:
					MapKey key( it.typeno, it.termno, *di);
					m_map[ key];	//... construct member (default 0) if it does not exist
							// <=> mark as deleted, if not member of set of inserts
	
					// decrement stats for all old elements, for the new elements it will be incremented again:
					m_dfmap.decrement( it.typeno, it.termno);
				}
				dbadapter_inv.remove( transaction, *di);
			}
		}
	}{
		std::map<Index, std::set<Index> >::const_iterator ui = m_docno_typeno_deletes.begin(), ue = m_docno_typeno_deletes.end();
		for (; ui != ue; ++ui)
		{
			InvTermBlock invblk;
			if (dbadapter_inv.load( ui->first, invblk))
			{
				if (m_invterms.size()) m_invterms.push_back( InvTerm());
				std::size_t newinvtermidx = m_invterms.size();
				char const* ei = invblk.begin();
				const char* ee = invblk.end();
				for (;ei != ee; ei = invblk.next( ei))
				{
					InvTerm it = invblk.element_at( ei);
	
					if (ui->second.find( it.typeno) != ui->second.end())
					{
						// ensure old search index elements are deleted:
						MapKey key( it.typeno, it.termno, ui->first);
						m_map[ key];	//... construct member (default 0) if it does not exist
								// <=> mark as deleted, if not member of set of inserts
					}
					else
					{
						// Ensure that all elements that are not part of a partial delete are readded again:
						m_invterms.push_back( it);
					}
					// decrement stats for all old elements, for the new elements it will be incremented again:
					m_dfmap.decrement( it.typeno, it.termno);
				}
				InvTermMap::iterator iitr = m_invtermmap.find( ui->first);
				if (iitr != m_invtermmap.end())
				{
					std::size_t li = iitr->second, le = m_invterms.size();
					for (; li != le && m_invterms[li].typeno; ++li)
					{
						if (ui->second.find( m_invterms[li].typeno) == ui->second.end())
						{
							throw strus::runtime_error( "%s", _TXT("mixing partial update with insert is not allowed"));
						}
						m_invterms.push_back( m_invterms[li]);
					}
				}
				m_invtermmap[ ui->first] = newinvtermidx;
				dbadapter_inv.remove( transaction, ui->first);
			}
		}
	}{
		// [2] Get inv and df map inserts:
		InvTermMap::const_iterator vi = m_invtermmap.begin(), ve = m_invtermmap.end();
		for (; vi != ve; ++vi)
		{
			InvTermBlock invblk;
			invblk.setId( vi->first);
			InvTermList::const_iterator li = m_invterms.begin() + vi->second, le = m_invterms.end();
	
			for (; li != le && li->typeno; ++li)
			{
				// inv blk:
				invblk.append( li->typeno, li->termno, li->ff, li->firstpos);
				// df map:
				m_dfmap.increment( li->typeno, li->termno);
			}
			dbadapter_inv.store( transaction, invblk);
		}
	}{
		// [3] Get index inserts and term deletes (defined in [1]):
		Map::const_iterator mi = m_map.begin(), me = m_map.end();
		while (mi != me)
		{
			Map::const_iterator
				ei = mi,
				ee = mi;
			for (; ee != me && ee->first.termkey == ei->first.termkey; ++ee){}
			mi = ee;
	
			BlockKey blkkey( ei->first.termkey);
			Index typeno = blkkey.elem(1);
			Index termno = blkkey.elem(2);
			DatabaseAdapter_PosinfoBlock::WriteCursor dbadapter_posinfo( m_database, typeno, termno);
	
			PosinfoBlockBuilder newposblk;
			std::vector<BooleanBlock::MergeRange> docrangear;
	
			// [1] Merge new elements with existing upper bound blocks:
			mergeNewPosElements( dbadapter_posinfo, transaction, ei, ee, newposblk, docrangear);
	
			// [2] Write the new blocks that could not be merged into existing ones:
			insertNewPosElements( dbadapter_posinfo, transaction, ei, ee, newposblk, docrangear);
	
			BooleanBlock newdocblk;
	
			std::vector<BooleanBlock::MergeRange>::iterator
				di = docrangear.begin(),
				de = docrangear.end();
			Index lastInsertBlockId = docrangear.back().to;
	
			// [3] Update document list of the term (boolean block) in the database:
			DatabaseAdapter_DocListBlock::WriteCursor dbadapter_doclist( m_database, typeno, termno);
	
			// [3.1] Merge new docno boolean block elements
			BooleanBlockBatchWrite::mergeNewElements( &dbadapter_doclist, di, de, newdocblk, transaction);
	
			// [3.2] Insert new docno boolean block elements
			BooleanBlockBatchWrite::insertNewElements( &dbadapter_doclist, di, de, newdocblk, lastInsertBlockId, transaction);
		}
	}{
		// [4] Get df writes (and df changes to populate, if statisticsBuilder defined):
		m_dfmap.getWriteBatch( transaction, statisticsBuilder, dfbatch, termTypeMapInv, termValueMapInv);
	}
}

void InvertedIndexMap::defineDocnoRangeElement(
		std::vector<BooleanBlock::MergeRange>& docrangear,
		const Index& docno,
		bool isMember)
{
	if (docrangear.empty())
	{
		docrangear.push_back( BooleanBlock::MergeRange( docno, docno, isMember));
	}
	else
	{
		if (docrangear.back().isMember == isMember && docrangear.back().to+1 == docno)
		{
			docrangear.back().to += 1;
		}
		else
		{
			docrangear.push_back( BooleanBlock::MergeRange( docno, docno, isMember));
		}
	}
}

void InvertedIndexMap::insertNewPosElements(
		DatabaseAdapter_PosinfoBlock::WriteCursor& dbadapter_posinfo, 
		DatabaseTransactionInterface* transaction,
		Map::const_iterator& ei,
		const Map::const_iterator& ee,
		PosinfoBlockBuilder& newposblk,
		std::vector<BooleanBlock::MergeRange>& docrangear)
{
	while (ei != ee)
	{
		if (ei->second)
		{
			// Define posinfo block elements (PosinfoBlock):
			if (newposblk.fitsInto( m_posinfo[ ei->second] + 1) || newposblk.empty())
			{
				// Define docno list block elements (BooleanBlock):
				defineDocnoRangeElement( docrangear, ei->first.docno, true);

				// Define posinfo block elements (PosinfoBlock):
				newposblk.append( ei->first.docno, m_posinfo.data() + ei->second);
				++ei;
			}
			else
			{
				if (!newposblk.empty())
				{
					dbadapter_posinfo.store( transaction, newposblk.createBlock());
				}
				newposblk.clear();
			}
		}
		else
		{
			// Delete docno list block element (BooleanBlock):
			defineDocnoRangeElement( docrangear, ei->first.docno, false);
		}
	}
	if (!newposblk.empty())
	{
		dbadapter_posinfo.store( transaction, newposblk.createBlock());
		newposblk.clear();
	}
}

void InvertedIndexMap::mergeNewPosElements(
		DatabaseAdapter_PosinfoBlock::WriteCursor& dbadapter_posinfo,
		DatabaseTransactionInterface* transaction,
		Map::const_iterator& ei,
		const Map::const_iterator& ee,
		PosinfoBlockBuilder& newposblk,
		std::vector<BooleanBlock::MergeRange>& docrangear)
{
	PosinfoBlock blk;
	while (ei != ee && dbadapter_posinfo.loadUpperBound( ei->first.docno, blk))
	{
		// Merge posinfo block elements (PosinfoBlock):
		Map::const_iterator newposblk_start = ei;
		for (; ei != ee && ei->first.docno <= blk.id(); ++ei)
		{
			// Define docno list block elements (BooleanBlock):
			defineDocnoRangeElement( docrangear, ei->first.docno, ei->second?true:false);
		}

		mergePosBlock( dbadapter_posinfo, transaction, newposblk_start, ei, blk, newposblk);
		if (dbadapter_posinfo.loadNext( blk))
		{
			// ... is not the last block, so we store it
			if (!newposblk.empty())
			{
				dbadapter_posinfo.store( transaction, newposblk.createBlock());
			}
			newposblk.clear();
		}
		else
		{
			if (newposblk.full())
			{
				// ... it is the last block, but full
				dbadapter_posinfo.store( transaction, newposblk.createBlock());
				newposblk.clear();
			}
			else
			{
				dbadapter_posinfo.remove( transaction, blk.id());
				newposblk.setId(0);
			}
			break;
		}
	}
	if (newposblk.empty())
	{
		// Fill first new block with elements of last 
		// block and dispose the last block:
		if (ei != ee && dbadapter_posinfo.loadLast( blk))
		{
			newposblk = PosinfoBlockBuilder( blk);
			dbadapter_posinfo.remove( transaction, blk.id());
			newposblk.setId(0);
		}
	}
}

void InvertedIndexMap::mergePosBlock( 
		DatabaseAdapter_PosinfoBlock::WriteCursor& dbadapter_posinfo,
		DatabaseTransactionInterface* transaction,
		Map::const_iterator ei,
		const Map::const_iterator& ee,
		const PosinfoBlock& oldblk,
		PosinfoBlockBuilder& newblk)
{
	newblk.setId( oldblk.id());
	PosinfoBlock::Cursor blkcursor;

	Index old_docno = oldblk.firstDoc( blkcursor);
	while (ei != ee && old_docno)
	{
		if (ei->first.docno <= old_docno)
		{
			if (ei->second)
			{
				//... append only if not empty (empty => delete)
				if (!newblk.fitsInto( m_posinfo[ ei->second]))
				{
					newblk.setId(0);
					if (!newblk.empty())
					{
						dbadapter_posinfo.store( transaction, newblk.createBlock());
					}
					newblk.clear();
					newblk.setId( oldblk.id());
				}
				newblk.append( ei->first.docno, m_posinfo.data() + ei->second);
			}
			if (ei->first.docno == old_docno)
			{
				//... defined twice -> prefer new entry and ignore old
				old_docno = oldblk.nextDoc( blkcursor);
			}
			++ei;
		}
		else
		{
			newblk.append( old_docno, oldblk.posinfo_at( blkcursor));
			old_docno = oldblk.nextDoc( blkcursor);
		}
	}
	while (ei != ee)
	{
		if (ei->second)
		{
			//... append only if not empty (empty => delete)
			if (!newblk.fitsInto( m_posinfo[ ei->second]))
			{
				newblk.setId(0);
				if (!newblk.empty())
				{
					dbadapter_posinfo.store( transaction, newblk.createBlock());
				}
				newblk.clear();
				newblk.setId( oldblk.id());
			}
			newblk.append( ei->first.docno, m_posinfo.data() + ei->second);
		}
		++ei;
	}
	while (old_docno)
	{
		if (!newblk.fitsInto( oldblk.frequency_at( blkcursor)))
		{
			newblk.setId(0);
			if (!newblk.empty())
			{
				dbadapter_posinfo.store( transaction, newblk.createBlock());
			}
			newblk.clear();
			newblk.setId( oldblk.id());
		}
		newblk.append( old_docno, oldblk.posinfo_at( blkcursor));
		old_docno = oldblk.nextDoc( blkcursor);
	}
}

void InvertedIndexMap::print( std::ostream& out) const
{
	out << "[typeno,termno,docno] to positions map:" << std::endl;
	Map::const_iterator mi = m_map.begin(), me = m_map.end();
	for (;mi != me; ++mi)
	{
		Index termno = BlockKey(mi->first.termkey).elem(2);
		Index docno = mi->first.docno;
		Index typeno = BlockKey( mi->first.termkey).elem(1);
		std::vector<PosinfoBlock::PositionType>::const_iterator pi = m_posinfo.begin() + mi->second;
		std::size_t nofpos = *pi++;
		std::vector<PosinfoBlock::PositionType>::const_iterator pe = pi + nofpos;
		out << "[termno=" << termno;
		out << ", typeno=" << typeno;
		out << ", docno=" << docno << "] ";
		for (int pidx=0; pi != pe; ++pi,++pidx)
		{
			if (pidx) out << ", ";
			out << *pi;
		}
		out << std::endl;
	}
}

