/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "storageTransaction.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "strus/storageDocumentInterface.hpp"
#include "strus/storageDocumentUpdateInterface.hpp"
#include "strus/statisticsBuilderInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "storageDocument.hpp"
#include "storageDocumentUpdate.hpp"
#include "storageClient.hpp"
#include "databaseAdapter.hpp"
#include "strus/numericVariant.hpp"
#include "strus/base/local_ptr.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <vector>
#include <string>
#include <set>
#include <map>

using namespace strus;

StorageTransaction::StorageTransaction(
		StorageClient* storage_,
		DatabaseClientInterface* database_,
		const MetaDataDescription* metadescr_,
		const Index& maxtypeno_,
		ErrorBufferInterface* errorhnd_)
	:m_storage(storage_)
	,m_database(database_)
	,m_metadescr(metadescr_)
	,m_attributeMap(database_)
	,m_metaDataMap(database_,metadescr_)
	,m_invertedIndexMap(database_)
	,m_forwardIndexMap(database_,maxtypeno_)
	,m_userAclMap(database_)
	,m_termTypeMap(database_,DatabaseKey::TermTypePrefix,DatabaseKey::TermTypeInvPrefix,storage_->createTypenoAllocator())
	,m_termValueMap(database_,DatabaseKey::TermValuePrefix,DatabaseKey::TermValueInvPrefix,storage_->createTermnoAllocator())
	,m_docIdMap(database_,DatabaseKey::DocIdPrefix,storage_->createDocnoAllocator())
	,m_userIdMap(database_,DatabaseKey::UserNamePrefix,storage_->createUsernoAllocator())
	,m_attributeNameMap(database_,DatabaseKey::AttributeKeyPrefix,storage_->createAttribnoAllocator())
	,m_termTypeMapInv()
	,m_termValueMapInv()
	,m_explicit_dfmap(database_)
	,m_nof_deleted_documents(0)
	,m_nof_documents_affected(0)
	,m_commit(false)
	,m_rollback(false)
	,m_errorhnd(errorhnd_)
{
	if (m_storage->getStatisticsProcessor() != 0)
	{
		m_termTypeMap.defineInv( &m_termTypeMapInv);
		m_termValueMap.defineInv( &m_termValueMapInv);
	}
}

StorageTransaction::~StorageTransaction()
{
	if (!m_rollback && !m_commit) rollback();
}

Index StorageTransaction::lookUpTermValue( const std::string& name)
{
	return m_termValueMap.lookUp( name);
}

Index StorageTransaction::getOrCreateTermValue( const std::string& name)
{
	return m_termValueMap.getOrCreate( name);
}

Index StorageTransaction::getOrCreateTermType( const std::string& name)
{
	return m_termTypeMap.getOrCreate( utils::tolower( name));
}

Index StorageTransaction::getOrCreateDocno( const std::string& name)
{
	return m_docIdMap.getOrCreate( name);
}

Index StorageTransaction::getOrCreateUserno( const std::string& name)
{
	return m_userIdMap.getOrCreate( name);
}

Index StorageTransaction::getOrCreateAttributeName( const std::string& name)
{
	return m_attributeNameMap.getOrCreate( utils::tolower( name));
}

void StorageTransaction::defineMetaData( const Index& docno, const std::string& varname, const NumericVariant& value)
{
	m_metaDataMap.defineMetaData( docno, varname, value);
}

void StorageTransaction::deleteMetaData( const Index& docno, const std::string& varname)
{
	m_metaDataMap.deleteMetaData( docno, varname);
}

void StorageTransaction::deleteMetaData( const Index& docno)
{
	m_metaDataMap.deleteMetaData( docno);
}

void StorageTransaction::defineAttribute( const Index& docno, const std::string& varname, const std::string& value)
{
	Index varno = getOrCreateAttributeName( varname);
	m_attributeMap.defineAttribute( docno, varno, value);
}

void StorageTransaction::deleteAttribute( const Index& docno, const std::string& varname)
{
	Index varno = getOrCreateAttributeName( varname);
	m_attributeMap.deleteAttribute( docno, varno);
}

void StorageTransaction::deleteAttributes( const Index& docno)
{
	m_attributeMap.deleteAttributes( docno);
}

void StorageTransaction::defineAcl( const Index& userno, const Index& docno)
{
	m_userAclMap.defineUserAccess( userno, docno);
}

void StorageTransaction::deleteAcl( const Index& userno, const Index& docno)
{
	m_userAclMap.deleteUserAccess( userno, docno);
}

void StorageTransaction::deleteAcl( const Index& docno)
{
	m_userAclMap.deleteDocumentAccess( docno);
}

void StorageTransaction::definePosinfoPosting(
	const Index& termtype, const Index& termvalue,
	const Index& docno, const std::vector<Index>& posinfo)
{
	m_invertedIndexMap.definePosinfoPosting(
		termtype, termvalue, docno, posinfo);
}

void StorageTransaction::openForwardIndexDocument( const Index& docno)
{
	m_forwardIndexMap.openForwardIndexDocument( docno);
}

void StorageTransaction::defineForwardIndexTerm(
	const Index& typeno,
	const Index& pos,
	const std::string& termstring)
{
	m_forwardIndexMap.defineForwardIndexTerm( typeno, pos, termstring);
}

void StorageTransaction::closeForwardIndexDocument()
{
	m_forwardIndexMap.closeForwardIndexDocument();
}

void StorageTransaction::deleteIndex( const Index& docno)
{
	m_invertedIndexMap.deleteIndex( docno);
	m_forwardIndexMap.deleteIndex( docno);
}

void StorageTransaction::deleteDocSearchIndexType( const Index& docno, const Index& typeno)
{
	m_invertedIndexMap.deleteIndex( docno, typeno);
}

void StorageTransaction::deleteDocForwardIndexType( const Index& docno, const Index& typeno)
{
	m_forwardIndexMap.deleteIndex( docno, typeno);
}

void StorageTransaction::deleteUserAccessRights(
	const std::string& username)
{
	try
	{
		Index userno = m_userIdMap.lookUp( username);
		if (userno != 0)
		{
			m_userAclMap.deleteUserAccess( userno);
		}
	}
	CATCH_ERROR_MAP( _TXT("error deleting document user access rights in transaction: %s"), *m_errorhnd);
}

void StorageTransaction::deleteDocument( const std::string& docid)
{
	try
	{
		Index docno = m_docIdMap.lookUp( docid);
		if (docno == 0) return;

		//[1] Delete metadata:
		deleteMetaData( docno);

		//[2] Delete attributes:
		deleteAttributes( docno);

		//[3] Delete index elements (forward index and inverted index):
		deleteIndex( docno);

		//[4] Delete ACL elements:
		deleteAcl( docno);

		//[5] Delete the document id
		m_docIdMap.deleteKey( docid);
		m_nof_deleted_documents += 1;
	}
	CATCH_ERROR_MAP( _TXT("error deleting document in transaction: %s"), *m_errorhnd);
}

StorageDocumentInterface*
	StorageTransaction::createDocument(
		const std::string& docid)
{
	try
	{
		Index dn = m_docIdMap.getOrCreate( docid);
		return new StorageDocument( this, docid, dn, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating document in transaction: %s"), *m_errorhnd, 0);
}

StorageDocumentUpdateInterface*
	StorageTransaction::createDocumentUpdate(
		const Index& docno_)
{
	try
	{
		return new StorageDocumentUpdate( this, docno_, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating document update in transaction: %s"), *m_errorhnd, 0);
}

void StorageTransaction::updateMetaData(
		const Index& docno, const std::string& varname, const NumericVariant& value)
{
	try
	{
		defineMetaData( docno, varname, value);
	}
	CATCH_ERROR_MAP( _TXT("error updating document meta data in transaction: %s"), *m_errorhnd);
}

void StorageTransaction::updateDocumentFrequency( const std::string& type, const std::string& value, int df_change)
{
	try
	{
		Index typeno = getOrCreateTermType( type);
		Index termno = getOrCreateTermValue( value);
		m_explicit_dfmap.increment( typeno, termno, df_change);
	}
	CATCH_ERROR_MAP( _TXT("error updating document frequency of a feature in transaction: %s"), *m_errorhnd);
}

bool StorageTransaction::commit()
{
	// Structure to make the rollback method be called in case of an exeption
	struct StatisticsBuilderScope
	{
		StatisticsBuilderScope( StatisticsBuilderInterface* obj_)
			:m_obj(obj_)
		{
			if (m_obj) m_obj->start();
		}
		void done()
		{
			m_obj = 0;
		}
		~StatisticsBuilderScope()
		{
			if (m_obj) m_obj->rollback();
		}

	private:
		 StatisticsBuilderInterface* m_obj;
	};
	if (m_errorhnd->hasError())
	{
		return false;
	}
	if (m_commit)
	{
		m_errorhnd->report( _TXT( "called transaction commit twice"));
		return false;
	}
	if (m_rollback)
	{
		m_errorhnd->report( _TXT( "called transaction commit after rollback"));
		return false;
	}
	if (m_errorhnd->hasError())
	{
		m_errorhnd->explain( _TXT( "storage transaction with error: %s"));
		return false;
	}
	try
	{
		StorageClient::TransactionLock lock( m_storage);
		//... we need a lock because transactions need to be sequentialized

		StatisticsBuilderInterface* statisticsBuilder = m_storage->getStatisticsBuilder();
		DocumentFrequencyCache* dfcache = m_storage->getDocumentFrequencyCache();

		strus::local_ptr<DatabaseTransactionInterface> transaction( m_database->createTransaction());
		if (!transaction.get())
		{
			m_errorhnd->explain( _TXT( "error creating transaction: %s"));
			return false;
		}
		std::map<Index,Index> termnoUnknownMap;
		m_termValueMap.getWriteBatch( termnoUnknownMap, transaction.get());
		std::map<Index,Index> docnoUnknownMap;
		int nof_new_documents = 0;
		int nof_chg_documents = 0;
		m_docIdMap.getWriteBatch( docnoUnknownMap, transaction.get(), &nof_new_documents, &nof_chg_documents);
		int nof_documents_incr = nof_new_documents - m_nof_deleted_documents;
		std::vector<Index> refreshList;
		m_attributeMap.renameNewDocNumbers( docnoUnknownMap);
		m_attributeMap.getWriteBatch( transaction.get());
		m_metaDataMap.renameNewDocNumbers( docnoUnknownMap);
		m_metaDataMap.getWriteBatch( transaction.get(), refreshList);

		m_invertedIndexMap.renameNewNumbers( docnoUnknownMap, termnoUnknownMap);

		StatisticsBuilderScope statisticsBuilderScope( statisticsBuilder);
		DocumentFrequencyCache::Batch dfbatch;

		m_invertedIndexMap.getWriteBatch(
				transaction.get(),
				statisticsBuilder, dfcache?&dfbatch:(DocumentFrequencyCache::Batch*)0,
				m_termTypeMapInv, m_termValueMapInv);
		if (statisticsBuilder)
		{
			statisticsBuilder->setNofDocumentsInsertedChange( nof_documents_incr);
		}
		m_forwardIndexMap.renameNewDocNumbers( docnoUnknownMap);
		m_forwardIndexMap.getWriteBatch( transaction.get());

		m_explicit_dfmap.renameNewTermNumbers( termnoUnknownMap);
		m_explicit_dfmap.getWriteBatch( transaction.get(), statisticsBuilder,
						dfcache?&dfbatch:(DocumentFrequencyCache::Batch*)0,
						m_termTypeMapInv, m_termValueMapInv);

		m_userAclMap.renameNewDocNumbers( docnoUnknownMap);
		m_userAclMap.getWriteBatch( transaction.get());

		m_storage->getVariablesWriteBatch( transaction.get(), nof_documents_incr);
		if (!transaction->commit())
		{
			m_errorhnd->explain(_TXT("error in database transaction commit: %s"));
			return false;
		}
		if (dfcache)
		{
			dfcache->writeBatch( dfbatch);
		}
		m_storage->declareNofDocumentsInserted( nof_documents_incr);
		m_storage->releaseTransaction( refreshList);
		statisticsBuilderScope.done();

		m_commit = true;
		m_nof_documents_affected = nof_new_documents + nof_chg_documents + m_nof_deleted_documents;
		clearMaps();
		return true;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error in transaction commit: %s"), *m_errorhnd, false);
}

void StorageTransaction::rollback()
{
	if (m_rollback)
	{
		m_errorhnd->report( _TXT( "called transaction rollback twice"));
		return;
	}
	if (m_commit)
	{
		m_errorhnd->report( _TXT( "called transaction rollback after commit"));
		return;
	}
	std::vector<Index> refreshList;
	m_storage->releaseTransaction( refreshList);
	m_rollback = true;
	m_nof_documents_affected = 0;
	clearMaps();
}

void StorageTransaction::clearMaps()
{
	m_attributeMap.clear();
	m_metaDataMap.clear();

	m_invertedIndexMap.clear();
	m_forwardIndexMap.clear();
	m_userAclMap.clear();

	m_termTypeMap.clear();
	m_termValueMap.clear();
	m_docIdMap.clear();
	m_userIdMap.clear();
	m_attributeNameMap.clear();

	m_termTypeMapInv.clear();
	m_termValueMapInv.clear();

	m_explicit_dfmap.clear();
}

