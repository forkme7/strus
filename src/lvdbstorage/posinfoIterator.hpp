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
#ifndef _STRUS_LVDB_POSINFO_ITERATOR_HPP_INCLUDED
#define _STRUS_LVDB_POSINFO_ITERATOR_HPP_INCLUDED
#include "posinfoBlock.hpp"
#include "blockStorage.hpp"
#include <leveldb/db.h>

namespace strus {

class PosinfoIterator
{
public:
	PosinfoIterator( leveldb::DB* db_, Index termtypeno, Index termvalueno);
	~PosinfoIterator(){}

	Index skipDoc( const Index& docno_);
	Index skipPos( const Index& firstpos_);

	Index docno() const					{return m_docno;}
	Index posno() const					{return m_positionScanner.initialized()?m_positionScanner.curpos():0;}

	bool isCloseCandidate( const Index& docno_) const	{return m_docno_start <= docno_ && m_docno_end >= docno_;}
	Index documentFrequency() const;
	unsigned int frequency();

private:
	bool loadBlock( const Index& elemno_);

private:
	leveldb::DB* m_db;
	BlockStorage<PosinfoBlock> m_posinfoStorage;
	const PosinfoBlock* m_posinfoBlk;
	char const* m_posinfoItr;
	PosinfoBlock::PositionScanner m_positionScanner;
	Index m_termtypeno;
	Index m_termvalueno;
	Index m_docno;
	Index m_docno_start;
	Index m_docno_end;
	mutable Index m_documentFrequency;
};

}
#endif
