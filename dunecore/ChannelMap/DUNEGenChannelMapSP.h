///////////////////////////////////////////////////////////////////////////////////////////////////
// Class:       DUNEGenChannelMapSP
// Module type: algorithm
// File:        DUNEGenChannelMapSP.h
// Author:      Tom Junk, October 2023
//
// Implementation of hardware-offline channel mapping reading from a file.
// art-independent class
// This class is intended to be entirely driven by the input data file -- names of colums, which
// are to be used as indexes for lookup from electronics, and types.
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef DUNEGenChannelMapSP_H
#define DUNEGenChannelMapSP_H

#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <stdexcept>
#include <any>

namespace dune {
  class DUNEGenChannelMapSP;
}

class dune::DUNEGenChannelMapSP {

public:

  typedef struct ChanInfoStruct : std::unordered_map<std::string, std::any>
  {
    int offlchan;   // for convenience so we don't have to any_cast to get this piece of information
  } ChanInfo_t;
  
  //typedef std::unordered_map<std::string, std::any> ChanInfo_t;

  DUNEGenChannelMapSP();  // constructor

  // initialize:  read map from file.  File contains number of columns, column names, types, and
  // which columns are to be used for map lookup

  void ReadMapFromFile(const std::string &fullname);

  // TPC channel map accessors

  ChanInfo_t GetChanInfoFromDetectorElements(ChanInfo_t &detinfo) const;
  ChanInfo_t GetChanInfoFromOfflChan(int offlchan) const;

private:

// maximum of four-dimensional lookup
// for definiteness, if the dimensionality of the map is less than four, dummy constant
// keys are used to fill out the four keys
// Furthermore, this map is limited to lookup by integer keys -- no lookups by string or
// floating point numbers or other more exotic things are allowed.
// save the channel info as a vector of data instead of as the chaninfo map since that otherwise would
// involve saving nchannels copies of all the column names
  
  std::unordered_map<int,
  		     std::unordered_map<int,
				        std::unordered_map<int,
							   std::unordered_map<int,int> > > > m_DetToChanInfo;

  // map of chan info indexed by offline channel number 

  std::unordered_map<unsigned int, int> m_OfflToChanInfo;

  // a list of the names of the lookup keys in the order in which the map
  std::vector<std::string> m_KeyStrings;
  std::vector<std::string> m_ColumnNames;
  std::unordered_map<std::string, int> m_CIMap;  // convenience map to look up column number from its name
  std::vector<int> m_ColumnTypes;   // 0: int,  1: string, 2: float
  std::vector<int> m_KeyColFlags;   // 0: not used in electronics map,  1:  used in electronics map
  std::vector<int> m_WhichKey;      // convenience for assigning key ordering
  
  std::vector<std::vector<std::any> > m_MapInfo;

  //-----------------------------------------------

  bool check_offline_channel(int offlineChannel) const
  {
    if ( m_OfflToChanInfo.find(offlineChannel) == m_OfflToChanInfo.end())
    {
      return false;
      //throw std::range_error( "DUNEGenChannelMapSP offline Channel out of range"); 
    }
    return true;
  };

};


#endif
