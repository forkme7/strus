/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Similarity hash structure
#include "simhash.hpp"
#include "private/bitOperations.hpp"
#include "private/internationalization.hpp"
#include <string>
#include <iostream>
#include <sstream>
#include <limits>

using namespace strus;

SimHash::SimHash( std::size_t size_, bool initval)
	:m_ar(),m_size(size_)
{
	std::size_t idx = 0;

	uint64_t elem = (initval)?std::numeric_limits<uint64_t>::max():0;
	for (; idx + NofElementBits <= m_size; idx+=NofElementBits)
	{
		m_ar.push_back( elem);
	}
	if (idx)
	{
		elem = 0;
		for (; idx > 0; ++idx)
		{
			elem <<= 1;
			elem |= initval ? 1:0;
		}
		m_ar.push_back( elem);
	}
}

SimHash::SimHash( const std::vector<bool>& bv)
	:m_ar(),m_size(bv.size())
{
	std::size_t idx = 0;
	for (; idx < m_size; idx+=NofElementBits)
	{
		uint64_t elem = 0;
		std::vector<bool>::const_iterator ai = bv.begin() + idx, ae = bv.end();
		unsigned int aidx=0;
		for (; ai != ae && aidx < NofElementBits; ++ai,++aidx)
		{
			elem <<= 1;
			elem |= *ai ? 1:0;
		}
		if (ai == ae && aidx < NofElementBits)
		{
			elem <<= (NofElementBits - aidx);
		}
		m_ar.push_back( elem);
	}
}

bool SimHash::operator[]( std::size_t idx) const
{
	std::size_t aridx = idx / NofElementBits;
	std::size_t arofs = idx % NofElementBits;
	if (aridx >= m_ar.size()) return false;
	return (m_ar[ aridx] & (1  << (NofElementBits-1 - arofs))) != 0;
}

void SimHash::set( std::size_t idx, bool value)
{
	std::size_t aridx = idx / NofElementBits;
	std::size_t arofs = idx % NofElementBits;
	if (aridx >= m_ar.size())
	{
		throw strus::runtime_error(_TXT("array bound write in %s"), "SimHash");
	}
	if (value)
	{
		m_ar[ aridx] |= (1  << (NofElementBits-1 - arofs));
	}
	else
	{
		m_ar[ aridx] &= ~(1  << (NofElementBits-1 - arofs));
	}
}

std::vector<std::size_t> SimHash::indices( bool what) const
{
	std::vector<std::size_t> rt;
	std::vector<uint64_t>::const_iterator ai = m_ar.begin(), ae = m_ar.end();
	std::size_t aridx = 0;
	for (; ai != ae; ++ai,++aridx)
	{
		std::size_t arofs = 0;
		uint64_t elem = (uint64_t)1 << (NofElementBits-1);
		for (; arofs < NofElementBits; ++arofs,elem>>=1)
		{
			if (((elem & *ai) != 0) == what)
			{
				rt.push_back( aridx * NofElementBits + arofs);
			}
		}
	}
	return rt;
}

unsigned int SimHash::count() const
{
	unsigned int rt = 0;
	std::vector<uint64_t>::const_iterator ai = m_ar.begin(), ae = m_ar.end();
	for (; ai != ae; ++ai)
	{
		rt += strus::BitOperations::bitCount( *ai);
	}
	return rt;
}

unsigned int SimHash::dist( const SimHash& o) const
{
	unsigned int rt = 0;
	std::vector<uint64_t>::const_iterator ai = m_ar.begin(), ae = m_ar.end();
	std::vector<uint64_t>::const_iterator oi = o.m_ar.begin(), oe = o.m_ar.end();
	for (; oi != oe && ai != ae; ++oi,++ai)
	{
		rt += strus::BitOperations::bitCount( *ai ^ *oi);
	}
	for (; oi != oe; ++oi)
	{
		rt += strus::BitOperations::bitCount( *oi);
	}
	for (; ai != ae; ++ai)
	{
		rt += strus::BitOperations::bitCount( *ai);
	}
	return rt;
}

bool SimHash::near( const SimHash& o, unsigned int dist) const
{
	unsigned int cnt = 0;
	std::vector<uint64_t>::const_iterator ai = m_ar.begin(), ae = m_ar.end();
	std::vector<uint64_t>::const_iterator oi = o.m_ar.begin(), oe = o.m_ar.end();
	for (; oi != oe && ai != ae; ++oi,++ai)
	{
		cnt += strus::BitOperations::bitCount( *ai ^ *oi);
		if (cnt > dist) return false;
	}
	for (; oi != oe; ++oi)
	{
		cnt += strus::BitOperations::bitCount( *oi);
		if (cnt > dist) return false;
	}
	for (; ai != ae; ++ai)
	{
		cnt += strus::BitOperations::bitCount( *ai);
		if (cnt > dist) return false;
	}
	return true;
}

std::string SimHash::tostring() const
{
	std::ostringstream rt;
	std::vector<uint64_t>::const_iterator ai = m_ar.begin(), ae = m_ar.end();
	for (unsigned int aidx=0; ai != ae; ++ai,++aidx)
	{
		if (aidx) rt << '|';
		uint64_t mi = 1;
		unsigned int cnt = m_size - (aidx * NofElementBits);
		mi <<= (NofElementBits-1);
		for (; mi && cnt; mi >>= 1, --cnt)
		{
			rt << ((*ai & mi)?'1':'0');
		}
	}
	return rt.str();
}





