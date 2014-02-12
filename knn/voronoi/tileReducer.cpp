#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <cmath>

//#include <boost/foreach.hpp>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/point_generators_2.h>
#include <CGAL/Delaunay_triangulation_2.h>

using namespace std;

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;

typedef CGAL::Delaunay_triangulation_2<K>  Triangulation;
typedef Triangulation::Edge_iterator  Edge_iterator;
typedef Triangulation::Point          Point;
typedef CGAL::Creator_uniform_2<double,Point>            Creator;

const char offset = '0';
const string tab = "\t";
const char comma = ',';

const string shapebegin = "POLYGON((";
const string shapeend = "))";

const int million= 1000000;

//Triangulation T;
map<string,Triangulation> vd;
map<string,vector<Point> > cells;

bool readSpatialInput();

bool readSpatialInput() {
    string input_line;
    int index =-1;
    string key ;
    string value;
    string temp;
    double x,y ;
    while(cin && getline(cin, input_line) && !cin.eof()) {
	size_t pos = input_line.find_first_of(tab);
	key = input_line.substr(0,pos);
	index = input_line[pos+1] - offset;
	pos = input_line.find_first_of(tab, pos+1);
	value = input_line.substr(pos+1); // ignore the double quote both at the beginning and at the end.

	std::istringstream iss(value);

	if (index == 1) // blood vessels
	{
	    while (!iss.eof()){
		iss>>x;
		iss>>y;
		if (!iss.eof())
		    iss>>temp;
		vd[key].insert(Point(x,y));
	    }
	}
	else if (index==0){ 
	    //  cell centroids 
	    iss>>x;
	    iss>>y;
	    cells[key].push_back(Point(x,y));
	}
	else continue;

	/*
	   cerr << "key: "   <<  key << endl;
	   cerr << "index: " << index << endl;
	   cerr << "value: " << value << endl;
	   */
    }

    // cerr << "tile size" << "\t markup " << markup.size() << "\t mbb " << outline.size()<< endl;
    return true;
}

int main(int argc, char** argv)
{
    vector<Point>::iterator iter;
    map<string,vector<Point> >::iterator it;
    double squared_dist ;

    if (readSpatialInput())
    {
	// for each image in the input stream 
	string key ;
	for (it= cells.begin(); it != cells.end(); it++)
	{
	    key = it->first;
	    cerr<< "tile ["<<key<<"] is crunching."<< endl;
	    
	    if(vd.count(key) == 0){
		cerr<< "tile ["<<key<<"] is empty."<< endl;
		continue ;
	    }
	    
		// for each cell in the input stream 
	    for (iter= cells[key].begin(); iter != cells[key].end(); iter++ )
	    {
		Point p =vd[key].nearest_vertex(*iter)->point();
		squared_dist = CGAL::to_double(CGAL::squared_distance(p,*iter));
		cout << key << tab <<squared_dist << endl;
	    }

	    cerr<< "tile ["<<key<<"] is done."<< endl;
	}
	//cout<< "okay" <<endl;
	//cout << "Cell size: " << cells.size() <<endl;
    }
    cout.flush();
    cerr.flush();
    return 0;
}

