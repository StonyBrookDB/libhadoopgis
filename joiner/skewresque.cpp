#include <iostream>
#include <fstream>
#include <cstring>
#include <cmath>
#include <map>
#include <cstdlib> 

// tokenizer 
#include "tokenizer.h"

// geos
#include <geos/geom/PrecisionModel.h>
#include <geos/geom/GeometryFactory.h>
#include <geos/geom/Geometry.h>
#include <geos/geom/Point.h>
#include <geos/io/WKTReader.h>
#include <geos/io/WKTWriter.h>
#include <geos/opBuffer.h>

#include <spatialindex/SpatialIndex.h>

using namespace geos;
using namespace geos::io;
using namespace geos::geom;
using namespace geos::operation::buffer; 


// Constants
const int OSM_SRID = 4326;
const int ST_INTERSECTS = 1;
const int ST_TOUCHES = 2;
const int ST_CROSSES = 3;
const int ST_CONTAINS = 4;
const int ST_ADJACENT = 5;
const int ST_DISJOINT = 6;
const int ST_EQUALS = 7;
const int ST_DWITHIN = 8;
const int ST_WITHIN = 9;
const int ST_OVERLAPS = 10;

const int DATABASE_ID_ONE = 1;
const int DATABASE_ID_TWO = 2;

const string cacheFile= "hgskewinput"; // ctrl+a

// sepertors for parsing
const string tab = "\t";
const string sep = "\x02"; // ctrl+a


// data type declaration 
std::vector<Geometry*> poly_set_one;
std::vector<Geometry*> poly_set_two;
std::vector<string> data1;
std::vector<string> data2;



double expansion_distance = 0.0;
int JOIN_PREDICATE = 0;
int shape_idx_1 = -1;
int shape_idx_2 = -1;


bool readSpatialInputGEOS() 
{
  string input_line;
  vector<string> fields;


  GeometryFactory *gf = new GeometryFactory(new PrecisionModel(),OSM_SRID);
  WKTReader *wkt_reader = new WKTReader(gf);
  Geometry *poly = NULL; 
  int i = 1; 
  while(cin && getline(cin, input_line) && !cin.eof()) {


    tokenize(input_line, fields,tab);
    poly = wkt_reader->read(fields[shape_idx_1]);

    poly_set_two.push_back(poly);
    data2.push_back(input_line);

    fields.clear();
  }

  // parse the second file from Distributed Cache

  std::ifstream skewFile(cacheFile);
  while (std::getline(skewFile, input_line))
  {
    tokenize(input_line, fields,tab);
    poly = wkt_reader->read(fields[shape_idx_2]);

    poly_set_one.push_back(poly);
    data1.push_back(input_line);

    fields.clear();
  }
  skewFile.close();

  return true; //data1.size() >0 && data2.size()>0 ;
}


bool join_with_predicate(const Geometry * geom1 , const Geometry * geom2, const int jp){
  bool flag = false ; 
  const Envelope * env1 = geom1->getEnvelopeInternal();
  const Envelope * env2 = geom2->getEnvelopeInternal();
  BufferOp * buffer_op1 = NULL ;
  // BufferOp * buffer_op2 = NULL ;
  Geometry* geom_buffer1 = NULL;
  // Geometry* geom_buffer2 = NULL;

  switch (jp){

    case ST_INTERSECTS:
      flag = env1->intersects(env2) && geom1->intersects(geom2);
      break;

    case ST_TOUCHES:
      flag = geom1->touches(geom2);
      break;

    case ST_CROSSES:
      flag = geom1->crosses(geom2);
      break;

    case ST_CONTAINS:
      flag = env1->contains(env2) && geom1->contains(geom2);
      break;

    case ST_ADJACENT:
      flag = ! geom1->disjoint(geom2);
      break;

    case ST_DISJOINT:
      flag = geom1->disjoint(geom2);
      break;

    case ST_EQUALS:
      flag = env1->equals(env2) && geom1->equals(geom2);
      break;

    case ST_DWITHIN:
      buffer_op1 = new BufferOp(geom1);
      //buffer_op2 = new BufferOp(geom2);

      geom_buffer1 = buffer_op1->getResultGeometry(expansion_distance);
      // geom_buffer2 = buffer_op2->getResultGeometry(expansion_distance);

      flag = join_with_predicate(geom_buffer1,geom2, ST_INTERSECTS);
      break;

    case ST_WITHIN:
      flag = geom1->within(geom2);
      break; 

    case ST_OVERLAPS:
      flag = geom1->overlaps(geom2);
      break;

    default:
      std::cerr << "ERROR: unknown spatial predicate " << endl;
      break;
  }
  return flag; 
}

int indexedJoin() { return -1; }

int join() 
{
  // cerr << "---------------------------------------------------" << endl;
  int pairs = 0; 

  // for each tile (key) in the input stream 
  try { 

    int len1 = poly_set_one.size();
    int len2 = poly_set_two.size();

    // cerr << "len1 = " << len1 << endl;
    // cerr << "len2 = " << len2 << endl;

    // should use iterator, update later
    for (int i = 0; i < len1 ; i++) {
      const Geometry* geom1 = poly_set_one[i];

      for (int j = 0; j < len2 ; j++) {
        const Geometry* geom2 = poly_set_two[j];

        // data[key][object_id] = input_line;
        if (join_with_predicate(geom1, geom2, JOIN_PREDICATE))  {
          cout << data1[i] << sep << data2[j] << endl; 
          pairs++;
        }

      } // end of for (int j = 0; j < len2 ; j++) 
    } // end of for (int i = 0; i < len1 ; i++) 	
  } // end of try
  catch (Tools::Exception& e) {
    std::cerr << "******ERROR******" << std::endl;
    std::string s = e.what();
    std::cerr << s << std::endl;
    return -1;
  } // end of catch
  return pairs ;
}

bool cleanup(){ return true; }

bool extractParams(int argc, char** argv ){ 
  /*  if (argc < 4) {
      cerr << "usage: resque [predicate] [shape_idx 1] [shape_idx 2] " <<endl;
      return 1;
      } 

      cerr << "argv[1] = " << argv[1] << endl;
      cerr << "argv[2] = " << argv[2] << endl;
      cerr << "argv[3] = " << argv[3] << endl;
      */
  char *predicate_str = NULL;
  char *distance_str = NULL;
  // get param from environment variables 
  if (argc < 2) {
    if (std::getenv("stpredicate") && std::getenv("shapeidx1") && std::getenv("shapeidx1")) {
      predicate_str = std::getenv("stpredicate");
      shape_idx_1 = strtol(std::getenv("shapeidx1"), NULL, 10) - 1;
      shape_idx_2 = strtol(std::getenv("shapeidx2"), NULL, 10) - 1;
      distance_str = std::getenv("stexpdist");
    } else {
      std::cerr << "ERROR: query parameters are not set in environment variables." << endl;
      return false;
    }
  } 
  // get param from command line arguments
  else if (argc >= 4){
    predicate_str = argv[1];
    if (argc >4)
      distance_str = argv[4];

    shape_idx_1 = strtol(argv[2], NULL, 10) - 1;
    shape_idx_2 = strtol(argv[3], NULL, 10) - 1;
  }
  else {
    return false;
  }

  // std::cerr << "SHAPE INDEX: ["<< shape_idx_1 << "]"<<std::endl; 

  if (strcmp(predicate_str, "st_intersects") == 0) {
    JOIN_PREDICATE = ST_INTERSECTS;
  } 
  else if (strcmp(predicate_str, "st_touches") == 0) {
    JOIN_PREDICATE = ST_TOUCHES;
  } 
  else if (strcmp(predicate_str, "st_crosses") == 0) {
    JOIN_PREDICATE = ST_CROSSES;
  } 
  else if (strcmp(predicate_str, "st_contains") == 0) {
    JOIN_PREDICATE = ST_CONTAINS;
  } 
  else if (strcmp(predicate_str, "st_adjacent") == 0) {
    JOIN_PREDICATE = ST_ADJACENT;
  } 
  else if (strcmp(predicate_str, "st_disjoint") == 0) {
    JOIN_PREDICATE = ST_DISJOINT;
  }
  else if (strcmp(predicate_str, "st_equals") == 0) {
    JOIN_PREDICATE = ST_EQUALS;
  }
  else if (strcmp(predicate_str, "st_dwithin") == 0) {
    JOIN_PREDICATE = ST_DWITHIN;
    if (NULL != distance_str)
      expansion_distance = atof(distance_str);
    else 
      std::cerr << "ERROR: expansion distance is not set." << std::endl;
    return false;
  }
  else if (strcmp(predicate_str, "st_within") == 0) {
    JOIN_PREDICATE = ST_WITHIN;
  }
  else if (strcmp(predicate_str, "st_overlaps") == 0) {
    JOIN_PREDICATE = ST_OVERLAPS;
  }
  else {
    std::cerr << "unrecognized join predicate " << endl;
    return false ;
  }
  return true;
}

// main body of the engine
int main(int argc, char** argv)
{
  if (!extractParams(argc,argv)) {
    std::cerr <<"ERROR: query parameter extraction error." << std::endl << "Please see documentations, or contact author." << std::endl;
    return 1;
  }

  // std::cerr << "extractParams is fine. " << std::endl; 

  if (!readSpatialInputGEOS()) {
    std::cerr <<"ERROR: input data parsing error." << std::endl << "Please see documentations, or contact author." << std::endl;
    return 1;
  }

  // std::cerr << "readSpatialInputGEOS is fine. " << std::endl; 

  int c = join();
  if (c==0) 
    std::cout << std::endl;
  std::cerr << "Number of tweets in retail stores " << c <<std::endl; 
  return 0;
}

