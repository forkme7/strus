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
#include "weightingBM25.hpp"
#include "strus/constants.hpp"
#include <cmath>

using namespace strus;

WeightingBM25::WeightingBM25(
	const StorageInterface* storage_,
	float b_,
	float k1_,
	float avgDocLength_)
		:WeightingIdfBased(storage_)
		,m_storage(storage_)
		,m_docno(0)
		,m_b(b_)
		,m_k1(k1_)
		,m_avgDocLength(avgDocLength_)
{}

WeightingBM25::WeightingBM25( const WeightingBM25& o)
	:WeightingIdfBased(o)
	,m_storage(o.m_storage)
	,m_docno(o.m_docno)
	,m_b(o.m_b)
	,m_k1(o.m_k1)
	,m_avgDocLength(o.m_avgDocLength)
{}

float WeightingBM25::call( IteratorInterface& itr)
{
	if (!idf_calculated())
	{
		calculateIdf( itr);
	}
	float relativeDocLen
		= (float)m_storage->documentAttributeNumeric(
				m_docno, Constants::DOC_ATTRIBUTE_DOCLEN)
		/ m_avgDocLength;

	return idf()
		* ((float)itr.frequency() * (m_k1 + 1.0))
		/ ((float)itr.frequency() + m_k1 * (1.0 - m_b + m_b * relativeDocLen));
}



