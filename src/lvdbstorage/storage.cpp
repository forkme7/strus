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
#include "strus/iteratorInterface.hpp"
#include "strus/forwardIndexViewerInterface.hpp"
#include "storage.hpp"
#include "iterator.hpp"
#include "nullIterator.hpp"
#include "transaction.hpp"
#include "forwardIndexViewer.hpp"
#include "indexPacker.hpp"
#include <string>
#include <vector>
#include <cstring>
#include <boost/thread/mutex.hpp>

using namespace strus;

Storage::Storage( const char* path_)
	:m_path(path_),m_db(0)
{
	leveldb::Options options;
	options.create_if_missing = false;

	leveldb::Status status = leveldb::DB::Open(options, path_, &m_db);
	if (status.ok())
	{
		m_next_termno = keyLookUp( VariablePrefix, "TermNo");
		m_next_typeno = keyLookUp( VariablePrefix, "TypeNo");
		m_next_docno = keyLookUp( VariablePrefix, "DocNo");
		m_nof_documents = keyLookUp( VariablePrefix, "NofDocs");
		if (m_nof_documents) m_nof_documents -= 1;
	}
	else
	{
		std::string err = status.ToString();
		if (!!m_db)
		{
			delete m_db;
			m_db = 0;
		}
		throw std::runtime_error( std::string( "failed to create storage: ") + err);
	}
}

void Storage::close()
{
	if (m_db)
	{
		flushDfs();

		leveldb::WriteBatch batch;
		batchDefineVariable( batch, "TermNo", m_next_termno);
		batchDefineVariable( batch, "TypeNo", m_next_typeno);
		batchDefineVariable( batch, "DocNo", m_next_docno);
		batchDefineVariable( batch, "NofDocs", m_nof_documents+1);
	
		leveldb::Status status = m_db->Write( leveldb::WriteOptions(), &batch);
		if (!status.ok())
		{
			throw std::runtime_error( std::string("error when flushing and closing storage") + status.ToString());
		}
		delete m_db;
		m_db = 0;
	}
}

Storage::~Storage()
{
	try
	{
		close();
	}
	catch (...)
	{
		//... silently ignored. Call close directly to catch errors
		if (m_db) delete m_db;
	}
}

void Storage::writeBatch( leveldb::WriteBatch& batch)
{
	if (!m_db) throw std::runtime_error("write on closed storage");
	leveldb::Status status = m_db->Write( leveldb::WriteOptions(), &batch);
	if (!status.ok())
	{
		throw std::runtime_error( status.ToString());
	}
}

void Storage::flushNewKeys()
{
	boost::mutex::scoped_lock( m_mutex);
	if (m_newKeyMap.size() == 0) return;

	leveldb::Status status = m_db->Write( leveldb::WriteOptions(), &m_newKeyBatch);
	if (!status.ok())
	{
		throw std::runtime_error( status.ToString());
	}
	m_newKeyBatch.Clear();
	m_newKeyMap.clear();
	m_nof_documents += 1;
}

void Storage::batchDefineVariable( leveldb::WriteBatch& batch, const char* name, Index value)
{
	std::string encoded_value;
	packIndex( encoded_value, value);
	batch.Put( Storage::keyString( Storage::VariablePrefix, name), encoded_value);
}

leveldb::Iterator* Storage::newIterator()
{
	if (!m_db) throw std::runtime_error("open read iterator on closed storage");
	return m_db->NewIterator( leveldb::ReadOptions());
}

std::string Storage::keyString( KeyPrefix prefix, const std::string& keyname)
{
	std::string rt;
	rt.push_back( (char)prefix);
	rt.append( keyname);
	return rt;
}

Index Storage::keyLookUp( const std::string& keystr) const
{
	if (!m_db) throw std::runtime_error("read on closed storage");
	std::string value;
	leveldb::Slice constkey( keystr.c_str(), keystr.size());
	leveldb::Status status = m_db->Get( leveldb::ReadOptions(), constkey, &value);
	if (status.IsNotFound())
	{
		return 0;
	}
	if (!status.ok())
	{
		throw std::runtime_error( status.ToString());
	}
	const char* cc = value.c_str();
	return unpackIndex( cc, cc + value.size());
}

Index Storage::keyLookUp( KeyPrefix prefix, const std::string& keyname) const
{

	std::string key = keyString( prefix, keyname);
	return keyLookUp( key);
}

Index Storage::keyGetOrCreate( KeyPrefix prefix, const std::string& keyname)
{
	if (!m_db) throw std::runtime_error("read on closed storage");
	std::string key = keyString( prefix, keyname);
	leveldb::Slice constkey( key.c_str(), key.size());
	std::string value;
	leveldb::Status status = m_db->Get( leveldb::ReadOptions(), constkey, &value);
	if (status.IsNotFound())
	{
		boost::mutex::scoped_lock( m_mutex);
		NewKeyMap::const_iterator ki = m_newKeyMap.find( key);
		if (ki != m_newKeyMap.end())
		{
			return ki->second;
		}
		leveldb::Status status = m_db->Get( leveldb::ReadOptions(), constkey, &value);
		if (status.IsNotFound())
		{
			Index rt = 0;
			if (prefix == TermTypePrefix)
			{
				rt = m_next_typeno++;
			}
			else if (prefix == TermValuePrefix)
			{
				rt = m_next_termno++;
			}
			else if (prefix == DocIdPrefix)
			{
				rt = m_next_docno++;
			}
			else
			{
				throw std::logic_error( "internal: Cannot create index value");
			}
			std::string valuebuf;
			packIndex( valuebuf, rt);
			m_newKeyMap[ key] = rt;
			m_newKeyBatch.Put( constkey, valuebuf);
			return rt;
		}
		else if (!status.ok())
		{
			throw std::runtime_error( status.ToString());
		}
	}
	else if (!status.ok())
	{
		throw std::runtime_error( status.ToString());
	}
	const char* cc = value.c_str();
	return unpackIndex( cc, cc + value.size());
}

IteratorInterface*
	Storage::createTermOccurrenceIterator(
		const std::string& typestr,
		const std::string& termstr)
{
	Index typeno = keyLookUp( TermTypePrefix, typestr);
	Index termno = keyLookUp( TermValuePrefix, termstr);
	if (!typeno || !termno)
	{
		return new NullIterator( typeno, termno, termstr.c_str());
	}
	return new Iterator( m_db, typeno, termno, termstr.c_str());
}

ForwardIndexViewerInterface*
	Storage::createForwardIndexViewer(
		const std::string& type)
{
	return new ForwardIndexViewer( this, m_db, type);
}

StorageInterface::TransactionInterface*
	Storage::createTransaction(
		const std::string& docid)
{
	return new Transaction( this, docid);
}

void Storage::incrementNofDocumentsInserted()
{
	boost::mutex::scoped_lock( m_mutex);
	++m_nof_documents;
}

Index Storage::nofDocumentsInserted() const
{
	return m_nof_documents;
}

Index Storage::maxDocumentNumber() const
{
	return m_next_docno-1;
}

Index Storage::documentNumber( const std::string& docid) const
{
	return keyLookUp( Storage::DocIdPrefix, docid);
}

std::string Storage::documentAttributeString( Index docno, char varname) const
{
	std::string key;
	key.push_back( (char)Storage::DocTextAttrPrefix);
	packIndex( key, docno);
	key.push_back( varname);

	leveldb::Slice constkey( key.c_str(), key.size());
	std::string value;
	leveldb::Status status = m_db->Get( leveldb::ReadOptions(), constkey, &value);
	if (status.IsNotFound())
	{
		return std::string();
	}
	if (!status.ok())
	{
		throw std::runtime_error( status.ToString());
	}
	return value;
}

float Storage::documentAttributeNumeric( Index docno, char varname) const
{
	std::string key;
	key.push_back( (char)Storage::DocNumAttrPrefix);
	packIndex( key, docno);
	key.push_back( varname);

	leveldb::Slice constkey( key.c_str(), key.size());
	std::string value;
	leveldb::Status status = m_db->Get( leveldb::ReadOptions(), constkey, &value);

	if (status.IsNotFound())
	{
		return 0.0;
	}
	if (!status.ok())
	{
		throw std::runtime_error( status.ToString());
	}
	const char* cc = value.c_str();
	return unpackFloat( cc, cc + value.size());
}


void Storage::incrementDf( Index typeno, Index termno)
{
	boost::mutex::scoped_lock( m_mutex);
	DfKey key( typeno, termno);
	m_dfMap[ key] += 1;
}

void Storage::decrementDf( Index typeno, Index termno)
{
	boost::mutex::scoped_lock( m_mutex);
	std::pair<Index,Index> key( typeno, termno);
	m_dfMap[ key] -= 1;
}

void Storage::flushDfs()
{
	if (!m_db)
	{
		throw std::runtime_error( "no storage defined (flush dfs)");
	}
	boost::mutex::scoped_lock( m_mutex);
	std::map<DfKey,Index>::const_iterator mi = m_dfMap.begin(), me = m_dfMap.end();
	leveldb::WriteBatch batch;

	for (; mi != me; ++mi)
	{
		std::string keystr;
		keystr.push_back( DocFrequencyPrefix);
		packIndex( keystr, mi->first.first);
		packIndex( keystr, mi->first.second);

		leveldb::Slice constkey( keystr.c_str(), keystr.size());
		std::string value;
		Index df;
		leveldb::Status status = m_db->Get( leveldb::ReadOptions(), constkey, &value);
		if (status.IsNotFound() || value.empty())
		{
			df = mi->second;
		}
		else
		{
			if (!status.ok())
			{
				throw std::runtime_error( status.ToString());
			}
			char const* cc = value.c_str();
			char const* ee = value.c_str() + value.size();
			df = mi->second + unpackIndex( cc, ee);
		}
		value.resize(0);
		packIndex( value, df);

		batch.Put( constkey, value);
	}
	writeBatch( batch);
	m_dfMap.clear();
}
