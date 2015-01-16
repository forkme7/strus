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
#ifndef _STRUS_ITERATOR_INTERSECT_HPP_INCLUDED
#define _STRUS_ITERATOR_INTERSECT_HPP_INCLUDED
#include "iterator/postingIteratorJoin.hpp"
#include "strus/postingJoinOperatorInterface.hpp"

namespace strus
{

class IteratorIntersect
	:public IteratorJoin
{
public:
	IteratorIntersect( std::size_t nofargs, PostingIteratorInterface** args);
	virtual ~IteratorIntersect();

	virtual const std::string& featureid() const
	{
		return m_featureid;
	}
	virtual Index skipDoc( const Index& docno);
	virtual Index skipPos( const Index& pos);

	virtual std::vector<const PostingIteratorInterface*> subExpressions( bool positive) const;

	virtual Index documentFrequency() const;

	virtual Index docno() const
	{
		return m_docno;
	}

	virtual Index posno() const
	{
		return m_posno;
	}

private:
	Index m_docno;
	Index m_posno;				///< current position
	std::size_t m_argarsize;		///< nof arguments
	PostingIteratorInterface** m_argar;	///< arguments
	std::string m_featureid;		///< unique id of the feature expression
	mutable Index m_documentFrequency;	///< document frequency (of the rarest subexpression)
};


class PostingJoinIntersect
	:public PostingJoinOperatorInterface
{
public:
	virtual ~PostingJoinIntersect(){}

	virtual PostingIteratorInterface* createResultIterator(
			std::size_t nofitrs_,
			PostingIteratorInterface** itrs_,
			int range) const
	{
		if (range != 0) throw std::runtime_error( "no range argument expected");
		if (nofitrs_ == 0) throw std::runtime_error( "too few arguments");

		return new IteratorIntersect( nofitrs_, itrs_);
	}
};

}//namespace
#endif


