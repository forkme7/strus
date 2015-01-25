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
#ifndef _STRUS_STORAGE_MAIN_INCLUDE_HPP_INCLUDED
#define _STRUS_STORAGE_MAIN_INCLUDE_HPP_INCLUDED
#include "strus/arithmeticVariant.hpp"
#include "strus/attributeReaderInterface.hpp"
#include "strus/constants.hpp"
#include "strus/invAclIteratorInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/index.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/peerStorageInterface.hpp"
#include "strus/peerStorageTransactionInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/queryEvalInterface.hpp"
#include "strus/queryEvalLib.hpp"
#include "strus/queryInterface.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/queryProcessorLib.hpp"
#include "strus/resultDocument.hpp"
#include "strus/statCounterValue.hpp"
#include "strus/storageAlterMetaDataTableInterface.hpp"
#include "strus/storageDocumentInterface.hpp"
#include "strus/storageInterface.hpp"
#include "strus/storageLib.hpp"
#include "strus/storageTransactionInterface.hpp"
#include "strus/summarizerClosureInterface.hpp"
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/weightedDocument.hpp"
#include "strus/weightingClosureInterface.hpp"
#include "strus/weightingFunctionInterface.hpp"
#endif

