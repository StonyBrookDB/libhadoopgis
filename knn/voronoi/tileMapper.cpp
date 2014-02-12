#include <iostream>
#include <string.h>
#include <string>
#include <boost/foreach.hpp>
#include <boost/geometry.hpp>
#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/domains/gis/io/wkt/wkt.hpp>

#include <boost/algorithm/string/predicate.hpp>

using namespace std;

typedef boost::geometry::model::d2::point_xy<int> point;
typedef boost::geometry::model::polygon<point> polygon;

const string tab = "\t";
const char comma = ',';
const string dash= "-";

const string shapebegin = "POLYGON((";
const string shapeend = "))";

int isBloodVessel()
{
    char * filename = getenv("map_input_file");
    //char * filename = "astroII.1.1.ves";
    if ( NULL == filename ){
	cerr << "map.input.file is NULL." << endl;
	return -1;
    }
    if (boost::algorithm::ends_with(filename,".txt"))
	return 1;
    else 
	return 0;
}

int main(int argc, char **argv) {

    string input_line;
    string image_key ;
    string tile_key ;
    string value;
    polygon poly;
    point p;
    size_t pos=0;
    size_t temp_pos=0;

    int bTag = isBloodVessel();
    if (bTag >0) {
	while(cin && getline(cin, input_line) && !cin.eof()){

	    pos=input_line.find_first_of(comma,0);
	    temp_pos=pos;
	    if (pos == string::npos)
		return 1; // failure

	    image_key = input_line.substr(0,pos);
	    temp_pos   = input_line.find_first_of(comma,pos+1);
	    tile_key  = input_line.substr(pos+1,temp_pos-pos-1);
	    pos	      = temp_pos;
	    value     = input_line.substr(pos+2,input_line.size() - pos - 3); // ignore the double quote both at the beginning and at the end.

	    cout << image_key << dash << tile_key << tab << bTag << tab <<value<<endl;
	    //cout << image_key<< tab << value<< endl;
	}
    } 
    else {
	while(cin && getline(cin, input_line) && !cin.eof()){

	    pos=input_line.find_first_of(comma,0);
	    if (pos == string::npos)
		return 1; // failure

	    image_key = input_line.substr(0,pos);
	    temp_pos   = input_line.find_first_of(comma,pos+1);
	    tile_key  = input_line.substr(pos+1,temp_pos-pos-1);
	    pos = temp_pos;
	    value     = input_line.substr(pos+2,input_line.size() - pos - 3); // ignore the double quote both at the beginning and at the end.
	    boost::geometry::read_wkt(shapebegin+value+shapeend,poly);
	    boost::geometry::centroid(poly, p);
	    cout << image_key << dash << tile_key << tab << bTag << tab << p.x() << tab << p.y()<< endl;
	    //cout << image_key<< tab << value<< endl;
	}

    }
    return 0; // success
}

