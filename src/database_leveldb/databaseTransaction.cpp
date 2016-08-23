/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "databaseTransaction.hpp"
#include "databaseCursor.hpp"
#include "databaseClient.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/databaseOptions.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <memory>
#include <cstring>
#include <stdexcept>

using namespace strus;

DatabaseTransaction::DatabaseTransaction( leveldb::DB* db_, DatabaseClient* database_, ErrorBufferInterface* errorhnd_)
	:m_database(database_),m_db(db_),m_commit_called(false),m_rollback_called(false),m_errorhnd(errorhnd_)
{}

DatabaseTransaction::~DatabaseTransaction()
{
	if (!m_commit_called && !m_rollback_called) rollback();
}

DatabaseCursorInterface* DatabaseTransaction::createCursor( const DatabaseOptions& options) const
{
	try
	{
		return m_database->createCursor( options);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating database cursor: %s"), *m_errorhnd, 0);
}

void DatabaseTransaction::write(
			const char* key,
			std::size_t keysize,
			const char* value,
			std::size_t valuesize)
{
	try
	{
		m_batch.Put(
			leveldb::Slice( key, keysize),
			leveldb::Slice( value, valuesize));
	}
	CATCH_ERROR_MAP( _TXT("error writing element in database transaction: %s"), *m_errorhnd);
}

void DatabaseTransaction::remove(
			const char* key,
			std::size_t keysize)
{
	try
	{
		m_batch.Delete( leveldb::Slice( key, keysize));
	}
	CATCH_ERROR_MAP( _TXT("error removing element in database transaction: %s"), *m_errorhnd);
}

void DatabaseTransaction::removeSubTree(
		const char* domainkey,
		std::size_t domainkeysize)
{
	try
	{
		std::auto_ptr<leveldb::Iterator> itr( m_db->NewIterator( leveldb::ReadOptions()));
		for (itr->Seek( leveldb::Slice( domainkey,domainkeysize));
			itr->Valid()
				&& domainkeysize <= itr->key().size()
				&& 0==std::memcmp( itr->key().data(), domainkey, domainkeysize);
			itr->Next())
		{
			m_batch.Delete( itr->key());
		}
	}
	CATCH_ERROR_MAP( _TXT("error removing subtree in database transaction: %s"), *m_errorhnd);
}

bool DatabaseTransaction::commit()
{
	try
	{
		if (m_errorhnd->hasError())
		{
			m_errorhnd->explain( _TXT( "database transaction with error: %s"));
			return false;
		}
		leveldb::WriteOptions options;
		options.sync = true;
		leveldb::Status status = m_db->Write( options, &m_batch);
		if (!status.ok())
		{
			std::string statusstr( status.ToString());
			m_errorhnd->report( _TXT( "error in commit when writing transaction batch: %s"), statusstr.c_str());
			return false;
		}
		m_batch.Clear();
		m_commit_called = true;
		return true;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error in database transaction commit: %s"), *m_errorhnd, false);
}

void DatabaseTransaction::rollback()
{
	m_batch.Clear();
	m_rollback_called = true;
}

