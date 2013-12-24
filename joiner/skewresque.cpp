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

const int SID_1 = 1;
const int SID_2 = 2;

// sepertors for parsing
const string tab = "\t";
const string sep = "\x02"; // ctrl+a

const string cacheFile= "hgskewinput"; // default hdfs cache file name 
const int OBJECT_LIMIT= 5000;

// data type declaration 
map<int, std::vector<Geometry*> > polydata;
map<int, std::vector<string> > rawdata;

struct query_op { 
  int JOIN_PREDICATE;
  int shape_idx_1;
  int shape_idx_2;
  int join_cardinality;
  double expansion_distance;
} stop; // st opeartor 

void init();
void print_stop();
int joinBucket();
int mJoinQuery(); 
void releaseShapeMem(const int k);
bool extractParams(int argc, char** argv );
void ReportResult( int i , int j);
string project( vector<string> & fields, int sid);

void init(){
  // initlize query operator 
  stop.expansion_distance = 0.0;
  stop.JOIN_PREDICATE = 0;
  stop.shape_idx_1 = 0;
  stop.shape_idx_2 = 0 ;
  stop.join_cardinality = 0;
}

void print_stop(){
  // initlize query operator 
  std::cerr << "predicate: " << stop.JOIN_PREDICATE << std::endl;
  std::cerr << "distance: " << stop.expansion_distance << std::endl;
  std::cerr << "shape index 1: " << stop.shape_idx_1 << std::endl;
  std::cerr << "shape index 2: " << stop.shape_idx_2 << std::endl;
  std::cerr << "join cardinality: " << stop.join_cardinality << std::endl;
}

int mJoinQuery()
{
  string input_line;
  vector<string> fields;
  PrecisionModel *pm = new PrecisionModel();
  GeometryFactory *gf = new GeometryFactory(pm,OSM_SRID);
  WKTReader *wkt_reader = new WKTReader(gf);
  Geometry *poly = NULL;
  Geometry *buffer_poly = NULL;

  int object_counter = 0;
  // parse the cache file from Distributed Cache
  std::ifstream skewFile(cacheFile);
  while (std::getline(skewFile, input_line))
  {
    tokenize(input_line, fields, tab, true);
    if (fields[stop.shape_idx_2].size() < 4) // this number 4 is really arbitrary
      continue ; // empty spatial object 

    try { 
      poly = wkt_reader->read(fields[stop.shape_idx_2]);
    }
    catch (...) {
      std::cerr << "******Geometry Parsing Error******" << std::endl;
      return -1;
    }

    if (ST_DWITHIN == stop.JOIN_PREDICATE && stop.expansion_distance != 0.0 )
    {
      // cerr << "BestBuy: " << ++object_counter << endl; 
      buffer_poly = poly ;
      poly = BufferOp::bufferOp(buffer_poly,stop.expansion_distance,0,BufferParameters::CAP_SQUARE);
      delete buffer_poly ;
    }

    polydata[SID_2].push_back(poly);
    //rawdata[SID_2].push_back(input_line);
    
    rawdata[SID_2].push_back(project(fields,SID_2)); //--- a better engine implements projection 

    fields.clear();
  }
  skewFile.close();
  // update predicate to st_intersects
  if (ST_DWITHIN == stop.JOIN_PREDICATE )
    stop.JOIN_PREDICATE = ST_INTERSECTS ;

  // parse the main dataset 
  object_counter = 0;
  while(cin && getline(cin, input_line) && !cin.eof()) {
    tokenize(input_line, fields, tab, true);
    //cerr << "Shape size: " << fields[stop.shape_idx_1].size()<< endl;
    if (fields[stop.shape_idx_1].size() < 4) // this number 4 is really arbitrary
      continue ; // empty spatial object 

    try { 
     // cerr << "Tweet: " << (object_counter+1) << endl; 
     // cerr.flush();
      poly = wkt_reader->read(fields[stop.shape_idx_1]);
    }
    catch (...) {
      std::cerr << "******Geometry Parsing Error******" << std::endl;
      return -1;
    }

    if (++object_counter % OBJECT_LIMIT == 0 ) {
      int  pairs = joinBucket();
      std::cerr <<rawdata[SID_1].size() << "|x|" << rawdata[SID_2].size() << "|=|" << pairs << "|" <<std::endl;
      releaseShapeMem(1);
    }

    // populate the bucket for join 
    polydata[SID_1].push_back(poly);
    //rawdata[SID_1].push_back(input_line);
    
    rawdata[SID_1].push_back(project(fields,SID_1)); //--- a better engine implements projection 

    fields.clear();
  }

  // last batch 
  int  pairs = joinBucket();
  std::cerr <<rawdata[SID_1].size() << "|x|" << rawdata[SID_2].size() << "|=|" << pairs << "|" <<std::endl;
  releaseShapeMem(stop.join_cardinality);

  // clean up newed objects
  delete wkt_reader ;
  delete gf ;
  delete pm ;

  return object_counter;
}

void releaseShapeMem(const int k ){
  if (k <=0)
    return ;
  for (int j =0 ; j <k ;j++ )
  {
    int delete_index = j+1 ;
    int len = polydata[delete_index].size();

    for (int i = 0; i < len ; i++) 
      delete polydata[delete_index][i];

    polydata[delete_index].clear();
    rawdata[delete_index].clear();
  }
}

bool join_with_predicate(const Geometry * geom1 , const Geometry * geom2, const int jp){
  bool flag = false ; 
  const Envelope * env1 = geom1->getEnvelopeInternal();
  const Envelope * env2 = geom2->getEnvelopeInternal();
  BufferOp * buffer_op1 = NULL ;
  BufferOp * buffer_op2 = NULL ;
  Geometry* geom_buffer1 = NULL;
  Geometry* geom_buffer2 = NULL;

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
      // buffer_op2 = new BufferOp(geom2);
      if (NULL == buffer_op1)
        cerr << "NULL: buffer_op1" <<endl;

      geom_buffer1 = buffer_op1->getResultGeometry(stop.expansion_distance);
      // geom_buffer2 = buffer_op2->getResultGeometry(expansion_distance);
      //Envelope * env_temp = geom_buffer1->getEnvelopeInternal();
      if (NULL == geom_buffer1)
        cerr << "NULL: geom_buffer1" <<endl;

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

// only print our ids for now 
string project( vector<string> & fields, int sid) {
  int idx = -1; 
  switch (sid){
    case 1:
      idx = 0;
      break;
    case 2:
      idx = 5;
      break;
    default:
      idx = 0 ;
  }
  return fields[idx];
}

void ReportResult( int i , int j)
{
  switch (stop.join_cardinality){
    case 1:
      cout << rawdata[SID_1][i] << sep << rawdata[SID_1][j] << endl;
      break;
    case 2:
      cout << rawdata[SID_1][i] << sep << rawdata[SID_2][j] << endl; 
      break;
    default:
      return ;
  }
}

int joinBucket() 
{
  // cerr << "---------------------------------------------------" << endl;
  int pairs = 0;
  bool selfjoin = stop.join_cardinality ==1 ? true : false ;
  int idx1 = SID_1 ; 
  int idx2 = selfjoin ? SID_1 : SID_2 ;

  // for each tile (key) in the input stream 
  try { 

    std::vector<Geometry*>  & poly_set_one = polydata[idx1];
    std::vector<Geometry*>  & poly_set_two = polydata[idx2];

    int len1 = poly_set_one.size();
    int len2 = poly_set_two.size();

    // cerr << "len1 = " << len1 << endl;
    // cerr << "len2 = " << len2 << endl;

    // should use iterator, update later
    for (int i = 0; i < len1 ; i++) {
      const Geometry* geom1 = poly_set_one[i];

      for (int j = 0; j < len2 ; j++) {
        if (i == j && selfjoin) 
          continue ; 

        const Geometry* geom2 = poly_set_two[j];
        if (join_with_predicate(geom1, geom2, stop.JOIN_PREDICATE))  {
          ReportResult(i,j);
          pairs++;
        }

      } // end of for (int j = 0; j < len2 ; j++) 
    } // end of for (int i = 0; i < len1 ; i++) 	
  } // end of try
  //catch (Tools::Exception& e) {
  catch (...) {
    std::cerr << "******ERROR******" << std::endl;
    //std::string s = e.what();
    //std::cerr << s << std::endl;
    return -1;
  } // end of catch
  return pairs ;
}

bool extractParams(int argc, char** argv ){ 
  /*
     std::cerr <<  "argc: " << argc << std::endl;
     cerr << "argv[1] = " << argv[1] << endl;
     cerr << "argv[2] = " << argv[2] << endl;
     cerr << "argv[3] = " << argv[3] << endl;
     */
  char *predicate_str = NULL;
  char *distance_str = NULL;

  switch (argc) {
    // get param from environment variables 
    case 1:
      // discouraged to use env vars. Many instance of resuqe may run in the same system. 
      if (std::getenv("stpredicate") && std::getenv("shapeidx1") && std::getenv("shapeidx2")) {
        predicate_str = std::getenv("stpredicate");
        stop.shape_idx_1 = strtol(std::getenv("shapeidx1"), NULL, 10) - 1;
        stop.shape_idx_2 = strtol(std::getenv("shapeidx2"), NULL, 10) - 1;
        distance_str = std::getenv("stexpdist");
      } else {
        std::cerr << "ERROR: query parameters are not set." << std::endl;
        return false;
      }
      break;

      // predicate only queries
    case 2:
      std::cerr << "ERROR: predicate only queries are not supported yet." << std::endl;
      return false;
      break;

      // single argument predicates -- self join
    case 3:
      predicate_str = argv[1];
      stop.shape_idx_1 = strtol(argv[2], NULL, 10) - 1;
      stop.join_cardinality +=1 ; 
      break;

      // two argument predicates
    case 4: 
      predicate_str = argv[1];
      stop.shape_idx_1 = strtol(argv[2], NULL, 10) - 1;
      stop.join_cardinality +=1 ; 
      if (strcmp(predicate_str, "st_dwithin") == 0) {
        distance_str = argv[3];
      }
      else {
        stop.shape_idx_2 = strtol(argv[3], NULL, 10) - 1;
        stop.join_cardinality +=1 ; 
      }
      break;
      // std::cerr << "Params: [" << predicate_str << "] [" << shape_idx_1 << "] " << shape_idx_2 << "]" << std::endl;  

    case 5: 
      predicate_str = argv[1];
      stop.shape_idx_1 = strtol(argv[2], NULL, 10) - 1;
      stop.join_cardinality +=1 ; 
      stop.shape_idx_2 = strtol(argv[3], NULL, 10) - 1;
      stop.join_cardinality +=1 ; 
      distance_str = argv[4];
      // std::cerr << "Params: [" << predicate_str << "] [" << shape_idx_1 << "] " << shape_idx_2 << "]" << std::endl;  

      break;
    default:
      return false;
  }

  if (strcmp(predicate_str, "st_intersects") == 0) {
    stop.JOIN_PREDICATE = ST_INTERSECTS;
  } 
  else if (strcmp(predicate_str, "st_touches") == 0) {
    stop.JOIN_PREDICATE = ST_TOUCHES;
  } 
  else if (strcmp(predicate_str, "st_crosses") == 0) {
    stop.JOIN_PREDICATE = ST_CROSSES;
  } 
  else if (strcmp(predicate_str, "st_contains") == 0) {
    stop.JOIN_PREDICATE = ST_CONTAINS;
  } 
  else if (strcmp(predicate_str, "st_adjacent") == 0) {
    stop.JOIN_PREDICATE = ST_ADJACENT;
  } 
  else if (strcmp(predicate_str, "st_disjoint") == 0) {
    stop.JOIN_PREDICATE = ST_DISJOINT;
  }
  else if (strcmp(predicate_str, "st_equals") == 0) {
    stop.JOIN_PREDICATE = ST_EQUALS;
  }
  else if (strcmp(predicate_str, "st_dwithin") == 0) {
    stop.JOIN_PREDICATE = ST_DWITHIN;
    if (NULL != distance_str)
      stop.expansion_distance = atof(distance_str);
    else {
      std::cerr << "ERROR: expansion distance is not set." << std::endl;
      return false;
    }
  }
  else if (strcmp(predicate_str, "st_within") == 0) {
    stop.JOIN_PREDICATE = ST_WITHIN;
  }
  else if (strcmp(predicate_str, "st_overlaps") == 0) {
    stop.JOIN_PREDICATE = ST_OVERLAPS;
  }
  else {
    std::cerr << "unrecognized join predicate " << std::endl;
    return false;
  }

  print_stop();

  return true;
}

// main body of the engine
int main(int argc, char** argv)
{
  /*  if (argc < 4) {
      cerr << "usage: resque [predicate] [shape_idx 1] [shape_idx 2] [distance]" <<endl;
      return 1;
      } */

  // initilize query operator 
  init();

  int c = 0 ;
  if (!extractParams(argc,argv)) {
    std::cerr <<"ERROR: query parameter extraction error." << std::endl << "Please see documentations, or contact author." << std::endl;
    return 1;
  }

  switch (stop.join_cardinality){
    case 1:
    case 2:
      c = mJoinQuery();
      // std::cerr <<"ERROR: input data parsing error." << std::endl << "Please see documentations, or contact author." << std::endl;
      break;

    default:
      std::cerr <<"ERROR: join cardinality does not match engine capacity." << std::endl ;
      return 1;
      break;
  }
  if (c >= 0 )
    std::cerr <<"Query Load: [" << c << "]" <<std::endl;
  else 
  {
    std::cerr <<"Error: ill formatted data. Terminating ....... " << std::endl;
    return 1;
  }

  cout.flush();
  cerr.flush();

  return 0;
}

