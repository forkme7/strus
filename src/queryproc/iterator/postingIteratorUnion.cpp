#include "iterator/postingIteratorUnion.hpp"
#include <cstdlib>

using namespace strus;


IteratorUnion::IteratorUnion( const std::vector<Reference<PostingIteratorInterface> >& args_)
	:m_docno(0)
	,m_posno(0)
	,m_argar(args_)
	,m_selected(0)
	,m_documentFrequency(-1)
{
	if (args_.size() > 64)
	{
		throw std::runtime_error( "number of arguments of union out of range (> 64)");
	}
	std::vector<Reference<PostingIteratorInterface> >::const_iterator
		ai = args_.begin(), ae = args_.end();
	for (int aidx=0; ai != ae; ++ai,++aidx)
	{
		if (aidx) m_featureid.push_back('=');
		m_featureid.append( (*ai)->featureid());
	}
	m_featureid.push_back( 'U');
}

IteratorUnion::~IteratorUnion()
{}

std::vector<const PostingIteratorInterface*>
	IteratorUnion::subExpressions( bool positive) const
{
	std::vector<const PostingIteratorInterface*> rt;
	if (positive)
	{
		rt.reserve( m_argar.size());
		std::vector<Reference<PostingIteratorInterface> >::const_iterator
			ai = m_argar.begin(), ae = m_argar.end();
		for (; ai != ae; ++ai)
		{
			rt.push_back( ai->get());
		}
	}
	return rt;
}

static inline Index selectSmallerNotNull( Index idx0, Index idx1)
{
	if (idx0 <= idx1)
	{
		if (idx0)
		{
			return idx0;
		}
		else
		{
			return idx1;
		}
	}
	else
	{
		if (idx1)
		{
			return idx1;
		}
		else
		{
			return idx0;
		}
	}
}

Index IteratorUnion::skipDoc( const Index& docno_)
{
	if (m_docno == docno_ && docno_)
	{
		return m_docno;
	}
	m_docno = docno_;
	std::vector<Reference<PostingIteratorInterface> >::iterator
		ai = m_argar.begin(), ae = m_argar.end();
	Index base = docno_?docno_:1;
	Index minimum = 0;

	clearSelected();
	int aidx=0;
	for (; ai != ae; ++ai,++aidx)
	{
		minimum = (*ai)->skipDoc( base);
		if (minimum) break;
	}
	if (!minimum)
	{
		m_docno = 0;
		return 0;
	}
	setSelected( aidx);

	for (aidx++,ai++; ai != ae; ++ai,++aidx)
	{
		Index next = (*ai)->skipDoc( base);
		if (next && next <= minimum)
		{
			if (next < minimum)
			{
				clearSelected();
				minimum = next;
			}
			setSelected( aidx);
		}
	}
	m_docno = minimum;
	return m_docno;
}

Index IteratorUnion::skipPos( const Index& pos_)
{
	selected_iterator si = selected_begin(), se = selected_end();
	Index pos = 0;
	Index basepos = pos_?pos_:1;
	for (; si != se; ++si)
	{
		pos = selectSmallerNotNull( pos, si->skipPos( basepos));
	}
	return m_posno=pos;
}

Index IteratorUnion::documentFrequency() const
{
	if (m_documentFrequency < 0)
	{
		std::vector<Reference<PostingIteratorInterface> >::const_iterator
			ai = m_argar.begin(), ae = m_argar.end();
		if (ai == ae) return 0;

		m_documentFrequency = (*ai)->documentFrequency();
		for (++ai; ai != ae && m_documentFrequency < 0; ++ai)
		{
			Index df = (*ai)->documentFrequency();
			if (df > m_documentFrequency)
			{
				m_documentFrequency = df;
			}
		}
	}
	return m_documentFrequency;
}


