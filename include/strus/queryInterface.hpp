/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for defining a query and evaluating it.
/// \file "queryInterface.hpp"
#ifndef _STRUS_QUERY_INTERFACE_HPP_INCLUDED
#define _STRUS_QUERY_INTERFACE_HPP_INCLUDED
#include "strus/storageClientInterface.hpp"
#include "strus/metaDataRestrictionInterface.hpp"
#include "strus/queryResult.hpp"
#include "strus/numericVariant.hpp"
#include "strus/termStatistics.hpp"
#include "strus/globalStatistics.hpp"
#include <string>
#include <vector>
#include <utility>
#include <iostream>

namespace strus {

/// \brief Forward declaration
class PostingJoinOperatorInterface;

/// \brief Defines a strus information retrieval query
class QueryInterface
{
public:
	/// \brief Destructor
	virtual ~QueryInterface(){}

	/// \brief Push a term to the query stack
	/// \param[in] type_ term type
	/// \param[in] value_ term value
	/// \param[in] length_ term length (ordinal position count)
	virtual void pushTerm(
			const std::string& type_,
			const std::string& value_,
			const Index& length_)=0;

	/// \brief Push an expression on the query stack that defines a single field per document. The fields are represented as a sets of postings. The postings are defined as meta data elements describing the start and the end of the field for each document.
	/// \param[in] metadataRangeStart name of metadata element that defines the start position of the field in a document
	/// \param[in] metadataRangeEnd name of metadata element that defines the end position of the field (first position after the field) in a document
	/// \note fields can be used to identify elements in a certain document area without access of posting blocks and without assigning a different feature type and thus having different statistics
	virtual void pushDocField(
			const std::string& metadataRangeStart,
			const std::string& metadataRangeEnd)=0;

	/// \brief Push an expression formed by the topmost elements from the stack to the query stack,
	///	removing the argument elements.
	/// \param[in] operation the expression join operator
	/// \param[in] argc number of expression arguments
	/// \param[in] range position proximity range of the expression
	/// \param[in] cardinality specifies a result dimension requirement (e.g. minimum number of elements of any input subset selection that builds a result) (0 for use default). Interpretation depends on operation, but in most cases it specifies the required size for a valid result.
	virtual void pushExpression(
			const PostingJoinOperatorInterface* operation,
			unsigned int argc, int range, unsigned int cardinality)=0;

	/// \brief Attaches a variable to the top expression or term on the query stack.
	/// \note The positions of the query matches of the referenced term or expression can be accessed through this variable in summarization.
	/// \param[in] name_ name of the variable attached
	/// \remark The stack is not changed
	/// \remark More than one variable can be attached
	virtual void attachVariable( const std::string& name_)=0;

	/// \brief Define the topmost element of the stack as feature, removing it from the stack
	/// \param[in] set_ name of the set of the new feature created
	/// \param[in] weight_ weight of the feature for the weighting function in query evaluation 
	virtual void defineFeature( const std::string& set_, double weight_=1.0)=0;

	/// \brief Define the statistics of a term for the case that they are defined by the client (for example in a system with a distributed index)
	/// \param[in] type_ term type
	/// \param[in] value_ term value
	/// \param[in] stats_ term statistics
	virtual void defineTermStatistics(
			const std::string& type_,
			const std::string& value_,
			const TermStatistics& stats_)=0;

	/// \brief Define the global statistics for the case that they are defined by the client
	/// \param[in] stats_ global statistics
	virtual void defineGlobalStatistics(
			const GlobalStatistics& stats_)=0;

	/// \brief Add a condition clause to the restriction on the document meta data of this query
	/// \param[in] opr condition compare operator
	/// \param[in] name name of meta data element to check
	/// \param[in] operand constant number to check against
	/// \param[in] newGroup true, if the conditional opens a new group of elements joined with a logical "OR" 
	///			false, if the conditional belongs to the last group of elements joined with a logical "OR".
	///		Different groups are joined with a logical "AND" to form the meta data restriction expression
	virtual void addMetaDataRestrictionCondition(
			const MetaDataRestrictionInterface::CompareOperator& opr,
			const std::string& name,
			const NumericVariant& operand,
			bool newGroup=true)=0;

	/// \brief Define a restriction on the documents as list of local document numbers (Add local document numbers to the list of documents to restrict the query on)
	/// \param[in] docnolist_ list of documents to evaluate the query on
	virtual void addDocumentEvaluationSet(
			const std::vector<Index>& docnolist_)=0;

	/// \brief Add a restriction for documents accessible by this query
	/// \param[in] username_ name of user role that is allowed to see the result documents
	virtual void addAccess( const std::string& username_)=0;

	/// \brief Set the maximum number of ranks to evaluate starting with the minimum rank
	/// \param[in] maxNofRanks_ maximum number of ranks
	virtual void setMaxNofRanks( std::size_t maxNofRanks_)=0;
	/// \brief Set the minimum rank number to return
	/// \param[in] minRank_ the minimum rank number
	virtual void setMinRank( std::size_t minRank_)=0;

	/// \brief Set the value of a variable in the weigthing formula defined with QueryEval::defineWeightingFormula(ScalarFunctionInterface* combinefunc) or in a weighting function or a summarizer
	/// \param[in] name name of the variable
	/// \param[in] value value of the variable
	virtual void setWeightingVariableValue( const std::string& name, double value)=0;

	/// \brief Switch debug mode on or off (default off). In case of debug mode additional attributes defined with the function definitions in the query evaluation are attached to the result.
	/// \param[in] debug true for enabling debug mode on and false for disabling debug mode (diabled by default)
	virtual void setDebugMode( bool debug)=0;

	/// \brief Evaluate the query
	/// \return result of query evaluation
	virtual QueryResult evaluate() const=0;

	/// \brief Map query to readable string
	/// \return query with query evaluation scheme in readable form
	virtual std::string tostring() const=0;
};

}//namespace
#endif
