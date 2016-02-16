/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
/// \brief Library providing some standard weighting functions
#include "weighting_standard.hpp"
#include "weightingFormula.hpp"
#include "weightingBM25.hpp"
#include "weightingBM25pff.hpp"
#include "weightingConstant.hpp"
#include "weightingMetadata.hpp"
#include "weightingFrequency.hpp"
#include "private/dll_tags.hpp"

using namespace strus;

WeightingFunctionInterface* strus::createWeightingFunctionFormula( ErrorBufferInterface* errorhnd)
{
	return new WeightingFunctionFormula( errorhnd);
}

WeightingFunctionInterface* strus::createWeightingFunctionBm25( ErrorBufferInterface* errorhnd)
{
	return new WeightingFunctionBM25( errorhnd);
}

WeightingFunctionInterface* strus::createWeightingFunctionBm25pff( ErrorBufferInterface* errorhnd)
{
	return new WeightingFunctionBM25pff( errorhnd);
}

WeightingFunctionInterface* strus::createWeightingFunctionConstant( ErrorBufferInterface* errorhnd)
{
	return new WeightingFunctionConstant( errorhnd);
}

WeightingFunctionInterface* strus::createWeightingFunctionMetadata( ErrorBufferInterface* errorhnd)
{
	return new WeightingFunctionMetadata( errorhnd);
}

WeightingFunctionInterface* strus::createWeightingFunctionTermFrequency( ErrorBufferInterface* errorhnd)
{
	return new WeightingFunctionTermFrequency( errorhnd);
}




