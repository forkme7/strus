TERM comma    ",":punctuation;
TERM sentence ".":punctuation;
TERM title     "":title;

# set selector operators: 
# (S 2) two element sequence (from the query)
# (P 2) two element permutation (from the query)
# (A 2) two element permutation with ascending order of elements (from the query)
# (D) scalar product with only distinct elements in the result (sequences with duplicates are not appearing in the result)
# no operator at start of [..] expression forms a scalar product of the elements within

# [0a] any basic term at the start of the title
INTO title FOREACH [title basic] DO intersect(succ(_1), _2);

# [0b] any basic term in the title
INTO title FOREACH [title basic] DO within_struct[-50]( _1, succ(_1), _2);

# [1a2] any sequence of two terms forming also a sequence in the query
INTO struct FOREACH [(S 2) basic] DO sequence[2] (_1, _2);
# [1a3] any sequence of three terms forming also a sequence in the query
INTO struct FOREACH [(S 3) basic] DO sequence[5] (_1, _2, _3);
# [1a4] any sequence of four terms forming also a sequence in the query
INTO struct FOREACH [(S 4) basic] DO sequence[8] (_1, _2, _3, _4);

# [1b] same as [1a2] but at the begin of a sentence
INTO struct FOREACH [phrase [(S 2) basic]] DO intersect( succ(_1), sequence[2](_2, _3));

# [1c] same as [1a2] but at the begin of the title
INTO title FOREACH [title [(S 2) basic]] DO intersect( succ(_1), sequence[2](_2, _3));

# [1d] same as [1a2] but at the somewhere in the title
INTO title FOREACH [title [(S 2) basic]] DO within_struct[-50]( _1, succ(_1), sequence[2] (_2, _3));

# [2a] any sentence containing in any order two terms forming a sequence in the query
INTO struct FOREACH [phrase [(S 2) basic]] DO within_struct[-50] (_1, _2, _3);

# [2b] same as [2a] but one term of the two at the begin of a sentence
INTO struct FOREACH [phrase [(S 2) basic]] DO intersect( succ(_1), within_struct[-50] (_1, _2, _3));

# [3] any sentence containing any two query terms and another different term in the same paragraph
INTO struct FOREACH [phrase [(D) basic [(A 2) basic]]] DO within_struct[-50] (_1, _2, _3);

# [4a] any sentence containing three terms
INTO struct FOREACH [phrase [(S 3) basic]] DO within_struct[-50] (_1, _2, _3, _4);

# [4b] same as [4a] but one term of the three at the begin of a sentence
INTO struct FOREACH [phrase [(S 2) basic]] DO intersect( succ(_1), within_struct[-50] (_1, _2, _3));

# [4c] title containing three terms
INTO struct FOREACH [title [(S 3) basic]] DO within_struct[-50] (_1, _2, _3, _4);

# [4d] same as [4c] but one term of the three at the begin of the title
INTO title FOREACH [title [(S 2) basic]] DO intersect( succ(_1), within_struct[-50] (_1, _2, _3));

EVAL bm15<title,1.5> * 0.3,
	bm25<struct,0.75,2.1,10000>,
	bm25<stem,0.75,2.1,10000> WITH struct,stem;

SUMMARIZE docid = metadata[D];
SUMMARIZE content = matchphrase[orig] FROM struct,stem;


