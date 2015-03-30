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
#ifndef _STRUS_SUMMARIZATION_VARIABLE_HPP_INCLUDED
#define _STRUS_SUMMARIZATION_VARIABLE_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/postingIteratorInterface.hpp"
#include <vector>
#include <string>

namespace strus
{

/// \brief Structure describing a variable referencing a named match of a subexpression. Variables are attached to features used for summarization.
class SummarizationVariable
{
public:
	SummarizationVariable( const std::string& name_, PostingIteratorInterface* itr_)
		:m_name(name_),m_itr(itr_){}
	SummarizationVariable( const SummarizationVariable& o)
		:m_name(o.m_name),m_itr(o.m_itr){}

	/// \brief Name of the variable
	const std::string& name() const			{return m_name;}
	/// \brief Feature occurrence attached to this variable at the current position of the summarizer feature posting iterator this variable is attached to.
	Index position() const				{return m_itr->posno();}

private:
	std::string m_name;
	PostingIteratorInterface* m_itr;
};

}//namespace
#endif
