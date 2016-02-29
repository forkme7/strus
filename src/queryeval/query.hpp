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
#ifndef _STRUS_QUERY_HPP_INCLUDED
#define _STRUS_QUERY_HPP_INCLUDED
#include "strus/queryInterface.hpp"
#include "strus/summarizationVariable.hpp"
#include "strus/reference.hpp"
#include "private/internationalization.hpp"
#include "strus/metaDataRestrictionInterface.hpp"
#include <vector>
#include <string>
#include <map>
#include <utility>
#include <iostream>
#include <stdexcept>

namespace strus {

/// \brief Forward declaration
class QueryProcessorInterface;
/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class QueryEval;
/// \brief Forward declaration
class PostingIteratorInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Implementation of the query interface
class Query
	:public QueryInterface
{
public:
	///\brief Constructor
	Query(
			const QueryEval* queryEval_,
			const StorageClientInterface* storage_,
			ErrorBufferInterface* errorhnd_);

	virtual ~Query(){}

	virtual void pushTerm( const std::string& type_, const std::string& value_);
	virtual void pushExpression(
			const PostingJoinOperatorInterface* operation,
			std::size_t argc, int range_, unsigned int cardinality_);

	virtual void attachVariable( const std::string& name_);
	virtual void defineFeature( const std::string& set_, float weight_=1.0);

	virtual void addMetaDataRestrictionCondition(
			MetaDataRestrictionInterface::CompareOperator opr, const std::string& name,
			const ArithmeticVariant& operand, bool newGroup);

	virtual void addDocumentEvaluationSet(
			const std::vector<Index>& docnolist_);

	virtual void setMaxNofRanks( std::size_t nofRanks_);
	virtual void setMinRank( std::size_t minRank_);
	virtual void addUserName( const std::string& username_);

	virtual void defineTermStatistics(
			const std::string& type_,
			const std::string& value_,
			const TermStatistics& stats_);
	virtual void defineGlobalStatistics(
			const GlobalStatistics& stats_);

	virtual QueryResult evaluate();

public:
	typedef int NodeAddress;

	enum NodeType
	{
		NullNode,
		TermNode,
		ExpressionNode
	};
	static const char* nodeTypeName( NodeType i)
	{
		static const char* ar[] = {"NullNode","TermNode","ExpressionNode"};
		return ar[i];
	}

	static NodeType nodeType( NodeAddress nd)
	{
		return (nd < 0)?ExpressionNode:((nd == 0)?NullNode:TermNode);
	}
	static std::size_t nodeIndex( NodeAddress nd)
	{
		return (std::size_t)((nd > 0)?(nd-1):(-nd-1));
	}
	NodeAddress nodeAddress( NodeType type, std::size_t idx)
	{
		switch (type)
		{
			case NullNode: return 0;
			case TermNode: return +(int)(idx+1);
			case ExpressionNode: return -(int)(idx+1);
		}
		throw strus::logic_error( _TXT( "illegal node address type (query)"));
	}

	struct Term
	{
		Term( const Term& o)
			:type(o.type),value(o.value){}
		Term( const std::string& t, const std::string& v)
			:type(t),value(v){}

		bool operator<( const Term& o) const;

		std::string type;	///< term type name
		std::string value;	///< term value
	};

	struct Expression
	{
		Expression(
				const PostingJoinOperatorInterface* operation_,
				const std::vector<NodeAddress>& subnodes_,
				int range_,
				unsigned int cardinality_)
			:operation(operation_),subnodes(subnodes_),range(range_),cardinality(cardinality_){}
		Expression(
				const Expression& o)
			:operation(o.operation),subnodes(o.subnodes),range(o.range),cardinality(o.cardinality){}

		const PostingJoinOperatorInterface* operation;
		std::vector<NodeAddress> subnodes;
		int range;
		unsigned int cardinality;
	};

	struct Feature
	{
		Feature( const std::string& set_, NodeAddress node_, float weight_)
			:set(set_),node(node_),weight(weight_){}
		Feature( const Feature& o)
			:set(o.set),node(o.node),weight(o.weight){}

		std::string set;	///< name of the set the feature belongs to
		NodeAddress node;	///< feature expression tree or single node reference
		float weight;		///< feature weight in ranking
	};

	///\brief Get the joined features defined in the query
	const std::vector<Feature>& features() const	{return m_features;}

	void print( std::ostream& out) const;

private:
	const TermStatistics& getTermStatistics( const std::string& type_, const std::string& value_) const;

	struct NodeStorageData
	{
		explicit NodeStorageData( PostingIteratorInterface* itr_=0)
			:itr(itr_),stats(){}
		NodeStorageData( PostingIteratorInterface* itr_, TermStatistics stats_)
			:itr(itr_),stats(stats_){}
		NodeStorageData( const NodeStorageData& o)
			:itr(o.itr),stats(o.stats){}

		PostingIteratorInterface* itr;
		TermStatistics stats;
	};

	typedef std::map<NodeAddress,NodeStorageData> NodeStorageDataMap;

	PostingIteratorInterface* createExpressionPostingIterator( const Expression& expr, NodeStorageDataMap& nodeStorageDataMap);
	PostingIteratorInterface* createNodePostingIterator( const NodeAddress& nodeadr, NodeStorageDataMap& nodeStorageDataMap);
	void collectSummarizationVariables(
				std::vector<SummarizationVariable>& variables,
				const NodeAddress& nodeadr,
				const NodeStorageDataMap& nodeStorageDataMap);
	const NodeStorageData& nodeStorageData( const NodeAddress& nodeadr, const NodeStorageDataMap& nodeStorageDataMap) const;

	void printNode( std::ostream& out, NodeAddress adr, std::size_t indent) const;
	void printVariables( std::ostream& out, NodeAddress adr) const;
	NodeAddress duplicateNode( NodeAddress adr);
	void printStack( std::ostream& out, std::size_t indent) const;

private:
	const QueryEval* m_queryEval;
	const StorageClientInterface* m_storage;
	Reference<MetaDataReaderInterface> m_metaDataReader;
	Reference<MetaDataRestrictionInterface> m_metaDataRestriction;
	std::vector<Term> m_terms;
	std::vector<Expression> m_expressions;
	std::vector<Feature> m_features;
	std::vector<NodeAddress> m_stack;
	std::multimap<NodeAddress,std::string> m_variableAssignments;
	std::size_t m_nofRanks;
	std::size_t m_minRank;
	std::vector<std::string> m_usernames;
	std::vector<Index> m_evalset_docnolist;
	bool m_evalset_defined;
	std::map<Term,TermStatistics> m_termstatsmap;
	GlobalStatistics m_globstats;
	ErrorBufferInterface* m_errorhnd;		///< buffer for error messages
};

}//namespace
#endif

