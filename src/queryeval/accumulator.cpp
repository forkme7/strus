#include "accumulator.hpp"
#include <cstdlib>
#include <limits>
#include <set>
#include <stdexcept>
#include <cmath>

using namespace strus;

Accumulator::Accumulator( const QueryProcessorInterface* qproc_)
	:m_queryprocessor(qproc_)
	,m_selectoridx(0)
	,m_docno(0)
{}

void Accumulator::addSelector(
		const IteratorInterface& iterator)
{
	IteratorReference itr( iterator.copy());
	if (m_queryprocessor->getEstimatedIdf( *itr)
		> std::numeric_limits<double>::epsilon())
	{
		m_selectors.push_back( itr);
	}
}

void Accumulator::addRanker(
		double factor,
		const std::string& function,
		const std::vector<float>& parameter,
		const IteratorInterface& iterator)
{
	WeightingFunctionReference weighting(
		m_queryprocessor->createWeightingFunction( function, parameter));
	IteratorReference itr( iterator.copy());

	m_rankers.push_back( AccumulatorArgument( factor, weighting, itr));
}


bool Accumulator::nextRank(
		Index& docno,
		unsigned int& selectorState,
		double& weight)
{
	// For all selectors:
	while (m_selectoridx < m_selectors.size())
	{
		// Select candidate document:
		m_docno = m_selectors[ m_selectoridx]->skipDoc( m_docno+1);
		if (!m_docno)
		{
			++m_selectoridx;
			continue;
		}
		if (m_visited.find( m_docno) != m_visited.end())
		{
			continue;
		}

		// Mark it as visited:
		m_visited.insert( docno = m_docno);
		selectorState = m_selectoridx+1;
		weight = 0.0;

		// Add a weight for every ranker that has a match:
		std::vector<AccumulatorArgument>::iterator ai = m_rankers.begin(), ae = m_rankers.end();

		for (; ai != ae; ++ai)
		{
			if (docno == ai->itr->skipDoc( docno))
			{
				weight += ai->function->call( *ai->itr) * ai->factor;
			}
		}
		return true;
	}
	return false;
}

