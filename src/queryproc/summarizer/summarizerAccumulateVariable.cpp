/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "summarizerAccumulateVariable.hpp"
#include "postingIteratorLink.hpp"
#include "ranker.hpp"
#include "strus/arithmeticVariant.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/constants.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/utils.hpp"
#include "private/bitOperations.hpp"
#include "private/localStructAllocator.hpp"
#include <cstdlib>

using namespace strus;

SummarizerFunctionContextAccumulateVariable::SummarizerFunctionContextAccumulateVariable(
		const StorageClientInterface* storage_,
		const QueryProcessorInterface* processor_,
		const std::string& type_,
		const std::string& var_,
		double norm_,
		unsigned int maxNofElements_,
		ErrorBufferInterface* errorhnd_)
	:m_storage(storage_)
	,m_processor(processor_)
	,m_forwardindex(storage_->createForwardIterator( type_))
	,m_type(type_)
	,m_var(var_)
	,m_norm(norm_)
	,m_maxNofElements(maxNofElements_)
	,m_features()
	,m_errorhnd(errorhnd_)
{
	if (!m_forwardindex.get()) throw strus::runtime_error(_TXT("error creating forward index iterator"));
	if (m_type.empty()) throw strus::runtime_error(_TXT("type of forward index to extract not defined (parameter 'type')"));
	if (m_var.empty()) throw strus::runtime_error(_TXT("name of variable to extract not defined (parameter 'var')"));
}


void SummarizerFunctionContextAccumulateVariable::addSummarizationFeature(
		const std::string& name,
		PostingIteratorInterface* itr,
		const std::vector<SummarizationVariable>& variables,
		float weight,
		const TermStatistics&)
{
	try
	{
		if (utils::caseInsensitiveEquals( name, "match"))
		{
			std::vector<const PostingIteratorInterface*> varitr;
			std::vector<SummarizationVariable>::const_iterator vi = variables.begin(), ve = variables.end();
			for (; vi != ve; ++vi)
			{
				if (utils::caseInsensitiveEquals( vi->name(), m_var))
				{
					varitr.push_back( vi->itr());
				}
			}
			if (m_features.size() >= 64)
			{
				m_errorhnd->report( _TXT("too many features (>64) defined for '%s' summarization"), "accuvariable");
			}
			if (varitr.empty())
			{
				m_errorhnd->report( _TXT("no variables with name '%s' defined in feature passed to '%s'"), m_var.c_str(), "accuvariable");
			}
			m_features.push_back( SummarizationFeature( itr, varitr, weight));
		}
		else
		{
			m_errorhnd->report( _TXT("unknown '%s' summarization feature '%s'"), "accuvariable", name.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding feature to '%s' summarizer: %s"), "accuvariable", *m_errorhnd);
}


static void setSelected( uint64_t& set, unsigned int idx)
{
	set |= ((uint64_t)1 << idx);
}

static int nextSelected( uint64_t& set)
{
	int rt = BitOperations::bitScanForward( set)-1;
	if (rt >= 0)
	{
		set -= (uint64_t)1<<rt;
	}
	return rt;
}

std::vector<SummaryElement>
	SummarizerFunctionContextAccumulateVariable::getSummary( const Index& docno)
{
	try
	{
		typedef LocalStructAllocator<std::pair<Index,double> > PosWeightAllocator;
		typedef std::map<Index,double,std::less<Index>,PosWeightAllocator> PosWeightMap;

		uint64_t docsel = 0;

		m_forwardindex->skipDoc( docno);

		// Build a bitmap with all matching documents:
		std::vector<SummarizationFeature>::const_iterator
			fi = m_features.begin(), fe = m_features.end();
		unsigned int fidx=0;
		for (; fi != fe; ++fi,++fidx)
		{
			if (docno==fi->itr->skipDocCandidate( docno))
			{
				setSelected( docsel, fidx);
			}
		}
		// For every match position multiply the weights for each position and add them 
		// to the final accumulation result:
		PosWeightMap posWeightMap;
		uint64_t docselitr = docsel;
		int idx = 0;
		while (0<=(idx=nextSelected( docselitr)))
		{
			const SummarizationFeature& sumfeat = m_features[ idx];
			Index curpos = sumfeat.itr->skipPos( 0);
			for (; curpos; curpos = sumfeat.itr->skipPos( curpos+1))
			{
				std::vector<const PostingIteratorInterface*>::const_iterator
					vi = sumfeat.varitr.begin(), ve = sumfeat.varitr.end();
				for (;vi != ve; ++vi)
				{
					PosWeightMap::iterator wi = posWeightMap.find( curpos);
					if (wi == posWeightMap.end())
					{
						posWeightMap[ curpos] = sumfeat.weight;
					}
					else
					{
						wi->second *= sumfeat.weight;
					}
				}
			}
		}
		// Build the accumulation result:
		std::vector<SummaryElement> rt;
		PosWeightMap::const_iterator wi = posWeightMap.begin(), we = posWeightMap.end();
		if (m_maxNofElements < posWeightMap.size())
		{
			Ranker ranker( m_maxNofElements);
			for (; wi != we; ++wi)
			{
				ranker.insert( wi->second, wi->first);
			}
			std::vector<Ranker::Element> ranklist = ranker.result();
			std::vector<Ranker::Element>::const_iterator ri = ranklist.begin(), re = ranklist.end();
			for (; ri != re; ++ri)
			{
				if (m_forwardindex->skipPos( ri->idx) == ri->idx)
				{
					rt.push_back( SummaryElement(
							m_var, m_forwardindex->fetch(), ri->weight * m_norm));
				}
			}
		}
		else
		{
			for (; wi != we; ++wi)
			{
				if (m_forwardindex->skipPos( wi->first) == wi->first)
				{
					rt.push_back( SummaryElement( m_var, m_forwardindex->fetch(), wi->second * m_norm));
				}
			}
		}
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summary: %s"), "accuvariable", *m_errorhnd, std::vector<SummaryElement>());
}


void SummarizerFunctionInstanceAccumulateVariable::addStringParameter( const std::string& name, const std::string& value)
{
	try
	{
		if (utils::caseInsensitiveEquals( name, "match"))
		{
			m_errorhnd->report( _TXT("parameter '%s' for summarizer '%s' expected to be defined as feature and not as string"), name.c_str(), "accuvariable");
		}
		else if (utils::caseInsensitiveEquals( name, "type"))
		{
			m_type = value;
		}
		else if (utils::caseInsensitiveEquals( name, "var"))
		{
			m_var = value;
		}
		else if (utils::caseInsensitiveEquals( name, "norm"))
		{
			m_errorhnd->report( _TXT("no string value expected for parameter '%s' in summarization function '%s'"), name.c_str(), "norm");
		}
		else if (utils::caseInsensitiveEquals( name, "nof"))
		{
			m_errorhnd->report( _TXT("no string value expected for parameter '%s' in summarization function '%s'"), name.c_str(), "nof");
		}
		else
		{
			throw strus::runtime_error( _TXT("unknown '%s' summarization function parameter '%s'"), "accuvariable", name.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding string parameter to '%s' summarizer: %s"), "accuvariable", *m_errorhnd);
}

void SummarizerFunctionInstanceAccumulateVariable::addNumericParameter( const std::string& name, const ArithmeticVariant& value)
{
	if (utils::caseInsensitiveEquals( name, "match"))
	{
		m_errorhnd->report( _TXT("parameter '%s' for summarizer '%s' expected to be defined as feature and not as numeric value"), name.c_str(), "accuvariable");
	}
	else if (utils::caseInsensitiveEquals( name, "type")
	||  utils::caseInsensitiveEquals( name, "var"))
	{
		m_errorhnd->report( _TXT("no numeric value expected for parameter '%s' in summarization function '%s'"), name.c_str(), "accuvariable");
	}
	else if (utils::caseInsensitiveEquals( name, "nof"))
	{
		m_maxNofElements = value.touint();
	}
	else if (utils::caseInsensitiveEquals( name, "norm"))
	{
		m_norm = value.tofloat();
	}
	else
	{
		m_errorhnd->report( _TXT("unknown '%s' summarization function parameter '%s'"), "accuvariable", name.c_str());
	}
}

SummarizerFunctionContextInterface* SummarizerFunctionInstanceAccumulateVariable::createFunctionContext(
		const StorageClientInterface* storage,
		MetaDataReaderInterface*,
		const GlobalStatistics&) const
{
	if (m_type.empty())
	{
		m_errorhnd->report( _TXT( "empty forward index type definition (parameter 'type') in match phrase summarizer configuration"));
	}
	try
	{
		return new SummarizerFunctionContextAccumulateVariable( storage, m_processor, m_type, m_var, m_norm, m_maxNofElements, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' summarizer: %s"), "accuvariable", *m_errorhnd, 0);
}

std::string SummarizerFunctionInstanceAccumulateVariable::tostring() const
{
	try
	{
		std::ostringstream rt;
		rt << "type='" << m_type << "'";
		if (!m_var.empty()) rt << ", var='" << m_var << "'";
		rt << ", nof=" << m_maxNofElements;
		rt << ", norm=" << m_norm;
		return rt.str();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error mapping '%s' summarizer to string: %s"), "accuvariable", *m_errorhnd, std::string());
}


SummarizerFunctionInstanceInterface* SummarizerFunctionAccumulateVariable::createInstance(
		const QueryProcessorInterface* processor) const
{
	try
	{
		return new SummarizerFunctionInstanceAccumulateVariable( processor, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating instance of '%s' summarizer: %s"), "accuvariable", *m_errorhnd, 0);
}


SummarizerFunctionInterface::Description SummarizerFunctionAccumulateVariable::getDescription() const
{
	try
	{
		Description rt( _TXT("Accumulate the weights of all contents of a variable in matching expressions. Weights with same positions are grouped and multiplied, the group results are added to the sum, the total weight assigned to the variable content."));
		rt( Description::Param::Feature, "match", _TXT( "defines the query features to inspect for variable matches"), "");
		rt( Description::Param::String, "type", _TXT( "the forward index feature type for the content to extract"), "");
		rt( Description::Param::String, "var", _TXT( "the name of the variable referencing the content to weight"), "");
		rt( Description::Param::Numeric, "nof", _TXT( "the maximum number of the best weighted elements  to return (default 10)"), "1:");
		rt( Description::Param::Numeric, "norm", _TXT( "the normalization factor of the calculated weights (default 1.0)"), "0.0:1.0");
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating summarizer function description for '%s': %s"), "accuvariable", *m_errorhnd, SummarizerFunctionInterface::Description());
}

