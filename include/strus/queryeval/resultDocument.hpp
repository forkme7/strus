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
#ifndef _STRUS_RESULT_DOCUMENT_HPP_INCLUDED
#define _STRUS_RESULT_DOCUMENT_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/queryeval/weightedDocument.hpp"
#include <vector>
#include <string>
#include <utility>

namespace strus {
namespace queryeval {

class ResultDocument
	:public WeightedDocument
{
public:
	class Attribute
	{
	public:
		Attribute( const std::string& name_, const std::string& value_)
			:m_name(name_),m_value(value_){}
		Attribute( const Attribute& o)
			:m_name(o.m_name),m_value(o.m_value){}

		const std::string& name() const			{return m_name;}
		const std::string& value() const		{return m_value;}

	private:
		std::string m_name;
		std::string m_value;
	};

	ResultDocument(){}
	ResultDocument( const ResultDocument& o)
		:WeightedDocument(o),m_attributes(o.m_attributes){}
	ResultDocument( const WeightedDocument& o, const std::vector<Attribute>& a)
		:WeightedDocument(o),m_attributes(a){}
	ResultDocument( const Index& docno_, float weight_, const std::vector<Attribute>& attributes_)
		:WeightedDocument(docno_,weight_),m_attributes(attributes_){}

	const std::vector<Attribute>& attributes() const	{return m_attributes;}

private:
	std::vector<Attribute> m_attributes;
};

}}//namespace
#endif
