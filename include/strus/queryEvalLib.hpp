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
/// \brief Exported functions of the strus query evaluation library
#ifndef _STRUS_QUERYEVAL_LIB_HPP_INCLUDED
#define _STRUS_QUERYEVAL_LIB_HPP_INCLUDED
#include "strus/weightedDocument.hpp"
#include <string>

namespace strus {

/// \brief Forward declaration query evaluation program
class QueryEvalInterface;

/// \brief Create a program for query evaluation
/// \param[in] evalProgramSource the source of the query evaluation procedure
/// \return the program reference
QueryEvalInterface*
	createQueryEval(
		const std::string& evalProgramSource);

}//namespace
#endif
