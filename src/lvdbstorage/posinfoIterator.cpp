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
#include "posinfoIterator.hpp"
#include "blockStorage.hpp"
#include "databaseKey.hpp"
#include "statistics.hpp"
#include "keyValueStorage.hpp"
#include <leveldb/db.h>

using namespace strus;

PosinfoIterator::PosinfoIterator( leveldb::DB* db_, Index termtypeno, Index termvalueno)
	:m_db(db_)
	,m_posinfoStorage( db_, DatabaseKey::PosinfoBlockPrefix, BlockKey( termtypeno, termvalueno), true)
	,m_posinfoBlk(0)
	,m_posinfoItr(0)
	,m_termtypeno(termtypeno)
	,m_termvalueno(termvalueno)
	,m_docno(0)
	,m_docno_start(0)
	,m_docno_end(0)
	,m_documentFrequency(-1){}


Index PosinfoIterator::skipDoc( const Index& docno_)
{
	if (m_docno && m_docno == docno_) return m_docno;

	m_positionScanner.clear();
	if (!m_posinfoBlk)
	{
		// [A] No block loaded yet
		m_posinfoBlk = m_posinfoStorage.load( docno_);
		if (!m_posinfoBlk)
		{
			m_posinfoItr = 0;
			m_docno_start = 0;
			m_docno_end = 0;
			return m_docno=0;
		}
		else
		{
			m_posinfoItr = m_posinfoBlk->upper_bound( docno_, m_posinfoBlk->begin());
			m_docno_start = m_posinfoBlk->docno_at( m_posinfoBlk->begin());
			m_docno_end = m_posinfoBlk->id();
		}
	}
	else
	{
		if (m_docno_start <= docno_ && m_docno_end >= docno_)
		{
			// [B] Answer in same block as for the last query
			if (docno_ < m_docno)
			{
				m_posinfoItr = m_posinfoBlk->begin();
			}
			m_posinfoItr = m_posinfoBlk->upper_bound( docno_, m_posinfoItr);
		}
		else if (docno_ > m_docno_end && m_docno_end + (m_docno_end - m_docno_start) > docno_)
		{
			// [C] Try to get answer from a follow block
			do
			{
				m_posinfoBlk = m_posinfoStorage.loadNext();
				if (m_posinfoBlk)
				{
					m_docno_start = m_posinfoBlk->docno_at( m_posinfoBlk->begin());
					m_docno_end = m_posinfoBlk->id();

					Statistics::increment( Statistics::PosinfoBlockReadBlockFollow);

					if (m_posinfoBlk->id() < docno_ && !m_posinfoBlk->isFollowBlockAddress( docno_))
					{
						m_posinfoBlk = m_posinfoStorage.load( docno_);
						if (m_posinfoBlk)
						{
							Statistics::increment( Statistics::PosinfoBlockReadBlockRandom);
						}
						else
						{
							Statistics::increment( Statistics::PosinfoBlockReadBlockRandomMiss);
							m_posinfoItr = 0;
							m_docno_start = 0;
							m_docno_end = 0;
							return m_docno=0;
						}
					}
				}
				else
				{
					Statistics::increment( Statistics::PosinfoBlockReadBlockFollowMiss);
					m_posinfoItr = 0;
					m_docno_start = 0;
					m_docno_end = 0;
					return m_docno=0;
				}
			}
			while (m_posinfoBlk->id() < docno_);

			m_posinfoItr = m_posinfoBlk->upper_bound( docno_, m_posinfoBlk->begin());
		}
		else
		{
			// [D] Answer is in a 'far away' block
			m_posinfoBlk = m_posinfoStorage.load( docno_);
			if (!m_posinfoBlk)
			{
				Statistics::increment( Statistics::PosinfoBlockReadBlockRandomMiss);
				m_posinfoItr = 0;
				m_docno_start = 0;
				m_docno_end = 0;
				return m_docno=0;
			}
			else
			{
				Statistics::increment( Statistics::PosinfoBlockReadBlockRandom);
				m_posinfoItr = m_posinfoBlk->upper_bound( docno_, m_posinfoBlk->begin());
				m_docno_start = m_posinfoBlk->docno_at( m_posinfoBlk->begin());
				m_docno_end = m_posinfoBlk->id();
			}
		}
	}
	if (!m_posinfoItr || m_posinfoItr == m_posinfoBlk->end())
	{
		m_docno_start = 0;
		m_docno_end = 0;
		return m_docno=0;
	}
	return m_docno=m_posinfoBlk->docno_at( m_posinfoItr);
}

Index PosinfoIterator::skipPos( const Index& firstpos_)
{
	if (!m_posinfoItr || !m_docno) return 0;
	if (!m_positionScanner.initialized())
	{
		m_positionScanner = m_posinfoBlk->positionScanner_at( m_posinfoItr);
		if (!m_positionScanner.initialized()) return 0;
	}
	Index rt = m_positionScanner.skip( firstpos_);
	if (rt && rt < firstpos_)
	{
		throw std::runtime_error( "internal: corrupt index (posinfo block)");
	}
	return rt;
}

unsigned int PosinfoIterator::frequency()
{
	if (!m_posinfoItr || !m_docno) return 0;
	return m_posinfoBlk->frequency_at( m_posinfoItr);
}

Index PosinfoIterator::documentFrequency() const
{
	if (m_documentFrequency < 0)
	{
		KeyValueStorage dfstorage(
			m_db, DatabaseKey::DocFrequencyPrefix, false);
		const KeyValueStorage::Value* dfpacked
			= dfstorage.load( BlockKey( m_termtypeno, m_termvalueno));

		if (!dfpacked) return 0;
		char const* cc = dfpacked->ptr();
		m_documentFrequency = unpackIndex( cc, cc + dfpacked->size());
	}
	return m_documentFrequency;
}
