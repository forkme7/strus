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
#ifndef _STRUS_STORAGE_HPP_INCLUDED
#define _STRUS_STORAGE_HPP_INCLUDED
#include "strus/storageClientInterface.hpp"
#include "strus/index.hpp"
#include "strus/arithmeticVariant.hpp"
#include "strus/reference.hpp"
#include "private/utils.hpp"
#include "metaDataBlockCache.hpp"
#include "indexSetIterator.hpp"
#include "strus/statisticsProcessorInterface.hpp"
namespace strus {

/// \brief Forward declaration
class PostingIteratorInterface;
/// \brief Forward declaration
class ForwardIteratorInterface;
/// \brief Forward declaration
class DocumentTermIteratorInterface;
/// \brief Forward declaration
class InvAclIteratorInterface;
/// \brief Forward declaration
class StorageTransactionInterface;
/// \brief Forward declaration
class StorageDocumentInterface;
/// \brief Forward declaration
class AttributeReaderInterface;
/// \brief Forward declaration
class MetaDataReaderInterface;
/// \brief Forward declaration
class KeyAllocatorInterface;
/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class DocumentFrequencyCache;
/// \brief Forward declaration
class StorageDumpInterface;
/// \brief Forward declaration
class ErrorBufferInterface;


/// \brief Implementation of the StorageClientInterface
class StorageClient
	:public StorageClientInterface
{
public:
	/// \param[in] database key value store database used by this storage (ownership passed to this)
	/// \param[in] termnomap_source end of line separated list of terms to cache for eventually faster lookup
	/// \param[in] statisticsProc_ peer message processor interface
	/// \param[in] errorhnd_ error buffering interface for error handling
	StorageClient(
			DatabaseClientInterface* database_,
			const char* termnomap_source,
			const StatisticsProcessorInterface* statisticsProc_,
			ErrorBufferInterface* errorhnd_);
	virtual ~StorageClient();

	virtual PostingIteratorInterface*
			createTermPostingIterator(
				const std::string& termtype,
				const std::string& termid) const;

	virtual ForwardIteratorInterface*
			createForwardIterator(
				const std::string& type) const;

	virtual DocumentTermIteratorInterface*
			createDocumentTermIterator(
				const std::string& type) const;

	virtual InvAclIteratorInterface*
			createInvAclIterator(
				const std::string& username) const;

	virtual StorageTransactionInterface*
			createTransaction();

	virtual StorageDocumentInterface*
			createDocumentChecker(
				const std::string& docid,
				const std::string& logfilename) const;

	virtual MetaDataReaderInterface* createMetaDataReader() const;

	virtual AttributeReaderInterface* createAttributeReader() const;

	virtual StatisticsIteratorInterface* createInitStatisticsIterator( bool sign);

	virtual StatisticsIteratorInterface* createUpdateStatisticsIterator();

	virtual const StatisticsProcessorInterface* getStatisticsProcessor() const;

	virtual Index nofDocumentsInserted() const;

	virtual Index documentFrequency(
			const std::string& type,
			const std::string& term) const;

	virtual Index maxDocumentNumber() const;

	virtual Index documentNumber( const std::string& docid) const;

	virtual ValueIteratorInterface* createTermTypeIterator() const;

	virtual ValueIteratorInterface* createTermValueIterator() const;

	virtual ValueIteratorInterface* createDocIdIterator() const;

	virtual ValueIteratorInterface* createUserNameIterator() const;

	virtual Index documentStatistics(
			const Index& docno,
			const DocumentStatisticsType& stat,
			const std::string& type) const;

	virtual bool checkStorage( std::ostream& errorlog) const;

	virtual StorageDumpInterface* createDump() const;

public:/*QueryEval,AttributeReader,documentTermIterator*/
	Index getTermValue( const std::string& name) const;
	Index getTermType( const std::string& name) const;
	Index getDocno( const std::string& name) const;
	Index getUserno( const std::string& name) const;
	Index getAttributeno( const std::string& name) const;
	std::vector<std::string> getAttributeNames() const;
	Index userId( const std::string& username) const;

public:/*StorageTransaction*/
	void getVariablesWriteBatch(
			DatabaseTransactionInterface* transaction,
			int nof_documents_incr);

	void releaseTransaction( const std::vector<Index>& refreshList);

	void declareNofDocumentsInserted( int incr);
	Index nofAttributeTypes();

	KeyAllocatorInterface* createTypenoAllocator();
	KeyAllocatorInterface* createDocnoAllocator();
	KeyAllocatorInterface* createUsernoAllocator();
	KeyAllocatorInterface* createAttribnoAllocator();
	KeyAllocatorInterface* createTermnoAllocator();

	bool withAcl() const;

	Index allocTermno();
	Index allocDocno();

	Index allocTypenoImm( const std::string& name, bool& isNew);///< immediate allocation of a term type
	Index allocUsernoImm( const std::string& name, bool& isNew); ///< immediate allocation of a user number
	Index allocAttribnoImm( const std::string& name, bool& isNew);///< immediate allocation of a attribute number

	StatisticsBuilderInterface* getStatisticsBuilder();

	friend class TransactionLock;
	class TransactionLock
	{
	public:
		TransactionLock( StorageClient* storage_)
			:m_mutex(&storage_->m_transaction_mutex)
		{
			m_mutex->lock();
		}
		~TransactionLock()
		{
			m_mutex->unlock();
		}

	private:
		utils::Mutex* m_mutex;
	};

public:/*StatisticsBuilder*/
	Index documentFrequency( const Index& typeno, const Index& termno) const;

public:/*StorageDocumentChecker*/
	IndexSetIterator getAclIterator( const Index& docno) const;
	IndexSetIterator getUserAclIterator( const Index& userno) const;

public:/*StatisticsIterator*/
	///\brief Get the document frequency cache
	DocumentFrequencyCache* getDocumentFrequencyCache();
	///\brief Fetch a message from a storage update transaction
	bool fetchPeerUpdateMessage( const char*& msg, std::size_t& msgsize);

public:/*strusResizeBlocks*/
	Index maxTermTypeNo() const;

private:
	void cleanup();
	void loadTermnoMap( const char* termnomap_source);
	void loadVariables( DatabaseClientInterface* database_);
	void storeVariables();
	void fillDocumentFrequencyCache();

private:
	Reference<DatabaseClientInterface> m_database;		///< reference to key value store database
	utils::AtomicCounter<Index> m_next_typeno;		///< next index to assign to a new term type
	utils::AtomicCounter<Index> m_next_termno;		///< next index to assign to a new term value
	utils::AtomicCounter<Index> m_next_docno;		///< next index to assign to a new document id
	utils::AtomicCounter<Index> m_next_userno;		///< next index to assign to a new user id
	utils::AtomicCounter<Index> m_next_attribno;		///< next index to assign to a new attribute name

	utils::AtomicCounter<Index> m_nof_documents;		///< number of documents inserted

	utils::Mutex m_transaction_mutex;			///< mutual exclusion in the critical part of a transaction

	MetaDataDescription m_metadescr;			///< description of the meta data
	MetaDataBlockCache* m_metaDataBlockCache;		///< read cache for meta data blocks

	const StatisticsProcessorInterface* m_statisticsProc;	///< peer message processor
	Reference<StatisticsBuilderInterface> m_statisticsBuilder; ///< builder of peer messages from updates by transactions
	Reference<DocumentFrequencyCache> m_documentFrequencyCache; ///< reference to document frequency cache

	ErrorBufferInterface* m_errorhnd;			///< error buffer for exception free interface
};

}
#endif


