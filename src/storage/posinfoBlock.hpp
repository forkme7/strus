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
#ifndef _STRUS_POSINFO_BLOCK_HPP_INCLUDED
#define _STRUS_POSINFO_BLOCK_HPP_INCLUDED
#include "dataBlock.hpp"
#include <vector>

namespace strus {

/// \class PosinfoBlock
/// \brief Block of term occurrence positions
class PosinfoBlock
	:public DataBlock
{
public:
	enum {
		MaxBlockSize=1024
	};
	typedef unsigned short PositionType;

public:
	explicit PosinfoBlock()
		:DataBlock(),m_docindexptr(0),m_posinfoptr(0)
	{}
	PosinfoBlock( const PosinfoBlock& o)
		:DataBlock(o)
		{initDocIndexNodeFrame();}
	PosinfoBlock( const Index& id_, const void* ptr_, std::size_t size_, bool allocated_=false)
		:DataBlock( id_, ptr_, size_, allocated_)
		{initDocIndexNodeFrame();}

	PosinfoBlock& operator=( const PosinfoBlock& o)
	{
		DataBlock::operator =(o);
		initDocIndexNodeFrame();
		return *this;
	}
	void swap( DataBlock& o)
	{
		DataBlock::swap( o);
		initDocIndexNodeFrame();
	}

	typedef unsigned int Cursor;
	/// \brief Get the document number of the current PosinfoBlock::Cursor
	Index docno_at( const Cursor& idx) const;
	/// \brief Get the internal representation of the postions of the current PosinfoBlock::Cursor
	const PositionType* posinfo_at( const Cursor& idx) const;
	/// \brief Get the postions of the current Cursor
	std::vector<Index> positions_at( const Cursor& idx) const;
	/// \brief Get the feature frequency of the current PosinfoBlock::Cursor
	unsigned int frequency_at( const Cursor& idx) const;

	/// \brief Get the next document with the current cursor
	Index nextDoc( Cursor& idx) const;
	/// \brief Get the first document with the current cursor
	Index firstDoc( Cursor& idx) const;
	/// \brief Upper bound search for a docnument number in the block
	Index skipDoc( const Index& docno_, Cursor& idx) const;

	bool isThisBlockAddress( const Index& docno_) const
	{
		return (docno_ <= id() && m_nofDocIndexNodes && docno_ > m_docindexptr[ 0].base);
	}
	/// \brief Check if the address 'docno_', if it exists, is most likely located in the following block (cheaper to fetch) or not
	bool isFollowBlockAddress( const Index& docno_) const
	{
		Index diff = id() - (m_nofDocIndexNodes?m_docindexptr[ 0].base:1);
		return (docno_ > id()) && (docno_ < id() + diff - (diff>>4));
	}

	class PositionScanner
	{
	public:
		PositionScanner()
			:m_ar(0),m_size(0),m_itr(0){}
		PositionScanner( const PositionType* ar_)
			:m_ar(ar_+1),m_size(ar_[0]),m_itr(0){}
		PositionScanner( const PositionScanner& o)
			:m_ar(o.m_ar),m_size(o.m_size),m_itr(o.m_itr){}

		bool initialized() const		{return !!m_itr;}

		void init( const PositionType* ar_)
		{
			if (ar_)
			{
				m_ar = ar_+1;
				m_size = ar_[0];
				m_itr = 0;
			}
			else
			{
				m_ar = 0;
				m_size = 0;
				m_itr = 0;
			}
		}

		void clear()						{init(0);}

		Index curpos() const					{return (m_itr<m_size)?m_ar[m_itr]:0;}
		Index skip( const Index& pos);

	private:
		const PositionType* m_ar;
		unsigned short m_size;
		unsigned short m_itr;
	};

	PositionScanner positionScanner_at( unsigned short posrefIdx) const
	{
		return PositionScanner( m_posinfoptr + posrefIdx);
	}

public:/*PosinfoBlockBuilder*/
	struct DocIndexNode
	{
		enum {Size=7};
		DocIndexNode();
		DocIndexNode( const DocIndexNode& o);

		bool addDocument( const Index& docno_, unsigned short posrefIdx_);
		Index skipDoc( const Index& docno_, unsigned short& posrefIdx_) const;
		std::size_t nofElements() const;

		Index base;
		unsigned short ofs[ Size-1];
		unsigned short posrefIdx[ Size];
	};

private:
	void initDocIndexNodeFrame();

private:
	unsigned int m_nofDocIndexNodes;
	const DocIndexNode* m_docindexptr;
	const PositionType* m_posinfoptr;
};

class PosinfoBlockBuilder
{
public:
	typedef PosinfoBlock::PositionType PositionType;
	typedef PosinfoBlock::DocIndexNode DocIndexNode;

public:
	PosinfoBlockBuilder( const PosinfoBlock& o);
	PosinfoBlockBuilder()
		:m_lastDoc(0),m_id(0){}
	PosinfoBlockBuilder( const PosinfoBlockBuilder& o)
		:m_docIndexNodeArray(o.m_docIndexNodeArray)
		,m_posinfoArray(o.m_posinfoArray)
		,m_lastDoc(o.m_lastDoc)
		,m_id(o.m_id){}

	Index id() const						{return m_id;}
	void setId( const Index& id_);

	bool empty() const						{return m_docIndexNodeArray.empty();}
	void append( const Index& docno, const std::vector<Index>& pos);
	void append( const Index& docno, const PositionType* posar);
	bool fitsInto( std::size_t nofpos) const;
	bool full() const
	{
		return m_posinfoArray.size() >= PosinfoBlock::MaxBlockSize;
	}

	const std::vector<DocIndexNode>& docIndexNodeArray() const	{return m_docIndexNodeArray;}
	const std::vector<PositionType>& posinfoArray() const		{return m_posinfoArray;}

	PosinfoBlock createBlock() const;
	void clear();

private:
	std::vector<DocIndexNode> m_docIndexNodeArray;
	std::vector<PositionType> m_posinfoArray;
	Index m_lastDoc;
	Index m_id;
};
}//namespace
#endif

