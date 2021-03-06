/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_KEY_MAP_INV_HPP_INCLUDED
#define _STRUS_STORAGE_KEY_MAP_INV_HPP_INCLUDED
#include "strus/index.hpp"
#include "private/stringMap.hpp"
#include "private/utils.hpp"
#include <cstdlib>
#include <string>
#include <map>

namespace strus {

class KeyMapInv
{
public:
	KeyMapInv(){}
	KeyMapInv( const KeyMapInv& o)
		:m_map(o.m_map),m_strings(o.m_strings){}

	void set( const Index& idx, const std::string& value)
	{
		m_strings.push_back( value);
		m_map[ idx] = m_strings.back();
	}

	const char* get( const Index& idx) const
	{
		Map::const_iterator ei = m_map.find( idx);
		if (ei == m_map.end()) return 0;
		return ei->second;
	}
	void clear()
	{
		m_map.clear();
		m_strings.clear();
	}
	void erase( const Index& idx)
	{
		m_map.erase( idx);
	}

private:
	typedef utils::UnorderedMap<Index,const char*> Map;
	Map m_map;
	StringVector m_strings;
};

}//namespace
#endif


