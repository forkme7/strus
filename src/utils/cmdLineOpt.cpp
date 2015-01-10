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
#include "dll_tags.hpp"
#include "strus/utils/cmdLineOpt.hpp"
#include <iostream>
#include <string>
#include <cstring>

using namespace strus;

DLL_PUBLIC void strus::printIndentMultilineString( std::ostream& out, std::size_t indentsize, const char* descr)
{
	std::string indent( indentsize, ' ');

	char const* cc = descr;
	char const* ee;
	do
	{
		ee = std::strchr( cc,'\n');
		std::string line = ee?std::string( cc, ee-cc):std::string( cc);
		std::cerr << indent << line << std::endl;
		cc = ee + 1;
	}
	while (ee);
}

