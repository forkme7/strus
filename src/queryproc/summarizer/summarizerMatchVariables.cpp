/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "summarizerMatchVariables.hpp"
#include "postingIteratorLink.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/constants.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/base/string_format.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/utils.hpp"
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace strus;

#define METHOD_NAME "matchvar"

SummarizerFunctionContextMatchVariables::SummarizerFunctionContextMatchVariables(
		const StorageClientInterface* storage_,
		const QueryProcessorInterface* processor_,
		const Reference<MatchVariablesData>& data_,
		ErrorBufferInterface* errorhnd_)
	:m_storage(storage_)
	,m_processor(processor_)
	,m_forwardindex(storage_->createForwardIterator( data_->type))
	,m_data(data_)
	,m_features()
	,m_errorhnd(errorhnd_)
{
	if (!m_forwardindex.get()) throw strus::runtime_error( "%s", _TXT("error creating forward index iterator"));
}

void SummarizerFunctionContextMatchVariables::setVariableValue( const std::string&, double)
{
	m_errorhnd->report( _TXT("no variables known for function '%s'"), METHOD_NAME);
}

void SummarizerFunctionContextMatchVariables::addSummarizationFeature(
		const std::string& name,
		PostingIteratorInterface* itr,
		const std::vector<SummarizationVariable>& variables,
		double weight,
		const TermStatistics&)
{
	try
	{
		if (utils::caseInsensitiveEquals( name, "match"))
		{
			m_features.push_back( SummarizationFeature( itr, variables, weight));
		}
		else
		{
			m_errorhnd->report( _TXT("unknown '%s' summarization feature '%s'"), METHOD_NAME, name.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding feature to '%s' summarizer: %s"), METHOD_NAME, *m_errorhnd);
}


std::vector<SummaryElement>
	SummarizerFunctionContextMatchVariables::getSummary( const Index& docno)
{
	try
	{
		std::vector<SummaryElement> rt;
		m_forwardindex->skipDoc( docno);
		Index curpos = 0;

		std::vector<SummarizationFeature>::const_iterator
			fi = m_features.begin(), fe = m_features.end();
		for (; fi != fe; ++fi)
		{
			if (docno==fi->itr->skipDoc( docno))
			{
				curpos = fi->itr->skipPos( 0);
				for (int groupidx=0; curpos; curpos = fi->itr->skipPos( curpos+1),++groupidx)
				{
					std::vector<SummarizationVariable>::const_iterator
						vi = fi->variables.begin(),
						ve = fi->variables.end();

					for (; vi != ve; ++vi)
					{
						Index pos = vi->position();
						if (pos)
						{
							if (pos == m_forwardindex->skipPos( pos))
							{
								MatchVariablesData::NameMap::const_iterator ni = m_data->namemap.find( vi->name());
								if (ni == m_data->namemap.end())
								{
									rt.push_back( SummaryElement( vi->name(), m_forwardindex->fetch(), fi->weight, groupidx));
								}
								else
								{
									rt.push_back( SummaryElement( ni->second, m_forwardindex->fetch(), fi->weight, groupidx));
								}
							}
						}
					}
				}
			}
		}
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summary: %s"), METHOD_NAME, *m_errorhnd, std::vector<SummaryElement>());
}

std::string SummarizerFunctionContextMatchVariables::debugCall( const Index& docno)
{
	std::ostringstream out;
	out << std::fixed << std::setprecision(8);
	out << string_format( _TXT( "summarize %s"), METHOD_NAME) << std::endl;

	std::vector<SummaryElement> res = getSummary( docno);
	std::vector<SummaryElement>::const_iterator ri = res.begin(), re = res.end();
	for (; ri != re; ++ri)
	{
		out << string_format( _TXT( "variable name=%s, value='%s'"), ri->name().c_str(), ri->value().c_str()) << std::endl;
	}
	return out.str();
}


void SummarizerFunctionInstanceMatchVariables::addStringParameter( const std::string& name, const std::string& value)
{
	try
	{
		if (utils::caseInsensitiveEquals( name, "match"))
		{
			m_errorhnd->report( _TXT("parameter '%s' for summarizer '%s' expected to be defined as feature and not as string"), name.c_str(), METHOD_NAME);
		}
		else if (utils::caseInsensitiveEquals( name, "type"))
		{
			m_data->type = value;
		}
		else
		{
			throw strus::runtime_error( _TXT("unknown '%s' summarization function parameter '%s'"), METHOD_NAME, name.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding string parameter to '%s' summarizer: %s"), METHOD_NAME, *m_errorhnd);
}

void SummarizerFunctionInstanceMatchVariables::addNumericParameter( const std::string& name, const NumericVariant& value)
{
	if (utils::caseInsensitiveEquals( name, "match"))
	{
		m_errorhnd->report( _TXT("parameter '%s' for summarizer '%s' expected to be defined as feature and not as numeric value"), name.c_str(), METHOD_NAME);
	}
	else if (utils::caseInsensitiveEquals( name, "type"))
	{
		m_errorhnd->report( _TXT("no numeric value expected for parameter '%s' in summarization function '%s'"), name.c_str(), METHOD_NAME);
	}
	else
	{
		m_errorhnd->report( _TXT("unknown '%s' summarization function parameter '%s'"), METHOD_NAME, name.c_str());
	}
}

void SummarizerFunctionInstanceMatchVariables::defineResultName(
		const std::string& resultname,
		const std::string& itemname)
{
	try
	{
		m_data->namemap[ resultname] = itemname;
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error defining result name of '%s' summarizer: %s"), METHOD_NAME, *m_errorhnd);
}

SummarizerFunctionContextInterface* SummarizerFunctionInstanceMatchVariables::createFunctionContext(
		const StorageClientInterface* storage,
		MetaDataReaderInterface*,
		const GlobalStatistics&) const
{
	if (m_data->type.empty())
	{
		m_errorhnd->report( _TXT( "empty forward index type definition (parameter 'type') in match phrase summarizer configuration"));
	}
	try
	{
		return new SummarizerFunctionContextMatchVariables( storage, m_processor, m_data, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' summarizer: %s"), METHOD_NAME, *m_errorhnd, 0);
}

std::string SummarizerFunctionInstanceMatchVariables::tostring() const
{
	try
	{
		std::ostringstream rt;
		rt << "type='" << m_data->type << "'";
		MatchVariablesData::NameMap::const_iterator ni = m_data->namemap.begin(), ne = m_data->namemap.end();
		for (; ni != ne; ++ni)
		{
			rt << ", var " << ni->first << "=" << ni->second;
		}
		return rt.str();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error mapping '%s' summarizer to string: %s"), METHOD_NAME, *m_errorhnd, std::string());
}


SummarizerFunctionInstanceInterface* SummarizerFunctionMatchVariables::createInstance(
		const QueryProcessorInterface* processor) const
{
	try
	{
		return new SummarizerFunctionInstanceMatchVariables( processor, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating instance of '%s' summarizer: %s"), METHOD_NAME, *m_errorhnd, 0);
}



FunctionDescription SummarizerFunctionMatchVariables::getDescription() const
{
	try
	{
		typedef FunctionDescription::Parameter P;
		FunctionDescription rt( _TXT("Extract all variables assigned to subexpressions of features specified."));
		rt( P::Feature, "match", _TXT( "defines the query features to inspect for variable matches"), "");
		rt( P::String, "type", _TXT( "the forward index feature type for the content to extract"), "");
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating summarizer function description for '%s': %s"), METHOD_NAME, *m_errorhnd, FunctionDescription());
}

