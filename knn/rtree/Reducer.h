#include <math.h>
#include <time.h>
#include <iostream>
#include <vector>
#include <map>
#include <list>
#include <string>
#include <sstream>
#include <spatialindex/SpatialIndex.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "IndexParam.h"

using namespace SpatialIndex;
using namespace std;
const string tab = "\t";
const char comma = ',';



// example of a Visitor pattern.
// findes the index and leaf IO for answering the query and prints
// the resulting data IDs to stdout.
class MyVisitor : public IVisitor
{
    public:
	size_t m_indexIO;
	size_t m_leafIO;

    private: 
	int m_count;

    public:
	MyVisitor() : m_indexIO(0), m_leafIO(0) {m_count=0;}

	void visitNode(const INode& n)
	{   
	    if (n.isLeaf()) m_leafIO++;
	    else m_indexIO++;
	}   
	void visitData(std::vector<uint32_t>& v) {}
	void visitData(std::string& s) {}

	void visitData(const IData& d)
	{   
	    IShape* ps;
	    d.getShape(&ps);
	    Region* pr = dynamic_cast<Region*>(ps);

	    cout<<d.getIdentifier() << endl;
	    // the ID of this data entry is an answer to the query. I will just print it to stdout.
	}

	void visitData(std::vector<const IData*>& v)
	{   
	}
};


