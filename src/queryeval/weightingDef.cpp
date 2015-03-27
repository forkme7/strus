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
#include "weightingDef.hpp"
#include "queryEval.hpp"
#include "private/utils.hpp"
#include "strus/weightingFunctionInterface.hpp"
#include "private/internationalization.hpp"

using namespace strus;

static std::size_t countArguments( char const** arg)
{
	if (!arg) return 0;
	std::size_t aidx = 0;
	for (; arg[aidx]; ++aidx){}
	return aidx;
}

WeightingDef::WeightingDef(
		const WeightingFunctionInterface* function_,
		const std::string& functionName_,
		const WeightingConfig& config,
		const std::vector<std::string>& weightingSets_)
	:m_function(function_),m_functionName(functionName_),m_weightingSets(weightingSets_)
{
	m_parameters.resize( countArguments( m_function->numericParameterNames()));

	std::map<std::string,ArithmeticVariant>::const_iterator
		pi = config.numericParameters().begin(),
		pe = config.numericParameters().end();

	const char** arg = m_function->numericParameterNames();
	if (!arg && pi != pe)
	{
		throw strus::runtime_error( _TXT( "no numeric arguments expected for weighting function '%s'"), m_functionName.c_str());
	}
	for (; pi != pe; ++pi)
	{
		std::size_t aidx = 0;
		for (; arg[aidx]; ++aidx)
		{
			if (utils::caseInsensitiveEquals( pi->first, arg[aidx]))
			{
				break;
			}
		}
		if (!arg[aidx])
		{
			throw strus::runtime_error( _TXT( "unknown numeric argument name '%s' for weighting function '"), pi->first.c_str(), m_functionName.c_str());
		}
		m_parameters[aidx] = pi->second;
	}
}



