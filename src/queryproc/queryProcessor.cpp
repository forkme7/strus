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
#include "queryProcessor.hpp"
#include "summarizer/summarizer_standard.hpp"
#include "iterator/iterator_standard.hpp"
#include "weighting/weighting_standard.hpp"
#include "strus/constants.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/utils.hpp"
#include <string>
#include <vector>
#include <stdexcept>
#include <set>
#include <limits>
#include <iostream>

using namespace strus;

QueryProcessor::QueryProcessor( ErrorBufferInterface* errorhnd_)
	:m_errorhnd(errorhnd_)
{
	definePostingJoinOperator( "within", createPostingJoinWithin( m_errorhnd));
	definePostingJoinOperator( "within_struct", createPostingJoinStructWithin( m_errorhnd));
	definePostingJoinOperator( "sequence", createPostingJoinSequence( m_errorhnd));
	definePostingJoinOperator( "sequence_struct", createPostingJoinStructSequence( m_errorhnd));
	definePostingJoinOperator( "diff", createPostingJoinDifference( m_errorhnd));
	definePostingJoinOperator( "intersect", createPostingJoinIntersect( m_errorhnd));
	definePostingJoinOperator( "union", createPostingJoinUnion( m_errorhnd));
	definePostingJoinOperator( "succ", createPostingSucc( m_errorhnd));
	definePostingJoinOperator( "pred", createPostingPred( m_errorhnd));
	definePostingJoinOperator( "contains", createPostingJoinContains( m_errorhnd));
	
	defineWeightingFunction( "bm25", createWeightingFunctionBm25( m_errorhnd));
	defineWeightingFunction( "bm25_dpfc", createWeightingFunctionBm25_dpfc( m_errorhnd));
	defineWeightingFunction( "tf", createWeightingFunctionTermFrequency( m_errorhnd));
	defineWeightingFunction( "td", createWeightingFunctionConstant( m_errorhnd));
	defineWeightingFunction( "metadata", createWeightingFunctionMetadata( m_errorhnd));
	defineWeightingFunction( "formula", createWeightingFunctionFormula( m_errorhnd));

	defineSummarizerFunction( "metadata", createSummarizerMetaData( m_errorhnd));
	defineSummarizerFunction( "matchphrase", createSummarizerMatchPhrase( m_errorhnd));
	defineSummarizerFunction( "matchpos", createSummarizerListMatches( m_errorhnd));
	defineSummarizerFunction( "attribute", createSummarizerAttribute( m_errorhnd));
	defineSummarizerFunction( "matchvariables", createSummarizerMatchVariables( m_errorhnd));
}

QueryProcessor::~QueryProcessor()
{}

void QueryProcessor::definePostingJoinOperator(
		const std::string& name,
		PostingJoinOperatorInterface* op)
{
	try
	{
		Reference<PostingJoinOperatorInterface> opref( op);
		m_joiners[ utils::tolower( std::string(name))] = opref;
	}
	catch (std::bad_alloc&)
	{
		m_errorhnd->report( _TXT("out of memory"));
	}
}

const PostingJoinOperatorInterface* QueryProcessor::getPostingJoinOperator(
		const std::string& name) const
{
	std::map<std::string,Reference<PostingJoinOperatorInterface> >::const_iterator 
		ji = m_joiners.find( utils::tolower( name));
	if (ji == m_joiners.end())
	{
		m_errorhnd->report( _TXT( "posting set join operator not defined: '%s'"), name.c_str());
		return 0;
	}
	return ji->second.get();
}

void QueryProcessor::defineWeightingFunction(
		const std::string& name,
		WeightingFunctionInterface* func)
{
	try
	{
		Reference<WeightingFunctionInterface> funcref( func);
		m_weighters[ utils::tolower( std::string(name))] = funcref;
	}
	catch (std::bad_alloc&)
	{
		m_errorhnd->report( _TXT("out of memory"));
	}
}

const WeightingFunctionInterface* QueryProcessor::getWeightingFunction(
		const std::string& name) const
{
	std::map<std::string,Reference<WeightingFunctionInterface> >::const_iterator 
		wi = m_weighters.find( utils::tolower( std::string(name)));
	if (wi == m_weighters.end())
	{
		m_errorhnd->report( _TXT( "weighting function not defined: '%s'"), name.c_str());
		return 0;
	}
	return wi->second.get();
}

void QueryProcessor::defineSummarizerFunction(
		const std::string& name,
		SummarizerFunctionInterface* sumfunc)
{
	try
	{
		Reference<SummarizerFunctionInterface> funcref( sumfunc);
		m_summarizers[ utils::tolower( std::string(name))] = funcref;
	}
	catch (std::bad_alloc&)
	{
		m_errorhnd->report( _TXT("out of memory"));
	}
}

const SummarizerFunctionInterface* QueryProcessor::getSummarizerFunction(
		const std::string& name) const
{
	std::map<std::string,Reference<SummarizerFunctionInterface> >::const_iterator 
		si = m_summarizers.find( utils::tolower( std::string(name)));
	if (si == m_summarizers.end())
	{
		m_errorhnd->report( _TXT( "summarization function not defined: '%s'"), name.c_str());
		return 0;
	}
	return si->second.get();
}



