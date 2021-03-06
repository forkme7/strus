/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "summarizerForwardIndex.hpp"
#include "strus/numericVariant.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/base/string_format.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/utils.hpp"
#include <set>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <limits>

using namespace strus;
#define METHOD_NAME "forwardindex"

SummarizerFunctionContextForwardIndex::SummarizerFunctionContextForwardIndex( const StorageClientInterface* storage_, const std::string& resultname_, const std::string& type_, unsigned int maxNofMatches_, ErrorBufferInterface* errorhnd_)
	:m_storage(storage_),m_forwardindex(storage_->createForwardIterator(type_))
	,m_resultname(resultname_),m_type(type_),m_maxNofMatches(maxNofMatches_),m_errorhnd(errorhnd_)
{
	if (m_type.empty() || m_resultname.empty())
	{
		throw strus::runtime_error(_TXT("missing type and result definition of '%s'"), METHOD_NAME);
	}
	if (!m_forwardindex.get())
	{
		throw strus::runtime_error(_TXT("failed to create summarizer context for '%s': %s"), METHOD_NAME, m_errorhnd->fetchError());
	}
}

void SummarizerFunctionContextForwardIndex::setVariableValue( const std::string&, double)
{
	m_errorhnd->report( _TXT("no variables known for function '%s'"), METHOD_NAME);
}

void SummarizerFunctionContextForwardIndex::addSummarizationFeature(
		const std::string&,
		PostingIteratorInterface*,
		const std::vector<SummarizationVariable>&,
		double /*weight*/,
		const TermStatistics&)
{
	m_errorhnd->report( _TXT("no summarization feature defined for '%s'"), METHOD_NAME);
}

std::vector<SummaryElement>
	SummarizerFunctionContextForwardIndex::getSummary( const Index& docno)
{
	try
	{
		std::vector<SummaryElement> rt;
		unsigned int cnt = m_maxNofMatches;
		m_forwardindex->skipDoc( docno);
		Index pos = 0;
		while (0 != (pos = m_forwardindex->skipPos( pos+1)))
		{
			rt.push_back( SummaryElement( m_resultname, m_forwardindex->fetch(), 1.0, pos));
			if (--cnt == 0) break;
		}
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summary: %s"), METHOD_NAME, *m_errorhnd, std::vector<SummaryElement>());
}

std::string SummarizerFunctionContextForwardIndex::debugCall( const Index& docno)
{
	std::ostringstream out;
	out << std::fixed << std::setprecision(8);
	out << string_format( _TXT( "summarize %s"), METHOD_NAME) << std::endl;

	std::vector<SummaryElement> res = getSummary( docno);
	std::vector<SummaryElement>::const_iterator ri = res.begin(), re = res.end();
	for (; ri != re; ++ri)
	{
		out << string_format( _TXT("match %s %s"), ri->name().c_str(), ri->value().c_str()) << std::endl;
	}
	return out.str();
}


void SummarizerFunctionInstanceForwardIndex::addStringParameter( const std::string& name, const std::string& value)
{
	if (utils::caseInsensitiveEquals( name, "name"))
	{
		m_resultname = value;
	}
	else if (utils::caseInsensitiveEquals( name, "type"))
	{
		m_type = value;
		if (m_resultname.empty())
		{
			m_resultname = value;
		}
	}
	else
	{
		m_errorhnd->report( _TXT("unknown '%s' summarization function parameter '%s'"), METHOD_NAME, name.c_str());
	}
}

void SummarizerFunctionInstanceForwardIndex::addNumericParameter( const std::string& name, const NumericVariant& val)
{
	if (utils::caseInsensitiveEquals( name, "name"))
	{
		m_errorhnd->report( _TXT("parameter '%s' for summarizer '%s' expected to be defined as string and not as numeric value"), name.c_str(), METHOD_NAME);
	}
	else if (utils::caseInsensitiveEquals( name, "type"))
	{
		m_errorhnd->report( _TXT("parameter '%s' for summarizer '%s' expected to be defined as string and not as numeric value"), name.c_str(), METHOD_NAME);
	}
	else if (utils::caseInsensitiveEquals( name, "N"))
	{
		m_maxNofMatches = std::min(
			val.touint(),
			(NumericVariant::UIntType)std::numeric_limits<unsigned int>::max());
	}
	else
	{
		m_errorhnd->report( _TXT("unknown '%s' summarization function parameter '%s'"), METHOD_NAME, name.c_str());
	}
}

void SummarizerFunctionInstanceForwardIndex::defineResultName(
		const std::string& resultname,
		const std::string& itemname)
{
	try
	{
		if (itemname.empty() || utils::caseInsensitiveEquals( itemname, "name"))
		{
			m_resultname = resultname;
		}
		else
		{
			throw strus::runtime_error( _TXT("unknown item name '%s"), itemname.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error defining result name of '%s' summarizer: %s"), METHOD_NAME, *m_errorhnd);
}


SummarizerFunctionContextInterface* SummarizerFunctionInstanceForwardIndex::createFunctionContext(
		const StorageClientInterface* storage_,
		MetaDataReaderInterface*,
		const GlobalStatistics&) const
{
	try
	{
		return new SummarizerFunctionContextForwardIndex( storage_, m_resultname, m_type, m_maxNofMatches, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' summarizer: %s"), METHOD_NAME, *m_errorhnd, 0);
}

std::string SummarizerFunctionInstanceForwardIndex::tostring() const
{
	std::ostringstream rt;
	rt << "type='" << m_type << "'"
		<< " resultname='" << m_resultname << "'";
	if (m_maxNofMatches) rt << " N=" << m_maxNofMatches;
	
	return rt.str();
}

SummarizerFunctionInstanceInterface* SummarizerFunctionForwardIndex::createInstance(
		const QueryProcessorInterface*) const
{
	try
	{
		return new SummarizerFunctionInstanceForwardIndex( m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating instance of '%s' summarizer: %s"), METHOD_NAME, *m_errorhnd, 0);
}


FunctionDescription SummarizerFunctionForwardIndex::getDescription() const
{
	try
	{
		typedef FunctionDescription::Parameter P;
		FunctionDescription rt( _TXT("Get the complete forward index"));
		rt( P::String, "type", _TXT( "the forward index type to fetch the summary elements"), "");
		rt( P::String, "name", _TXT( "the name of the result attribute (default is the value of 'type'')"), "");
		rt( P::Numeric, "N", _TXT( "the maximum number of matches to return"), "1:");
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating summarizer function description for '%s': %s"), METHOD_NAME, *m_errorhnd, FunctionDescription());
}


