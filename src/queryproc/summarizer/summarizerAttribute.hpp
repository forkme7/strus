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
#ifndef _STRUS_SUMMARIZER_ATTRIBUTE_HPP_INCLUDED
#define _STRUS_SUMMARIZER_ATTRIBUTE_HPP_INCLUDED
#include "strus/summarizerFunctionContextInterface.hpp"
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/summarizerFunctionInstanceInterface.hpp"
#include "private/internationalization.hpp"
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

namespace strus
{

/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class AttributeReaderInterface;
/// \brief Forward declaration
class PostingIteratorInterface;
/// \brief Forward declaration
class QueryProcessorInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

class SummarizerFunctionContextAttribute
	:public SummarizerFunctionContextInterface
{
public:
	/// \param[in] attribreader_ reader for document attributes
	/// \param[in] name_ attribute identifier
	SummarizerFunctionContextAttribute( AttributeReaderInterface* attribreader_, const std::string& name_, ErrorBufferInterface* errorhnd_);

	virtual ~SummarizerFunctionContextAttribute();

	virtual void addSummarizationFeature(
			const std::string&,
			PostingIteratorInterface*,
			const std::vector<SummarizationVariable>&,
			float /*weight*/,
			const TermStatistics&);

	virtual std::vector<SummarizerFunctionContextInterface::SummaryElement> getSummary( const Index& docno);

private:
	AttributeReaderInterface* m_attribreader;
	int m_attrib;
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


/// \class SummarizerFunctionInstanceAttribute
/// \brief Summarizer instance for retrieving meta data
class SummarizerFunctionInstanceAttribute
	:public SummarizerFunctionInstanceInterface
{
public:
	explicit SummarizerFunctionInstanceAttribute( ErrorBufferInterface* errorhnd_)
		:m_name(),m_errorhnd(errorhnd_){}

	virtual ~SummarizerFunctionInstanceAttribute(){}

	virtual void addStringParameter( const std::string& name, const std::string& value);
	virtual void addNumericParameter( const std::string& name, const ArithmeticVariant& value);

	virtual SummarizerFunctionContextInterface* createFunctionContext(
			const StorageClientInterface* storage,
			MetaDataReaderInterface*,
			const GlobalStatistics&) const;

	virtual std::string tostring() const;

private:
	std::string m_name;
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


class SummarizerFunctionAttribute
	:public SummarizerFunctionInterface
{
public:
	explicit SummarizerFunctionAttribute( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}
	SummarizerFunctionAttribute(){}
	virtual ~SummarizerFunctionAttribute(){}

	virtual SummarizerFunctionInstanceInterface* createInstance(
			const QueryProcessorInterface*) const;

	virtual Description getDescription() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


}//namespace
#endif


