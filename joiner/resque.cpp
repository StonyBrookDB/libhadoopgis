#include <iostream>
#include <cstring>
#include <cmath>
#include <map>
#include <cstdlib> 
#include <getopt.h>

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

// data type declaration 
map<int, std::vector<Geometry*> > polydata;
map<int, std::vector<string> > rawdata;

struct query_op { 
  int JOIN_PREDICATE;
  int shape_idx_1;
  int shape_idx_2;
  int join_cardinality;
  double expansion_distance;
  vector<int> proj1; /* Output fields for 1st set  */
  vector<int> proj2; /* Output fields for 2nd set */
} stop; // st opeartor 

void init();
void print_stop();
int joinBucket();
int mJoinQuery(); 
void releaseShapeMem(const int k);
int getJoinPredicate(char * predicate_str);
void setProjectionParam(char * arg);
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
  string tile_id ;
  string value;
  vector<string> fields;
  int sid = 0;

  PrecisionModel *pm = new PrecisionModel();
  GeometryFactory *gf = new GeometryFactory(pm,OSM_SRID);
  WKTReader *wkt_reader = new WKTReader(gf);
  Geometry *poly = NULL;
  string previd = "";

  int tile_counter =0;

  std::cerr <<"Bucketinfo:[ID] |A|x|B|=|R|" <<std::endl;
  int index = -1; 
  while(cin && getline(cin, input_line) && !cin.eof()) {
    tokenize(input_line, fields,tab,true);
    sid = atoi(fields[1].c_str());
    tile_id = fields[0];
    // object_id = fields[2];
    // cerr << "fields[0] = " << fields[0] << endl; 
    // cerr << "fields[1] = " << fields[1] << endl; 
    // cerr << "fields[2] = " << fields[2] << endl; 
    // cerr << "fields[9] = " << fields[9] << endl; 
    // std::cerr << "Shape: " << fields[stop.shape_idx_1]<< std::endl;

    switch(sid){
      case SID_1:
        index = stop.shape_idx_1 ; 
        break;
      case SID_2:
        index = stop.shape_idx_2 ; 
        break;
      default:
        std::cerr << "wrong sid : " << sid << endl;
        return false;
    }

    if (fields[index].size() < 4) // this number 4 is really arbitrary
      continue ; // empty spatial object 

    try { 
      poly = wkt_reader->read(fields[index]);
    }
    catch (...) {
      std::cerr << "******Geometry Parsing Error******" << std::endl;
      return -1;
    }

    if (previd.compare(tile_id) !=0 && previd.size() > 0 ) {
      int  pairs = joinBucket();
      std::cerr <<"T[" << previd << "] |" << polydata[SID_1].size() << "|x|" << polydata[SID_2].size() << "|=|" << pairs << "|" <<std::endl;
      tile_counter++; 
      releaseShapeMem(stop.join_cardinality);
    }

    // populate the bucket for join 
    polydata[sid].push_back(poly);
    // rawdata[sid].push_back(project(fields,sid));
    switch(sid){
      case SID_1:
        rawdata[sid].push_back(project(fields,SID_1));
        break;
      case SID_2:
        rawdata[sid].push_back(project(fields,SID_2));
        break;
      default:
        std::cerr << "wrong sid : " << sid << endl;
        return false;
    }
    previd = tile_id; 

    fields.clear();
  }
  // last tile
  int  pairs = joinBucket();
  std::cerr <<"T[" << previd << "] |" << polydata[SID_1].size() << "|x|" << polydata[SID_2].size() << "|=|" << pairs << "|" <<std::endl;
  tile_counter++;
  releaseShapeMem(stop.join_cardinality);
  
  // clean up newed objects
  delete wkt_reader ;
  delete gf ;
  delete pm ;

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

/* Filter selected fields for output
 * If there is no field selected, output all fields (except tileid and joinid) */
string project( vector<string> & fields, int sid) {
  std::stringstream ss;
  switch (sid){
    case 1:
      if (stop.proj1.size() == 0) {
          /* Do not output tileid and joinid */
	  ss << fields[2];
          for (int i = 3 ; i < fields.size(); i++)
          {
             ss << tab << fields[i];
          }
      } else {
          for (int i = 0 ; i <stop.proj1.size();i++)
          {
             if ( 0 == i )
               ss << fields[stop.proj1[i]] ;
             else
             {
                if (stop.proj1[i] < fields.size())
                ss << tab << fields[stop.proj1[i]];
             }
          }
      }
      break;
    case 2:
       if (stop.proj2.size() == 0) {
          /* Do not output tileid and joinid */
	  ss << fields[2];
          for (int i = 3 ; i < fields.size(); i++)
          {
             ss << tab << fields[i];
          }
      } else {
          for (int i = 0 ; i <stop.proj2.size();i++)
          {
             if ( 0 == i )
               ss << fields[stop.proj2[i]] ;
             else
             {
                if (stop.proj2[i] < fields.size())
                ss << tab << fields[stop.proj2[i]];
             }
          }
      }
      break;
    default:
      break;
  }

  return ss.str();
}

/* Set output fields
 * Fields are "technically" off by 3 (2 from extra field 
 * and 1 because of counting from 1 ) 
 */
void setProjectionParam(char * arg)
{
  string param(arg);
  vector<string> fields;
  vector<string> selec;
  tokenize(param, fields,":");

  if (fields.size()>0)
  {
    tokenize(fields[0], selec,",");
    for (int i =0 ;i < selec.size(); i++)
      stop.proj1.push_back(atoi(selec[i].c_str()) + 2);
  }
  selec.clear();

  if (fields.size()>1)
  {
    tokenize(fields[1], selec,",");
    for (int i =0 ;i < selec.size(); i++)
      stop.proj2.push_back(atoi(selec[i].c_str()) + 2);
  }
}

int getJoinPredicate(char * predicate_str)
{
  if (strcmp(predicate_str, "st_intersects") == 0) {
    // stop.JOIN_PREDICATE = ST_INTERSECTS;
    return ST_INTERSECTS ; 
  } 
  else if (strcmp(predicate_str, "st_touches") == 0) {
    return ST_TOUCHES;
  } 
  else if (strcmp(predicate_str, "st_crosses") == 0) {
    return ST_CROSSES;
  } 
  else if (strcmp(predicate_str, "st_contains") == 0) {
    return ST_CONTAINS;
  } 
  else if (strcmp(predicate_str, "st_adjacent") == 0) {
    return ST_ADJACENT;
  } 
  else if (strcmp(predicate_str, "st_disjoint") == 0) {
    return ST_DISJOINT;
  }
  else if (strcmp(predicate_str, "st_equals") == 0) {
    return ST_EQUALS;
  }
  else if (strcmp(predicate_str, "st_dwithin") == 0) {
    return ST_DWITHIN;
  }
  else if (strcmp(predicate_str, "st_within") == 0) {
    return ST_WITHIN;
  }
  else if (strcmp(predicate_str, "st_overlaps") == 0) {
    return ST_OVERLAPS;
  }
  else {
    // std::cerr << "unrecognized join predicate " << std::endl;
    return 0;
  }
}

/* Report result separated by sep */
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
  /* getopt_long stores the option index here. */
  int option_index = 0;
  /* getopt_long uses opterr to report error*/
  opterr = 0 ;
  struct option long_options[] =
  {
    {"distance",   required_argument, 0, 'd'},
    {"shpidx1",  required_argument, 0, 'i'},
    {"shpidx2",  required_argument, 0, 'j'},
    {"predicate",  required_argument, 0, 'p'},
    {"fields",     required_argument, 0, 'f'},
    {0, 0, 0, 0}
  };

  int c;
  while ((c = getopt_long (argc, argv, "p:i:j:d:f:",long_options, &option_index)) != -1){
    switch (c)
    {
      case 0:
        /* If this option set a flag, do nothing else now. */
        if (long_options[option_index].flag != 0)
          break;
        cout << "option " << long_options[option_index].name ;
        if (optarg)
          cout << "a with arg " << optarg ;
        cout << endl;
        break;

      case 'p':
        stop.JOIN_PREDICATE = getJoinPredicate(optarg);
        break;

      case 'i':
        stop.shape_idx_1 = strtol(optarg, NULL, 10) + 2;
        stop.join_cardinality++;
        //printf ("shape index i: `%s'\n", optarg);
        break;

      case 'j':
        stop.shape_idx_2 = strtol(optarg, NULL, 10) + 2;
        stop.join_cardinality++;
        break;

      case 'd':
        stop.expansion_distance = atof(optarg);
        break;

      case 'f':
        setProjectionParam(optarg);
        //printf ("projection fields:  `%s'\n", optarg);
        break;

      case '?':
        return false;
        /* getopt_long already printed an error message. */
        break;

      default:
        return false;
        //abort ();
    }
  }

  // query operator validation 
  if (stop.JOIN_PREDICATE <= 0 )// is the predicate supported 
  { 
    cerr << "Query predicate is NOT set properly. Please refer to the documentation." << endl ; 
    return false;
  }
  // if the distance is valid 
  if (ST_DWITHIN == stop.JOIN_PREDICATE && stop.expansion_distance == 0.0)
  { 
    cerr << "Distance parameter is NOT set properly. Please refer to the documentation." << endl ;
    return false;
  }
  if (0 == stop.join_cardinality)
  {
    cerr << "Geometry field indexes are NOT set properly. Please refer to the documentation." << endl ;
    return false; 
  }

  print_stop();

  return true;
}

void usage(){
  cerr  << endl << "Usage: resque [OPTIONS]" << endl << "OPTIONS:" << endl;
  cerr << tab << "-p,  --predicate" << tab <<  "The spatial join predicate for query processing. Acceptable values are [st_intersects, " 
      << "st_disjoint, st_overlaps, st_within, st_equals, st_dwithin, st_crosses, st_touches, st_contains]." << endl;
  cerr << tab << "-i, --shpidx1"  << tab << "The index of the geometry field from the larger dataset. Index value starts from 1." << endl;
  cerr << tab << "-j, --shpidx2"  << tab << "The index of the geometry field from the smaller dataset. Index value starts from 1." << endl;
  cerr << tab << "-d, --distance" << tab << "Used together with st_dwithin predicate to indicates the join distance." 
      << "This field has no effect o other join predicates." << endl;
  cerr << tab << "-f, --fields"   << tab << "Output field election parameter. Fields from different dataset are separated with a colon (:), " 
      <<"and fields from the same dataset are separated with a comma (,). For example: if we want to only output fields 1, 3, and 5 from " 
      << "the first dataset (indicated with param -i), and output fields 1, 2, and 9 from the second dataset (indicated with param -j) "
      << " then we can provide an option such as: --fields 1,3,5:1,2,9 " << endl;
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
    usage();
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

