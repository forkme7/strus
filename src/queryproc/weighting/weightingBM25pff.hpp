/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_WEIGHTING_BM25PFF_HPP_INCLUDED
#define _STRUS_WEIGHTING_BM25PFF_HPP_INCLUDED
#include "strus/weightingFunctionInterface.hpp"
#include "strus/weightingFunctionInstanceInterface.hpp"
#include "strus/weightingFunctionContextInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/index.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "private/internationalization.hpp"
#include "private/utils.hpp"
#include "proximityWeightAccumulator.hpp"
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>

namespace strus
{
/// \brief Forward declaration
class ErrorBufferInterface;

/// \class WeightingFunctionContextBM25pff
/// \brief Weighting function based on the BM25pff (BM25 with proximity feature frequency) formula
class WeightingFunctionContextBM25pff
	:public WeightingFunctionContextInterface
{
public:
	WeightingFunctionContextBM25pff(
		const StorageClientInterface* storage,
		MetaDataReaderInterface* metadata_,
		double k1_,
		double b_,
		unsigned int windowsize_,
		unsigned int cardinality_,
		double ffbase_,
		unsigned int fftie_,
		double mindf_,
		double avgDocLength_,
		double titleinc_,
		unsigned int tidocnorm_,
		double nofCollectionDocuments_,
		const std::string& metadata_doclen_,
		const std::string& metadata_title_maxpos_,
		const std::string& metadata_title_size_,
		ErrorBufferInterface* errorhnd_);

	virtual void addWeightingFeature(
			const std::string& name_,
			PostingIteratorInterface* itr_,
			float weight_,
			const TermStatistics& stats_);

	virtual double call( const Index& docno);

private:
	enum {MaxNofArguments=64};				///< chosen to fit in a bitfield of 64 bits
	double m_k1;						///< k1 value of BM25
	double m_b;						///< b value of BM25
	unsigned int m_windowsize;				///< maximum position range of a window considered for weighting
	unsigned int m_cardinality;				///< minumum number of features in a window considered for weighting
	double m_ffbase;					///< relative constant base factor of pure ff [0..1]
	unsigned int m_fftie;					///< the maximum pure ff value that is considered for weighting (used for normalization of pure ff part)
	double m_maxdf;						///< the maximum df of features considered for proximity weighing as fraction of the total collection size
	double m_avgDocLength;					///< average document length in the collection
	double m_titleinc;					///< ff increment for title features
	unsigned int m_tidocnorm;				///< the document size used for calibrating the title match weight normalization between 0 and 1 (0->0 titleinc_docsizenorm and bigger->1).
	double m_nofCollectionDocuments;			///< number of documents in the collection
	ProximityWeightAccumulator::WeightArray m_idfar;	///< array of idfs
	PostingIteratorInterface* m_itrar[ MaxNofArguments];	///< array if weighted features
	PostingIteratorInterface* m_structar[ MaxNofArguments];	///< array of end of structure elements
	PostingIteratorInterface* m_paraar[ MaxNofArguments];	///< array of end of paragraph elements
	std::size_t m_itrarsize;				///< number of weighted features
	std::size_t m_structarsize;				///< number of end of structure elements
	std::size_t m_paraarsize;				///< number of paragraph elements (now summary accross paragraph borders)
	std::size_t m_nof_maxdf_features;			///< number of features with a df bigger than maximum
	Index m_maxdist_featar[ MaxNofArguments];		///< array of distances indicating what proximity distance is considered at maximum for same sentence weight
	double m_normfactorar[ MaxNofArguments];		///< normalization factor taking missing features in a window into account
	ProximityWeightAccumulator::WeightArray m_weightincr;	///< array of proportional weight increments 
	bool m_initialized;					///< true, if the structures have already been initialized
	MetaDataReaderInterface* m_metadata;			///< meta data reader
	int m_metadata_doclen;					///< meta data doclen handle
	int m_metadata_title_maxpos;				///< meta data title maximum position handle
	int m_metadata_title_size;				///< meta data title size
	ErrorBufferInterface* m_errorhnd;			///< buffer for error messages
};


/// \class WeightingFunctionInstanceBM25pff
/// \brief Weighting function instance based on the BM25pff formula
class WeightingFunctionInstanceBM25pff
	:public WeightingFunctionInstanceInterface
{
public:
	explicit WeightingFunctionInstanceBM25pff( ErrorBufferInterface* errorhnd_)
		:m_k1(1.5),m_b(0.75),m_avgdoclen(1000),m_titleinc(0.0),m_tidocnorm(0)
		,m_windowsize(100),m_cardinality(0)
		,m_ffbase(0.4),m_fftie(0),m_maxdf(0.5),m_errorhnd(errorhnd_){}

	virtual ~WeightingFunctionInstanceBM25pff(){}

	virtual void addStringParameter( const std::string& name, const std::string& value);
	virtual void addNumericParameter( const std::string& name, const ArithmeticVariant& value);

	virtual WeightingFunctionContextInterface* createFunctionContext(
			const StorageClientInterface* storage_,
			MetaDataReaderInterface* metadata,
			const GlobalStatistics& stats) const;

	virtual std::string tostring() const;

private:
	double m_k1;					///< BM25 k1 parameter
	double m_b;					///< BM25 b parameter
	double m_avgdoclen;				///< average document length
	double m_titleinc;				///< ff increment for title features
	unsigned int m_tidocnorm;			///< the document size used for calibrating the title match weight normalization between 0 and 1 (0->0 titleinc_docsizenorm and bigger->1).
	std::string m_metadata_doclen;			///< attribute defining the document length
	std::string m_metadata_title_maxpos;		///< (optional) meta data element defining the last title position
	std::string m_metadata_title_size;		///< (optional) meta data element defining the size of the title
	unsigned int m_windowsize;			///< size of window for proximity weighting
	unsigned int m_cardinality;			///< minimal number of query features in a window
	double m_ffbase;				///< base used for feature frequency calculation
	unsigned int m_fftie;				///< the maximum pure ff value that is considered for weighting (used for normalization of pure ff part)
	double m_maxdf;					///< the maximum df of features considered for proximity weighing as fraction of the total collection size
	ErrorBufferInterface* m_errorhnd;		///< buffer for error messages
};


/// \class WeightingFunctionBM25pff
/// \brief Weighting function based on the BM25pff formula
class WeightingFunctionBM25pff
	:public WeightingFunctionInterface
{
public:
	explicit WeightingFunctionBM25pff( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}

	virtual ~WeightingFunctionBM25pff(){}

	virtual WeightingFunctionInstanceInterface* createInstance() const;

	virtual Description getDescription() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif

