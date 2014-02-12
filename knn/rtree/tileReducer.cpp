#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <cmath>
#include "Reducer.h"

const char offset = '0';


map<string,id_type> indexIdentifier;
map<string,IStorageManager *> storages ;// StorageManager::createNewMemoryStorageManager();
map<string,ISpatialIndex *> indexes ; //= RTree::createNewRTree(*storage, FillFactor, IndexCapacity, LeafCapacity, 2, SpatialIndex::RTree::RV_RSTAR, indexIdentifier); 
id_type id=0;

map<string,vector<Point> > cells;

bool readSpatialInput();

bool readSpatialInput() {
    string input_line;
    int index =-1;
    string key ;
    string value;
    string temp;
    double x,y ;
    double p[2];
    while(cin && getline(cin, input_line) && !cin.eof()) {
	size_t pos = input_line.find_first_of(tab);
	key = input_line.substr(0,pos);
	index = input_line[pos+1] - offset;
	pos = input_line.find_first_of(tab, pos+1);
	value = input_line.substr(pos+1); // ignore the double quote both at the beginning and at the end.

	std::istringstream iss(value);

	if (index> 0) // blood vessels
	{
	    if (storages.count(key) == 0) {
		storages[key] =  StorageManager::createNewMemoryStorageManager();
		indexes[key] = RTree::createNewRTree(*(storages[key]), FillFactor, IndexCapacity, LeafCapacity, 2, SpatialIndex::RTree::RV_RSTAR, indexIdentifier[key]); 
	    }
	    while (!iss.eof()){
		iss>>x;
		iss>>y;
		if (!iss.eof())
		    iss>>temp;
		// cerr << x << tab << y << tab << temp <<endl;
		p[0] = x; 
		p[1] = y;
		id++;
		Region r = Region(p, p, 2);
		indexes[key]->insertData(0,0, r, id);
	    }
	}
	else { 
	    //  cell centroids 
	    iss>>x;
	    iss>>y;
	    p[0] = x; 
	    p[1] = y;
	    cells[key].push_back(Point(p,2));
	}

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
    int k =1;
    vector<Point>::iterator iter;
    map<string,vector<Point> >::iterator it;
    double squared_dist ;

    if (readSpatialInput())
    {
	// for each image in the input stream 
	string key ;
	for (it= cells.begin(); it != cells.end(); it++)
	{
	    MyVisitor vis;
	    key = it->first;
	    
	    cerr<< "tile ["<<key<<"] is crunching."<< endl;
	    if(indexes.count(key) == 0){
		cerr<< "tile ["<<key<<"] is empty."<< endl;
		continue ;
	    }
	    
	    // for each cell in the input stream 
	    for (iter= cells[key].begin(); iter != cells[key].end(); iter++ )
	    {
		cout << key << tab ;
		Point p =*iter;
		indexes[key]->nearestNeighborQuery(k, p, vis);
	    }

	    cerr<< "tile ["<<key<<"] is done."<< endl;
	}
	//cout<< "okay" <<endl;
	//cout << "Cell size: " << cells.size() <<endl;
    }
    return 0;
}

