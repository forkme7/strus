/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013 Patrick Frey

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
#include "iterator.hpp"
#include "storage.hpp"
#include "indexPacker.hpp"
#include <string>
#include <vector>
#include <cstring>

using namespace strus;

Iterator::Iterator( leveldb::DB* db_, Index termtypeno, Index termvalueno)
	:m_db(db_),m_docno(0),m_itr(0),m_posno(0),m_positr(0),m_posend(0)
{
	m_key.push_back( (char)Storage::LocationPrefix);
	packIndex( m_key, termtypeno);
	packIndex( m_key, termvalueno);
	m_keysize = m_key.size();
}

Iterator::Iterator( const Iterator& o)
	:m_db(o.m_db)
	,m_key(o.m_key)
	,m_keysize(o.m_keysize)
	,m_docno(0)
	,m_itr(0)
	,m_posno(0)
	,m_positr(0)
	,m_posend(0)
{}

Iterator::~Iterator()
{
	if (m_itr) delete m_itr;
}


Index Iterator::skipDoc( const Index& docno)
{
	if (m_itr && m_docno +1 == docno)
	{
		return getNextTermDoc();
	}
	else
	{
		return getFirstTermDoc( docno);
	}
}

Index Iterator::skipPos( const Index& firstpos)
{
	if (m_posno > firstpos)
	{
		m_posno = 0;
		m_positr = m_itr->value().data();
		m_posend = m_positr + m_itr->value().size();
	}
	while (m_positr != m_posend && firstpos > m_posno)
	{
		// Get the next position increment and with it the next position number:
		Index incr = unpackIndex( m_positr, m_posend);
		m_posno += incr;
	}
	if (firstpos > m_posno)
	{
		return 0;
	}
	return m_posno;
}

Index Iterator::extractMatchDocno()
{
	if (m_keysize < m_itr->key().size() && 0==std::memcmp( m_key.c_str(), m_itr->key().data(), m_keysize))
	{
		// Check if we are still on the same term:
		const char* ki = m_itr->key().data();
		const char* ke = ki + m_itr->key().size();

		m_posno = 0;
		m_positr = m_itr->value().data();
		m_posend = m_positr + m_itr->value().size();

		// Return the matching document number:
		return m_docno=unpackIndex( ki, ke);
	}
	else
	{
		delete m_itr;
		m_docno = 0;
		m_itr = 0;
		m_posno = 0;
		m_positr = 0;
		m_posend = 0;
		return 0;
	}
}

Index Iterator::getNextTermDoc()
{
	m_itr->Next();
	return extractMatchDocno();
}

Index Iterator::getFirstTermDoc( const Index& docno)
{
	if (!m_itr)
	{
		m_itr = m_db->NewIterator( leveldb::ReadOptions());
	}
	m_key.resize( m_keysize);
	packIndex( m_key, docno);
	m_itr->Seek( leveldb::Slice( m_key.c_str(), m_keysize));

	return extractMatchDocno();
}

