/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief LSH Similarity model structure
#include "lshmodel.hpp"
#include "private/internationalization.hpp"
#include "strus/base/stdint.h"
#include <arpa/inet.h>
#include <cstdlib>

using namespace strus;

LshModel::LshModel( std::size_t dim_, std::size_t nofbits_, std::size_t variations_)
	:m_dim(dim_),m_nofbits(nofbits_),m_variations(variations_)
	,m_modelMatrix( createModelMatrix( dim_, nofbits_))
{
	std::size_t wi=0, we=variations_;
	for (; wi != we; ++wi)
	{
		m_rotations.push_back( arma::randu<arma::mat>( m_dim, m_dim));
		if (std::abs( det( m_rotations.back())) < 0.01)
		{
			--wi;
			continue;
		}
	}
}

LshModel::LshModel( std::size_t dim_, std::size_t nofbits_, std::size_t variations_, const arma::mat& modelMatrix_, const std::vector<arma::mat>& rotations_)
	:m_dim(dim_),m_nofbits(nofbits_),m_variations(variations_)
	,m_modelMatrix(modelMatrix_),m_rotations(rotations_)
{
	std::vector<arma::mat>::const_iterator ri=m_rotations.begin(), re=m_rotations.end();
	for (; ri != re; ++ri)
	{
		if (std::abs( det( *ri)) < 0.01)
		{
			throw strus::runtime_error( _TXT( "illegal rotation matrix in model"));
		}
	}
	if (std::abs( det( m_modelMatrix)) < 0.01)
	{
		throw strus::runtime_error( _TXT( "illegal main matrix in model"));
	}
}


arma::mat LshModel::createModelMatrix( std::size_t dim_, std::size_t nofbits_)
{
	if (dim_ <= 0 || nofbits_ <= 0)
	{
		throw std::runtime_error( "illegal dimension or nofbits");
	}
	if (dim_ < nofbits_)
	{
		throw std::runtime_error( "dimension must me two times bigger than nofbits");
	}
	double step = (float) dim_ / (float) nofbits_;
	arma::mat rt = arma::mat( nofbits_, dim_);
	std::size_t ri = 0, re = nofbits_;
	for (; ri != re; ++ri)
	{
		unsigned int ci = (unsigned int)(ri * step);
		unsigned int ce = (unsigned int)((ri+1) * step);
		if (ce > dim_) ce = dim_;
		if ((ri+1) == re) ce = dim_;
		rt.row( ri).fill( -1.0 / (dim_ - (ce - ci)));
		double val = 1.0 / (ce - ci);
		for (; ci < ce; ++ci)
		{
			rt( ri, ci) = val;
		}
	}
	return rt;
}

std::string LshModel::tostring() const
{
	std::ostringstream rt;
	std::vector<arma::mat>::const_iterator roti = m_rotations.begin(), rote = m_rotations.end();
	for (; roti != rote; ++roti)
	{
		rt << *roti << std::endl;
	}
	rt << m_modelMatrix << std::endl;
	return rt.str();
}

SimHash LshModel::simHash( const arma::vec& vec) const
{
	std::vector<bool> rt;
	if (m_dim != vec.size())
	{
		throw strus::runtime_error( _TXT("vector must have dimension of model"));
	}
	std::vector<arma::mat>::const_iterator roti = m_rotations.begin(), rote = m_rotations.end();
	for (; roti != rote; ++roti)
	{
		arma::vec res = m_modelMatrix * (*roti) * vec;
		arma::vec::const_iterator resi = res.begin(), rese = res.end();
		for (; resi != rese; ++resi)
		{
			rt.push_back( *resi >= 0.0);
		}
	}
	return SimHash( rt);
}

union PackedDouble
{
	double double_;
	uint32_t u32_[2];
};

struct DumpStructHeader
{
	DumpStructHeader()
		:dim(0),nofbits(0),variations(0),arsize(0){}
	DumpStructHeader( std::size_t dim_, std::size_t nofbits_, std::size_t variations_)
		:dim(dim_),nofbits(nofbits_),variations(variations_),arsize(0){}

	uint32_t dim;
	uint32_t nofbits;
	uint32_t variations;
	uint32_t arsize;

	void conv_hton()
	{
		dim = htonl(dim);
		nofbits = htonl(nofbits);
		variations = htonl(variations);
		arsize = htonl(arsize);
	}

	void conv_ntoh()
	{
		dim = ntohl(dim);
		nofbits = ntohl(nofbits);
		variations = ntohl(variations);
		arsize = ntohl(arsize);
	}
};

struct DumpStruct
	:public DumpStructHeader
{
	DumpStruct( std::size_t dim_, std::size_t nofbits_, std::size_t variations_)
		:DumpStructHeader(dim_,nofbits_,variations_),ar(0)
	{
		std::size_t nofFloats = (dim * nofbits) + (dim * dim * variations);
		arsize = nofFloats * 2;
		ar = (uint32_t*)std::malloc( arsize * sizeof(ar[0]));
		if (!ar) throw std::bad_alloc();
	}

	double getValue( std::size_t aidx)
	{
		PackedDouble rt;
		rt.u32_[0] = ar[ aidx*2+0];
		rt.u32_[1] = ar[ aidx*2+1];
		return rt.double_;
	}

	void setValue( std::size_t aidx, double val)
	{
		PackedDouble rt;
		rt.double_ = val;
		ar[ aidx*2+0] = rt.u32_[0];
		ar[ aidx*2+1] = rt.u32_[1];
	}

	std::size_t nofValues() const
	{
		return arsize * 2;
	}

	void loadValues( const void* ar_)
	{
		const uint32_t* ua = (const uint32_t*)ar_;
		std::size_t ai=0, ae=arsize;
		for (; ai != ae; ++ai)
		{
			ar[ ai*2+0] = ntohl( ua[ ai*2+0]);
			ar[ ai*2+1] = ntohl( ua[ ai*2+0]);
		}
	}

	~DumpStruct()
	{
		if (ar) std::free( ar);
	}

	void conv_hton()
	{
		std::size_t ai=0, ae=arsize;
		DumpStructHeader::conv_hton();
		for (; ai != ae; ++ai)
		{
			ar[ ai] = htonl( ar[ ai]);
		}
	}

	void conv_ntoh()
	{
		DumpStructHeader::conv_ntoh();
		std::size_t ai=0, ae=arsize;
		for (; ai != ae; ++ai)
		{
			ar[ ai] = ntohl( ar[ ai]);
		}
	}

	const void* getValuePtr() const
	{
		return (const void*)ar;
	}
	std::size_t getValuePtrSize() const
	{
		std::size_t nofFloats = (dim * nofbits) + (dim * dim * variations);
		if (arsize != nofFloats * 2) throw std::runtime_error( _TXT( "LSH model structure is corrupt"));
		return arsize * sizeof(ar[0]);
	}

private:
	uint32_t* ar;

private:
	DumpStruct( const DumpStruct&){} //non copyable, because we do not know LE of BE conversion state
};


std::string LshModel::serialize() const
{
	DumpStruct st( m_dim, m_nofbits, m_variations);

	std::size_t aidx = 0;
	std::vector<arma::mat>::const_iterator roti = m_rotations.begin(), rote = m_rotations.end();
	for (; roti != rote; ++roti)
	{
		arma::mat::const_iterator ri = roti->begin(), re = roti->end();
		for (; ri != re; ++ri)
		{
			st.setValue( aidx++, *ri);
		}
	}
	arma::mat::const_iterator mi = m_modelMatrix.begin(), me = m_modelMatrix.end();
	for (; mi != me; ++mi)
	{
		st.setValue( aidx++, *mi);
	}
	std::size_t valuePtrSize = st.getValuePtrSize();
	st.conv_hton();
	std::string rt;
	rt.append( (const char*)(const void*)&st, sizeof( DumpStructHeader));
	rt.append( (const char*)(const void*)st.getValuePtr(), valuePtrSize);
	return rt;
}

LshModel* LshModel::createFromSerialization( const std::string& dump)
{
	DumpStructHeader hdr;
	std::memcpy( &hdr, dump.c_str(), sizeof( DumpStructHeader));
	hdr.conv_ntoh();

	DumpStruct st( hdr.dim, hdr.nofbits, hdr.variations);
	st.loadValues( dump.c_str() + sizeof( DumpStructHeader));
	std::size_t ai=0, ae=st.nofValues();

	arma::mat modelMatrix( hdr.nofbits, hdr.dim);
	std::vector<arma::mat> rotations;

	std::size_t ri=0, re=hdr.variations;
	for (; ri != re; ++ri)
	{
		arma::mat rot( hdr.dim, hdr.dim);
		arma::mat::iterator mi = rot.begin(), me=rot.end();
		for (; mi != me; ++mi,++ai)
		{
			*mi = st.getValue( ai);
		}
		rotations.push_back( rot);
	}

	arma::mat::iterator mi = modelMatrix.begin(), me=modelMatrix.end();
	for (; mi != me; ++mi,++ai)
	{
		*mi = st.getValue( ai);
	}
	if (ai != ae)
	{
		throw strus::runtime_error( _TXT( "lsh model dump is corrupt"));
	}
	return new LshModel( hdr.dim, hdr.nofbits, hdr.variations, modelMatrix, rotations);
}



