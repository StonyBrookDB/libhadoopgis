#include "hadoopgis.h"
#include "cmdline.h"


GeometryFactory *gf = NULL;
WKTReader *wkt_reader = NULL;
IStorageManager * storage = NULL;
ISpatialIndex * spidx = NULL;
map<int,string> id_polygon ;
vector<int> hits ; 
char * prefix;

int GEOM_IDX = -1;
int ID_IDX = -1;

bool assignIndex(int uid_idx, int geom_idx) {
    if (uid_idx <1 || geom_idx <1 )
	return false;
    GEOM_IDX = geom_idx -1;
    ID_IDX = uid_idx -1;
    return true;
}

RTree::Data* parseInputPolygon(Geometry *p, id_type m_id) {
    double low[2], high[2];
    const Envelope * env = p->getEnvelopeInternal();

    low [0] = env->getMinX();
    low [1] = env->getMinY();

    high [0] = env->getMaxX();
    high [1] = env->getMaxY();

    Region r(low, high, 2);

    return new RTree::Data(0, 0 , r, m_id);// store a zero size null poiter.
}


class GEOSDataStream : public IDataStream
{
    public:
	GEOSDataStream(map<int,Geometry*> * inputColl ) : m_pNext(0), len(0),m_id(0)
    {
	if (inputColl->empty())
	    throw Tools::IllegalArgumentException("Input size is ZERO.");
	shapes = inputColl;
	len = inputColl->size();
	iter = shapes->begin();
	readNextEntry();
    }
	virtual ~GEOSDataStream()
	{
	    if (m_pNext != 0) delete m_pNext;
	}

	virtual IData* getNext()
	{
	    if (m_pNext == 0) return 0;

	    RTree::Data* ret = m_pNext;
	    m_pNext = 0;
	    readNextEntry();
	    return ret;
	}

	virtual bool hasNext()
	{
	    return (m_pNext != 0);
	}

	virtual uint32_t size()
	{
	    return len;
	    //throw Tools::NotSupportedException("Operation not supported.");
	}

	virtual void rewind()
	{
	    if (m_pNext != 0)
	    {
		delete m_pNext;
		m_pNext = 0;
	    }

	    m_id  = 0;
	    iter = shapes->begin();
	    readNextEntry();
	}

	void readNextEntry()
	{
	    if (iter != shapes->end())
	    {
		//std::cerr<< "readNextEntry m_id == " << m_id << std::endl;
		m_id = iter->first;
		m_pNext = parseInputPolygon(iter->second, m_id);
		iter++;
	    }
	}

	RTree::Data* m_pNext;
	map<int,Geometry*> * shapes; 
	map<int,Geometry*>::iterator iter; 

	int len;
	id_type m_id;
};


class MyVisitor : public IVisitor
{
    public:
	void visitNode(const INode& n) {}
	void visitData(std::string &s) {}

	void visitData(const IData& d)
	{
	    hits.push_back(d.getIdentifier());
	    //std::cout << d.getIdentifier()<< std::endl;
	}

	void visitData(std::vector<const IData*>& v) {}
	void visitData(std::vector<uint32_t>& v){}
};



void doQuery(Geometry* poly) {
    double low[2], high[2];
    const Envelope * env = poly->getEnvelopeInternal();

    low [0] = env->getMinX();
    low [1] = env->getMinY();

    high [0] = env->getMaxX();
    high [1] = env->getMaxY();

    Region r(low, high, 2);

    // clear the result container 
    hits.clear();

    MyVisitor vis ; 
    //spidx->containsWhatQuery(r, vis);
    spidx->intersectsWithQuery(r, vis);

}


vector<Geometry*> genTiles(double min_x, double max_x, double min_y, double  max_y, int x_split, int y_split) {
    vector<Geometry*> tiles;
    stringstream ss;
    double width =  (max_x - min_x) / x_split;
    //cerr << "Tile width" << SPACE <<width <<endl;
    double height = (max_y - min_y)/y_split ;
    //cerr << "Tile height" << SPACE << height <<endl;


    for (int i =0 ; i< x_split ; i++)
    {
	for (int j =0 ; j< y_split ; j++)
	{
	    // construct a WKT polygon 
	    ss << shapebegin ;
	    ss << min_x + i * width ;     ss << SPACE ; ss << min_y + j * height;     ss << COMMA;
	    ss << min_x + i * width ;     ss << SPACE ; ss << min_y + (j+1) * height; ss << COMMA;
	    ss << min_x + (i+1) * width ; ss << SPACE ; ss << min_y + (j+1) * height; ss << COMMA;
	    ss << min_x + (i+1) * width ; ss << SPACE ; ss << min_y + j * height;     ss << COMMA;
	    ss << min_x + i * width ;     ss << SPACE ; ss << min_y + j * height;
	    ss << shapeend ;
	    //cerr << ss.str() << endl;
	    tiles.push_back(wkt_reader->read(ss.str()));
	    ss.str(string()); // clear the content
	}
    }

    return tiles;
}

vector<string> parse(string & line) {
    vector<string> tokens ;
    // if we have boost then 
    tokenize(line, tokens,TAB,true);
    return tokens;
}

void freeObjects() {
    // garbage collection 
    delete wkt_reader ;
    delete gf ; 
    delete spidx;
    delete storage;
}

void emitHits(Geometry* poly) {
    double low[2], high[2];
    const Envelope * env = poly->getEnvelopeInternal();

    low [0] = env->getMinX();
    low [1] = env->getMinY();

    high [0] = env->getMaxX();
    high [1] = env->getMaxY();

    stringstream ss; // tile_id ; 
    if (NULL != prefix) ss << prefix << DASH;
    ss << low[0] ;
    ss << DASH ;
    ss << low[1] ;
    ss << DASH ;
    ss << high[0] ;
    ss << DASH ;
    ss<< high[1] ;

    for (int i = 0 ; i < hits.size(); i++ ) 
    {
	cout << ss.str() << TAB << id_polygon[hits[i]] << endl ;
    }
}


bool buildIndex(map<int,Geometry*> & geom_polygons) {
    // build spatial index on tile boundaries 
    id_type  indexIdentifier;
    GEOSDataStream stream(&geom_polygons);
    storage = StorageManager::createNewMemoryStorageManager();
    spidx   = RTree::createAndBulkLoadNewRTree(RTree::BLM_STR, stream, *storage, 
	    FillFactor,
	    IndexCapacity,
	    LeafCapacity,
	    2, 
	    RTree::RV_RSTAR, indexIdentifier);

    // Error checking 
    return spidx->isIndexValid();
}

int main(int argc, char **argv) {
    gengetopt_args_info args_info;
    if (cmdline_parser (argc, argv, &args_info) != 0)
    {
	cerr << "ERROR: command line parsing error." <<endl;
	exit(1) ;
    }

    double min_x = args_info.min_x_arg;
    double max_x = args_info.max_x_arg;
    double min_y = args_info.min_y_arg;
    double max_y = args_info.max_y_arg;
    int x_split = args_info.x_split_arg;
    int y_split = args_info.y_split_arg;
    int uid_idx  = args_info.uid_arg;
    int geom_idx = args_info.geom_arg;

    if (!assignIndex(uid_idx,geom_idx)) 
    {
	cerr << "ERROR: index assigning has failed." <<endl;
	exit(1); 
    }

    /* 
       cerr << "min_x "<< min_x << endl; 
       cerr << "max_x "<< max_x << endl; 
       cerr << "min_y "<< min_y << endl; 
       cerr << "max_y "<< max_y << endl; 
       cerr << "x_split "<< x_split << endl; 
       cerr << "y_split "<< y_split << endl; 
       */

    // initlize the GEOS ibjects
    gf = new GeometryFactory(new PrecisionModel(),0);
    wkt_reader= new WKTReader(gf);


    // process input data 
    map<int,Geometry*> geom_polygons;
    string input_line;
    vector<string> fields;
    cerr << "Reading input from stdin..." <<endl; 
    int i = -1; 
    Geometry* geom ; 

    while(cin && getline(cin, input_line) && !cin.eof()){
	fields = parse(input_line);
	if (fields[ID_IDX].length() <1 )
	    continue ;  // skip lines which has empty id field 
	i = atoi(fields[ID_IDX].c_str()); 

	if (fields[GEOM_IDX].length() <2 )
	{
#ifndef NDEBUG
	    cerr << "skipping record [" << i <<"]"<< endl;
#endif
	    continue ;  // skip lines which has empty geometry
	}
	// try {
	geom = wkt_reader->read(fields[GEOM_IDX]);
	//}
	/*catch (...)
	  {
	  cerr << "WARNING: Record [id = " <<i << "] is not well formatted "<<endl;
	  cerr << input_line << endl;
	  continue ;
	  }*/

	geom_polygons[i]= geom;
	id_polygon[i] = input_line; 
    }
    // build spatial index for input polygons 
    bool ret = buildIndex(geom_polygons);
    if (ret == false) {

	cerr << "ERROR: Index building on tile structure has failed ." << std::endl;
	return 1 ;
    }
    else 
#ifndef NDEBUG  
	cerr << "GRIDIndex Generated successfully." << endl;
#endif

    // genrate tile boundaries 
    vector <Geometry*> geom_tiles= genTiles(min_x, max_x, min_y, max_y,x_split,y_split);
    cerr << "Number of tiles: " << geom_tiles.size() << endl;



    for(std::vector<Geometry*>::iterator it = geom_tiles.begin(); it != geom_tiles.end(); ++it) {
	doQuery(*it);
	emitHits(*it);
    }

    cout.flush();
    cerr.flush();
    cmdline_parser_free (&args_info); /* release allocated memory */
    freeObjects();
    return 0; // success
}

