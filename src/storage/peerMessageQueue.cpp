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
/// \brief Implementation of input and an output queue for the peer messages of the StorageClient
/// \file peerMessageQueue.cpp
#include "peerMessageQueue.hpp"
#include "peerStorageTransaction.hpp"
#include "strus/peerMessageViewerInterface.hpp"
#include "strus/peerMessageBuilderInterface.hpp"
#include "strus/peerMessageProcessorInterface.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"

using namespace strus;

PeerMessageQueue::PeerMessageQueue(
		StorageClient* storage_,
		DatabaseClientInterface* database_,
		const PeerMessageProcessorInterface* proc_,
		ErrorBufferInterface* errorhnd_)
	:m_storage(storage_)
	,m_database(database_)
	,m_proc(proc_)
	,m_peerMessageBuilder()
	,m_bufferedPeerMessageIdx(0)
	,m_errorhnd(errorhnd_)
{
	PeerMessageProcessorInterface::BuilderOptions options( PeerMessageProcessorInterface::BuilderOptions::InsertInLexicalOrder);
	m_peerMessageBuilder.reset( m_proc->createBuilder( options));
	if (!m_peerMessageBuilder.get())
	{
		throw strus::runtime_error(_TXT("error creating peer message builder: %s"), m_errorhnd->fetchError());
	}
}

void PeerMessageQueue::start( bool sign)
{
	int nofdocs = m_storage->localNofDocumentsInserted();
	m_peerMessageBuilder->setNofDocumentsInsertedChange( sign?nofdocs:-nofdocs);
	m_peerMessageBuilder->start();

	std::map<Index,std::size_t> typenomap;
	std::map<Index,std::size_t> termnomap;
	std::string strings;

	// Fill a map with the strings of all types in the collection:
	{
		DatabaseAdapter_TermType::Cursor typecursor( m_database);
		Index typeno;
		std::string typestr;
		for (bool more=typecursor.loadFirst( typestr, typeno); more;
			more=typecursor.loadNext( typestr, typeno))
		{
			typenomap[ typeno] = strings.size();
			strings.append( typestr);
			strings.push_back( '\0');
		}
	}
	// Fill a map with the strings of all terms in the collection:
	{
		DatabaseAdapter_TermValue::Cursor termcursor( m_database);
		Index termno;
		std::string termstr;
		for (bool more=termcursor.loadFirst( termstr, termno); more;
			more=termcursor.loadNext( termstr, termno))
		{
			termnomap[ termno] = strings.size();
			strings.append( termstr);
			strings.push_back( '\0');
		}
	}
	// Feed all df changes to the peer message builder:
	{
		Index typeno;
		Index termno;
		Index df;

		DatabaseAdapter_DocFrequency::Cursor dfcursor( m_database);
		for (bool more=dfcursor.loadFirst( typeno, termno, df); more;
			more=dfcursor.loadNext( typeno, termno, df))
		{
			std::map<Index,std::size_t>::const_iterator ti;
			ti = typenomap.find( typeno);
			if (ti == typenomap.end()) throw strus::runtime_error( _TXT( "encountered undefined type when populating df's"));
			const char* typenam = strings.c_str() + ti->second;
	
			ti = termnomap.find( termno);
			if (ti == termnomap.end()) throw strus::runtime_error( _TXT( "encountered undefined term when populating df's"));
			const char* termnam = strings.c_str() + ti->second;
	
			m_peerMessageBuilder->addDfChange( typenam, termnam, sign?df:-df, sign/*isNew*/);
		}
	}
}

PeerMessageQueue::~PeerMessageQueue()
{}

void PeerMessageQueue::push( const char* inmsg, std::size_t inmsgsize, const char*& outmsg, std::size_t& outmsgsize)
{
	try
	{
		PeerStorageTransaction transaction( m_storage, m_database, m_storage->getDocumentFrequencyCache(), m_proc, m_errorhnd);
		m_peerReplyMessageBuffer = transaction.run( inmsg, inmsgsize);
		outmsg = m_peerReplyMessageBuffer.c_str();
		outmsgsize = m_peerReplyMessageBuffer.size();
	}
	CATCH_ERROR_MAP( _TXT("error pushing peer message to storage: %s"), *m_errorhnd);
}

bool PeerMessageQueue::fetch( const char*& msg, std::size_t& msgsize)
{
	if (m_errorhnd->hasError())
	{
		m_errorhnd->explain( _TXT("calling peer message queue fetch with pending error: %s"));
		return false;
	}
	try
	{
		if (m_peerMessageBuilder.get())
		{
			bool rt = true;
			do
			{
				rt = m_peerMessageBuilder->fetchMessage( msg, msgsize);
			}
			while (rt && msgsize == 0);
			if (rt)
			{
				return rt;
			}
			else if (m_errorhnd->hasError())
			{
				m_errorhnd->explain( _TXT("error fetching initialization peer message from storage: %s"));
				return false;
			}
			else
			{
				m_peerMessageBuilder.reset();
			}
		}
		return m_storage->fetchPeerUpdateMessage( msg, msgsize);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error fetching peer message from storage: %s"), *m_errorhnd, false);
}


