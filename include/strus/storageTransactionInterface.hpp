/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for a transaction on the storage
/// \file storageTransactionInterface.hpp
#ifndef _STRUS_STORAGE_TRANSACTION_INTERFACE_HPP_INCLUDED
#define _STRUS_STORAGE_TRANSACTION_INTERFACE_HPP_INCLUDED
#include <string>
#include "strus/storageDocumentInterface.hpp"
#include "strus/arithmeticVariant.hpp"

namespace strus
{

/// \brief Forward declaration
class StorageDocumentInterface;
/// \brief Forward declaration
class StorageDocumentUpdateInterface;

/// \class StorageTransactionInterface
/// \brief Object to declare all items for one insert/update of a document in the storage
class StorageTransactionInterface
{
public:
	/// \brief Destructor
	/// \remark Expected to do an implicit rollback, if neither 'commit()' or 'rollback' was called
	virtual ~StorageTransactionInterface(){}

	/// \brief Create one document to be inserted/replaced within this transaction
	/// \param[in] docid_ identifier of the document
	/// \return the document object
	virtual StorageDocumentInterface*
		createDocument( const std::string& docid_)=0;

	/// \brief Create an interface for a document to be updated within this transaction
	/// \param[in] docno_ document number of the document (StorageClientInterface::documentNumber( const std::string&))
	/// \return the document updater object
	virtual StorageDocumentUpdateInterface*
		createDocumentUpdate(
			const Index& docno_)=0;

	/// \brief Declare a document to be removed from the storage within this transaction
	/// \param[in] docid document identifier (URI)
	virtual void deleteDocument(
			const std::string& docid)=0;

	/// \brief Declare the access rights of a user to any document to be removed from the storage within this transaction
	/// \param[in] username user name
	virtual void deleteUserAccessRights(
			const std::string& username)=0;

	/// \brief Update of meta data for a specific document (without creating it)
	/// \param[in] docno document number
	/// \param[in] varname meta data element name to update
	/// \param[in] value new value of this meta data element
	virtual void updateMetaData(
			const Index& docno,
			const std::string& varname,
			const ArithmeticVariant& value)=0;

	/// \brief Insert all documents and executes all commands defined in the transaction or none if one operation fails
	/// \return true on success, false on error
	virtual bool commit()=0;

	/// \brief Rollback of the transaction, no changes made
	virtual void rollback()=0;
};

}//namespace
#endif


