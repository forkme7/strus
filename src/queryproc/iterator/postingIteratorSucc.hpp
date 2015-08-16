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
#ifndef _STRUS_ITERATOR_SUCCESSOR_HPP_INCLUDED
#define _STRUS_ITERATOR_SUCCESSOR_HPP_INCLUDED
#include "postingIteratorJoin.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "private/internationalization.hpp"

namespace strus
{

class IteratorSucc
	:public IteratorJoin
{
public:
	IteratorSucc( const Reference< PostingIteratorInterface>& origin_)
		:m_origin( origin_)
		,m_featureid( origin_->featureid())
	{
		m_featureid.push_back('>');
	}

	virtual ~IteratorSucc()
	{}

	virtual const char* featureid() const
	{
		return m_featureid.c_str();
	}

	virtual Index skipDoc( const Index& docno_)
	{
		return m_origin->skipDoc( docno_);
	}

	virtual Index skipPos( const Index& pos_)
	{
		Index rt = m_origin->skipPos( pos_);
		return rt?(rt + 1):0;
	}

	virtual std::vector<const PostingIteratorInterface*>
			subExpressions( bool positive) const
	{
		return m_origin->subExpressions( positive);
	}

	virtual GlobalCounter documentFrequency() const
	{
		return m_origin->documentFrequency();
	}

	virtual Index docno() const
	{
		return m_origin->docno();
	}

	virtual Index posno() const
	{
		return m_origin->posno();
	}

private:
	Reference<PostingIteratorInterface> m_origin;	///< base feature expression this is the successor of
	std::string m_featureid;			///< unique id of the feature expression
};



class PostingJoinSucc
	:public PostingJoinOperatorInterface
{
public:
	virtual ~PostingJoinSucc(){}

	virtual const char* name()
	{
		return "succ";
	}

	virtual PostingIteratorInterface* createResultIterator(
			const std::vector<Reference< PostingIteratorInterface> >& argitr,
			int range) const
	{
		if (range != 0) throw strus::runtime_error( _TXT( "no range argument expected for 'succ'"));
		if (argitr.size() < 1) throw strus::runtime_error( _TXT( "too few arguments for 'succ'"));
		if (argitr.size() > 1) throw strus::runtime_error( _TXT( "too many arguments for 'succ'"));

		return new IteratorSucc( argitr[0]);
	}
};

}//namespace
#endif


