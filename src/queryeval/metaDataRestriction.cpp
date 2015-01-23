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
#include "metaDataRestriction.hpp"
#include <boost/algorithm/string.hpp>
#include <limits>

using namespace strus;

#define EPSILON_FLOAT32 std::numeric_limits<float>::epsilon()

bool MetaDataRestriction::compareFunctionEqualFloat32( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	float val1 = op1;
	float val2 = op2;
	return (val1 + EPSILON_FLOAT32 >= val2 && val1 <= val2 + EPSILON_FLOAT32);
}

bool MetaDataRestriction::compareFunctionLessFloat32( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	float val1 = op1;
	float val2 = op2;
	return (val1 + EPSILON_FLOAT32 < val2);
}

bool MetaDataRestriction::compareFunctionLessEqualFloat32( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	float val1 = op1;
	float val2 = op2;
	return (val1 <= val2 + EPSILON_FLOAT32);
}

bool MetaDataRestriction::compareFunctionGreaterFloat32( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	float val1 = op1;
	float val2 = op2;
	return (val1 > val2 + EPSILON_FLOAT32);
}

bool MetaDataRestriction::compareFunctionGreaterEqualFloat32( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	float val1 = op1;
	float val2 = op2;
	return (val1 + EPSILON_FLOAT32 >= val2);
}

#define EPSILON_FLOAT16 0.0004887581f

bool MetaDataRestriction::compareFunctionEqualFloat16( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	float val1 = op1;
	float val2 = op2;
	return (val1 + EPSILON_FLOAT16 >= val2 && val1 <= val2 + EPSILON_FLOAT16);
}

bool MetaDataRestriction::compareFunctionLessFloat16( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	float val1 = op1;
	float val2 = op2;
	return (val1 + EPSILON_FLOAT16 < val2);
}

bool MetaDataRestriction::compareFunctionLessEqualFloat16( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	float val1 = op1;
	float val2 = op2;
	return (val1 <= val2 + EPSILON_FLOAT16);
}

bool MetaDataRestriction::compareFunctionGreaterFloat16( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	float val1 = op1;
	float val2 = op2;
	return (val1 > val2 + EPSILON_FLOAT16);
}

bool MetaDataRestriction::compareFunctionGreaterEqualFloat16( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	float val1 = op1;
	float val2 = op2;
	return (val1 + EPSILON_FLOAT16 >= val2);
}

bool MetaDataRestriction::compareFunctionEqualInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	return (int)op1 == (int)op1;
}

bool MetaDataRestriction::compareFunctionLessInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	return (int)op1 < (int)op2;
}

bool MetaDataRestriction::compareFunctionLessEqualInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	return (int)op1 <= (int)op2;
}

bool MetaDataRestriction::compareFunctionGreaterInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	return (int)op1 > (int)op2;
}

bool MetaDataRestriction::compareFunctionGreaterEqualInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	return (int)op1 >= (int)op2;
}

bool MetaDataRestriction::compareFunctionEqualUInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	return (unsigned int)op1 == (unsigned int)op2;
}

bool MetaDataRestriction::compareFunctionLessUInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	return (unsigned int)op1 < (unsigned int)op2;
}

bool MetaDataRestriction::compareFunctionLessEqualUInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	return (unsigned int)op1 <= (unsigned int)op2;
}

bool MetaDataRestriction::compareFunctionGreaterUInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	return (unsigned int)op1 > (unsigned int)op2;
}

bool MetaDataRestriction::compareFunctionGreaterEqualUInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	return (unsigned int)op1 >= (unsigned int)op2;
}

CompareFunction MetaDataRestriction::getCompareFunction( const char* type, QueryInterface::CompareOperator cmpop)
{
	if (iequals( type, "float16"))
	{
		switch (cmpop)
		{
			case QueryInterface::CompareLess:
				return &compareFunctionLessFloat16;
			case QueryInterface::CompareLessEqual:
				return &compareFunctionLessEqualFloat16;
			case QueryInterface::CompareEqual:
				return &compareFunctionEqualFloat16;
			case QueryInterface::CompareGreater:
				return &compareFunctionGreaterFloat16;
			case QueryInterface::CompareGreaterEqual:
				return &compareFunctionGreaterEqualFloat16;
		}
	}
	else if (iequals( type, "float32"))
	{
		switch (cmpop)
		{
			case QueryInterface::CompareLess:
				return &compareFunctionLessFloat32;
			case QueryInterface::CompareLessEqual:
				return &compareFunctionLessEqualFloat32;
			case QueryInterface::CompareEqual:
				return &compareFunctionEqualFloat32;
			case QueryInterface::CompareGreater:
				return &compareFunctionGreaterFloat32;
			case QueryInterface::CompareGreaterEqual:
				return &compareFunctionGreaterEqualFloat32;
		}
	}
	else if (istarts_with( type, "int"))
	{
		switch (cmpop)
		{
			case QueryInterface::CompareLess:
				return &compareFunctionLessInt;
			case QueryInterface::CompareLessEqual:
				return &compareFunctionLessEqualInt;
			case QueryInterface::CompareEqual:
				return &compareFunctionEqualInt;
			case QueryInterface::CompareGreater:
				return &compareFunctionGreaterInt;
			case QueryInterface::CompareGreaterEqual:
				return &compareFunctionGreaterEqualInt;
		}
	}
	else if (istarts_with( type, "uint"))
	{
		switch (cmpop)
		{
			case QueryInterface::CompareLess:
				return &compareFunctionLessUInt;
			case QueryInterface::CompareLessEqual:
				return &compareFunctionLessEqualUInt;
			case QueryInterface::CompareEqual:
				return &compareFunctionEqualUInt;
			case QueryInterface::CompareGreater:
				return &compareFunctionGreaterUInt;
			case QueryInterface::CompareGreaterEqual:
				return &compareFunctionGreaterEqualUInt;
		}
	}
	else
	{
		throw std::runtime_error( std::string("unknown type in meta data restriction: '") + type + "'");
	}
}

bool MetaDataRestriction::match( MetaDataReaderInterface* md)
{
	return func( md->getValue( elementHandle), operand);
}

bool strus::matchesMetaDataRestriction(
		const std::vector<MetaDataRestriction>& restr,
		MetaDataReaderInterface* md)
{
	std::vector<MetaDataRestriction>::const_iterator ri = restr.begin(), re = restr.end();
	while (ri != re)
	{
		bool val = ri->match( md);
		for (++ri; ri != re && !ri->newGroup; ++ri)
		{
			val |= ri->match( md);
		}
		if (!val) return false;
	}
	return true;
}
