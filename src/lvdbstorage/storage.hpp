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
#ifndef _STRUS_LVDB_STORAGE_HPP_INCLUDED
#define _STRUS_LVDB_STORAGE_HPP_INCLUDED
#include "strus/storageInterface.hpp"
#include "strus/index.hpp"
#include "strus/arithmeticVariant.hpp"
#include "databaseKey.hpp"
#include "metaDataBlock.hpp"
#include "metaDataRecord.hpp"
#include "metaDataBlockMap.hpp"
#include "metaDataBlockCache.hpp"
#include "metaDataReader.hpp"
#include "attributeMap.hpp"
#include "docnoBlock.hpp"
#include "docnoBlockMap.hpp"
#include "posinfoBlock.hpp"
#include "posinfoBlockMap.hpp"
#include "forwardIndexBlock.hpp"
#include "forwardIndexBlockMap.hpp"
#include "documentFrequencyMap.hpp"
#include "globalKeyMap.hpp"
#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <boost/thread/mutex.hpp>

namespace strus {

/// \brief Forward declaration
class PostingIteratorInterface;
/// \brief Forward declaration
class ForwardIteratorInterface;
/// \brief Forward declaration
class StorageInserterInterface;
/// \brief Forward declaration
class AttributeReaderInterface;
/// \brief Forward declaration
class MetaDataReaderInterface;


/// \brief Strus IR storage implementation based on LevelDB
class Storage
	:public StorageInterface
{
public:
	/// \param[in] path of the storage
	/// \param[in] cachesize_k number of K LRU cache for nodes
	Storage( const std::string& path_, unsigned int cachesize_k);
	virtual ~Storage();

	virtual void close();

	virtual PostingIteratorInterface*
			createTermPostingIterator(
				const std::string& termtype,
				const std::string& termid);

	virtual ForwardIteratorInterface*
			createForwardIterator(
				const std::string& type);

	virtual StorageInserterInterface*
			createInserter( const std::string& docid);

	virtual void deleteDocument( const std::string& docid);

	virtual MetaDataReaderInterface* createMetaDataReader() const;

	virtual AttributeReaderInterface* createAttributeReader() const;

	virtual Index nofDocumentsInserted() const;

	virtual Index maxDocumentNumber() const;

	virtual Index documentNumber( const std::string& docid) const;

	virtual std::vector<StatCounterValue> getStatistics() const;

public:
	void incrementDf( const Index& typeno, const Index& termno);
	void decrementDf( const Index& typeno, const Index& termno);

	enum {NofDocumentsInsertedBeforeAutoCommit=(1<<14)};

	void defineMetaData( const Index& docno, const std::string& varname, const ArithmeticVariant& value);
	void deleteMetaData( const Index& docno, const std::string& varname);
	void deleteMetaData( const Index& docno);

	void defineAttribute( const Index& docno, const std::string& varname, const std::string& value);
	void deleteAttribute( const Index& docno, const std::string& varname);
	void deleteAttributes( const Index& docno);

	void deleteIndex( const Index& docno);

	void defineDocnoPosting(
		const Index& termtype, const Index& termvalue,
		const Index& docno, unsigned int ff, float weight);

	void deleteDocnoPosting(
		const Index& termtype, const Index& termvalue,
		const Index& docno);

	void definePosinfoPosting(
		const Index& termtype, const Index& termvalue,
		const Index& docno, const std::vector<Index>& posinfo);

	void deletePosinfoPosting(
		const Index& termtype, const Index& termvalue,
		const Index& docno);

	void defineForwardIndexTerm(
		const Index& typeno, const Index& docno,
		const Index& pos, const std::string& termstring);

	void deleteForwardIndexTerm(
		const Index& typeno, const Index& docno,
		const Index& pos);

	void incrementNofDocumentsInserted();
	void decrementNofDocumentsInserted();

	Index maxTermValueNumber() const;

	Index getTermValue( const std::string& name) const;
	Index getTermType( const std::string& name) const;
	Index getDocno( const std::string& name) const;
	Index getAttributeName( const std::string& name) const;

	Index getOrCreateTermValue( const std::string& name);
	Index getOrCreateTermType( const std::string& name);
	Index getOrCreateAttributeName( const std::string& name);
	Index getOrCreateDocno( const std::string& name, bool& isNew);

	void checkFlush();
	void flush();
	void releaseInserter();

private:
	void writeInserterBatch();
	void aquireInserter();

private:
	std::string m_path;					///< levelDB storage path 
	leveldb::DB* m_db;					///< levelDB handle
	leveldb::Options m_dboptions;				///< options for levelDB

	Index m_next_termno;					///< next index to assign to a new term value
	Index m_next_typeno;					///< next index to assign to a new term type
	Index m_next_docno;					///< next index to assign to a new document id
	Index m_next_attribno;					///< next index to assign to a new attribute name
	Index m_nof_documents;					///< number of documents inserted
	boost::mutex m_nof_documents_mutex;			///< mutual exclusion for accessing m_nof_documents

	leveldb::WriteBatch m_inserter_batch;			///< batch used for an insert chunk written to disk with 'flush()', resp. 'writeInserterBatch()'

	MetaDataDescription m_metadescr;			///< description of the meta data

	DocumentFrequencyMap* m_dfMap;				///< temporary map for the document frequency of new inserted features
	AttributeMap* m_attributeMap;				///< map of document attributes for writing
	MetaDataBlockMap* m_metaDataBlockMap;			///< map of meta data blocks for writing
	MetaDataBlockCache* m_metaDataBlockCache;		///< read cache for meta data blocks

	DocnoBlockMap* m_docnoBlockMap;				///< map of docno postings for writing
	PosinfoBlockMap* m_posinfoBlockMap;			///< map of posinfo postings for writing
	ForwardIndexBlockMap* m_forwardIndexBlockMap;		///< map of forward index for writing

	GlobalKeyMap* m_termTypeMap;				///< map of term types
	GlobalKeyMap* m_termValueMap;				///< map of term values
	GlobalKeyMap* m_docIdMap;				///< map of document ids
	GlobalKeyMap* m_variableMap;				///< map of global variables (counters)
	GlobalKeyMap* m_attributeNameMap;			///< map of document attribute names

	boost::mutex m_nofInserterCnt_mutex;			///< mutual exclusion for aquiring inserter
	unsigned int m_nofInserterCnt;				///< counter of inserters
	Index m_flushCnt;					///< counter to do a commit after some inserts
};

}
#endif


