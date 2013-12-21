#include <iostream>
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

// sepertors for parsing
const string tab = "\t";
const string sep = "\x02"; // ctrl+a

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
int joinBucket();
int mJoinQuery(); 
void releaseShapeMem(const int k);
int extractParams(int argc, char** argv );

void init(){
 // initlize query operator 
  stop.expansion_distance = 0.0;
  stop.JOIN_PREDICATE = 0;
  stop.shape_idx_1 = 0;
  stop.shape_idx_2 = 0 ;
  stop.join_cardinality = 0;
}

int mJoinQuery()
{
  string input_line;
  string tile_id ;
  string value;
  vector<string> fields;
  int database_id = 0;

  GeometryFactory *gf = new GeometryFactory(new PrecisionModel(),OSM_SRID);
  WKTReader *wkt_reader = new WKTReader(gf);
  Geometry *poly = NULL;
  string previd = "";
  
  int tile_counter =0;

  std::cerr <<"Bucketinfo:[ID] |A|x|B|=|R|" <<std::endl;
  
  while(cin && getline(cin, input_line) && !cin.eof()) {

    tokenize(input_line, fields,tab,true);
    database_id = atoi(fields[1].c_str());
    tile_id = fields[0];
    // object_id = fields[2];

    // cerr << "fields[0] = " << fields[0] << endl; 
    // cerr << "fields[1] = " << fields[1] << endl; 
    // cerr << "fields[2] = " << fields[2] << endl; 
    // cerr << "fields[9] = " << fields[9] << endl; 

    switch(database_id){

      case DATABASE_ID_ONE:
        poly = wkt_reader->read(fields[stop.shape_idx_1]);
        break;

      case DATABASE_ID_TWO:
        poly = wkt_reader->read(fields[stop.shape_idx_2]);
        break;

      default:
        std::cerr << "wrong database id : " << database_id << endl;
        return false;
    }
    
    if (previd.compare(tile_id) !=0 && previd.size() > 0 ) {
      int  pairs = joinBucket();
      std::cerr <<"T[" << previd << "] |" << polydata[DATABASE_ID_ONE].size() << "|x|" << polydata[DATABASE_ID_TWO].size() << "|=|" << pairs << "|" <<std::endl;
      tile_counter++; 
      releaseShapeMem(stop.join_cardinality);
    }

    // populate the bucket for join 
    polydata[database_id].push_back(poly);
    rawdata[database_id].push_back(fields[2]);
    previd = tile_id; 

    fields.clear();
  }
  // last tile
  int  pairs = joinBucket();
  std::cerr <<"T[" << previd << "] |" << polydata[DATABASE_ID_ONE].size() << "|x|" << polydata[DATABASE_ID_TWO].size() << "|=|" << pairs << "|" <<std::endl;
  tile_counter++;
  releaseShapeMem(stop.join_cardinality);
  
  return tile_counter;
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

      geom_buffer1 = buffer_op1->getResultGeometry(stop.expansion_distance);
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

int joinBucket() 
{
  // cerr << "---------------------------------------------------" << endl;
  int pairs = 0;
  bool selfjoin = stop.join_cardinality ==1 ? true : false ;
  int idx1 = DATABASE_ID_ONE ; 
  int idx2 = selfjoin ? DATABASE_ID_ONE : DATABASE_ID_TWO ;

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
          cout << rawdata[DATABASE_ID_ONE][i] << tab << rawdata[DATABASE_ID_TWO][j] << endl; 
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

int extractParams(int argc, char** argv ){ 
  /*
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
      if (std::getenv("stpredicate") && std::getenv("shapeidx1") && std::getenv("shapeidx1")) {
        predicate_str = std::getenv("stpredicate");
        stop.shape_idx_1 = strtol(std::getenv("shapeidx1"), NULL, 10) + 2;
        stop.shape_idx_2 = strtol(std::getenv("shapeidx2"), NULL, 10) + 2;
        distance_str = std::getenv("stexpdist");
      } else {
        std::cerr << "ERROR: query parameters are not set in environment variables." << std::endl;
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
      stop.shape_idx_1 = strtol(argv[2], NULL, 10) + 2;
      stop.join_cardinality +=1 ; 
      break;

      // two argument predicates
    case 4: 
      predicate_str = argv[1];
      stop.shape_idx_1 = strtol(argv[2], NULL, 10) + 2;
      stop.join_cardinality +=1 ; 
      if (strcmp(predicate_str, "st_dwithin") == 0) {
        distance_str = argv[3];
      }
      else {
        stop.shape_idx_2 = strtol(argv[3], NULL, 10) + 2;
        stop.join_cardinality +=1 ; 
      }
      break;
      // std::cerr << "Params: [" << predicate_str << "] [" << shape_idx_1 << "] " << shape_idx_2 << "]" << std::endl;  

    case 5: 
      predicate_str = argv[1];
      stop.shape_idx_1 = strtol(argv[2], NULL, 10) + 2;
      stop.join_cardinality +=1 ; 
      stop.shape_idx_2 = strtol(argv[3], NULL, 10) + 2;
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
    else 
      std::cerr << "ERROR: expansion distance is not set." << std::endl;
    return false;
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

  return true;
}

// main body of the engine
int main(int argc, char** argv)
{
  /*  if (argc < 4) {
      cerr << "usage: resque [predicate] [shape_idx 1] [shape_idx 2] [distance]" <<endl;
      return 1;
      } */
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
  std::cerr <<"Query Load: [" << c << "]" <<std::endl;
  
  cout.flush();
  cerr.flush();
  
  return 0;
}

