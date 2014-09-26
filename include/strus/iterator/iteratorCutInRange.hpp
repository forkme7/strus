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
#ifndef _STRUS_ITERATOR_CUTINRANGE_HPP_INCLUDED
#define _STRUS_ITERATOR_CUTINRANGE_HPP_INCLUDED
#include "strus/iteratorInterface.hpp"

namespace strus
{

///\class IteratorCutInRange
///\brief Selects all elements that start a selected range defined by position difference between an element of first set and an element of a second set without apearance of an element of a third set in this range.
class IteratorCutInRange
	:public IteratorInterface
{
public:
	///\param[in] first_ Defines the positions of the set of elements defining the start of a range
	///\param[in] second_ Defines the positions of the set of elements defining the end of a range
	///\param[in] cut_ Defines the positions of the set of elements defining the negative condition on selected ranges. These elements must not appear in a selected range
	///\param[in] range_ Defines the maximum position difference between the start element and the end element of a selected range
	///\param[in] firstElemCut Negative (cut) selection starts one position after the first element
	///\param[in] secondElemCut true: Negative (cut) selection ends one position before the last element
	IteratorCutInRange( const IteratorInterfaceR& first_, const IteratorInterfaceR& second_, const IteratorInterfaceR& cut_, const Index& range_, bool firstElemCut_, bool secondElemCut_);
	virtual ~IteratorCutInRange(){}

	virtual Index skipDoc( const Index& docno)=0;
	virtual Index skipPos( const Index& pos)=0;

private:
	Index m_docno;
	Index m_docno_cut;
	IteratorInterfaceR m_first;	///< first set to intersect range
	IteratorInterfaceR m_second;	///< first set to intersect range 
	IteratorInterfaceR m_cut;	///< iterator for elements that make a negative selection on the result if they appear in a matching range
	Index m_range;			///< allowe position range between first and second element
	bool m_firstElemCut;		///< true => cut on first element position
	bool m_secondElemCut;		///< true => cut on second element position
};

}//namespace
#endif

