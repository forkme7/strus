result:
INTO title FOREACH [title, basic] DO intersect( succ( _1), _2);
INTO title FOREACH [title, basic] DO within_struct[-50]( _1, succ( _1), _2);
INTO struct FOREACH [(S 2) basic] DO sequence[2]( _1, _2);
INTO struct FOREACH [(S 3) basic] DO sequence[5]( _1, _2, _3);
INTO struct FOREACH [(S 4) basic] DO sequence[8]( _1, _2, _3, _4);
INTO struct FOREACH [phrase, [(S 2) basic]] DO intersect( succ( _1), sequence[2]( _2, _3));
INTO title FOREACH [title, [(S 2) basic]] DO intersect( succ( _1), sequence[2]( _2, _3));
INTO title FOREACH [title, [(S 2) basic]] DO within_struct[-50]( _1, succ( _1), sequence[2]( _2, _3));
INTO struct FOREACH [phrase, [(S 2) basic]] DO within_struct[-50]( _1, _2, _3);
INTO struct FOREACH [phrase, [(S 2) basic]] DO intersect( succ( _1), within_struct[-50]( _1, _2, _3));
INTO struct FOREACH [phrase, [(D) basic, [(A 2) basic]]] DO within_struct[-50]( _1, _2, _3);
INTO struct FOREACH [phrase, [(S 3) basic]] DO within_struct[-50]( _1, _2, _3, _4);
INTO struct FOREACH [phrase, [(S 2) basic]] DO intersect( succ( _1), within_struct[-50]( _1, _2, _3));
INTO struct FOREACH [title, [(S 3) basic]] DO within_struct[-50]( _1, _2, _3, _4);
INTO title FOREACH [title, [(S 2) basic]] DO intersect( succ( _1), within_struct[-50]( _1, _2, _3));
EVAL idfPriority(bm15<title,1.5> * 0, bm25<struct,0.75,2.1,10000>)

