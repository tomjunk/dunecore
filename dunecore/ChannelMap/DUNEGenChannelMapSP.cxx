///////////////////////////////////////////////////////////////////////////////////////////////////
// Class:       DUNEGenChannelMapSP
// Module type: standalone algorithm
// File:        DUNEGenChannelMapSP.cxx
// Author:      Tom Junk, October 2023
//
// Implementation of hardware-offline channel mapping reading from a file.
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "DUNEGenChannelMapSP.h"

#include <fstream>
#include <sstream>
#include <typeinfo>
#include <stdexcept>

// so far, nothing needs to be done in the constructor

dune::DUNEGenChannelMapSP::DUNEGenChannelMapSP()
{
}

// format of input file
// All lines beginning with # are comments
// first non-comment line:  single integer -- the number of columns in the file
// second non-comment line:  space-separated names of columns (names do not contain spaces)
// third non-comment line:  types of variables:   I,  C (or S),  F  for integer, string or float
// fourth non-comment line: switch to indicate which columns are used as keys in the lookup.  0: not in lookup.  1: in lookup
//   example:  crate, slot, stream, chan for WIBEth
// fifth through end:  data

void dune::DUNEGenChannelMapSP::ReadMapFromFile(const std::string &fullname)
{
  std::ifstream inFile(fullname, std::ios::in);
  std::string line;
  std::string rawline;
  int state=0;  
  // state = 0:  haven't read number of columns line
  // state = 1:  have read number of columns, haven't read column names
  // state = 2:  have read names, haven't read types
  // state = 3:  have read types, haven't read key flags
  // state = 4:  have read key flags, reading data

  int ncolumns = 0;
  
  while (std::getline(inFile,rawline)) {

    // comment filtering -- first pound sign indicates a comment
    // lines that are entirely comments are skipped.
    // lines that have a few characters (such as spaces) and then a comment will confuse this parser
    
    std::string line = rawline;
    auto ipound = rawline.find("#");
    if (ipound != std::string::npos)
      {
	line = rawline.substr(0,ipound);
      }
    if (line.size() == 0) continue;
    
    std::stringstream linestream(line);
    
    if (state == 0)  // read in ncolumns
      {
	linestream >> ncolumns;
	state++;
      }
    else if (state == 1)   // read in column names
      {
	for (int i=0; i<ncolumns; ++i)
	  {
	    std::string cn;
	    linestream >> cn;     //  question -- what is the delimeter here?
	    m_ColumnNames.push_back(cn);
	    m_CIMap[cn] = i;
	  }
	state++;
      }
    else if (state == 2)  // read in column types
      {
	for (int i=0; i<ncolumns; ++i)
	  {
	    std::string ct;
	    linestream >> ct;
	    if (ct == "I")
	      {
		m_ColumnTypes.push_back(0);
	      }
	    else if (ct == "C" || ct == "S")
	      {
		m_ColumnTypes.push_back(1);
	      }
	    else if (ct == "F")
	      {
		m_ColumnTypes.push_back(2);
	      }
	  }
	state++;
      }
    else if (state == 3)   // read in key flags
      {
	for (int i=0; i<ncolumns; ++i)
	  {
	    int cf = 0;
	    linestream >> cf;
	    if (cf != 0 && m_ColumnTypes.at(i) != 0)
	      {
		std::cout << "DUNEGenChannelMapSP: Map keys must be integer.  This one isn't: " <<
		  m_ColumnNames.at(i) << " " << m_ColumnTypes.at(i) << " key flag: " << cf << std::endl; 
		throw std::bad_typeid();
	      }
	    m_KeyColFlags.push_back(cf);
	    if (cf)
	      {
		m_WhichKey.push_back(m_KeyStrings.size());
		m_KeyStrings.push_back(m_ColumnNames.at(i));
	      }
	    else
	      {
		m_WhichKey.push_back(0);
	      }
	  }
	state++;
      }
    else if (state == 4) // read in data
      {
	int ci=0;
	std::string cs{""};
	float cf=0.0;
	std::vector<std::any> dataline;
	std::vector<int> mapkeys {0,0,0,0};
	int ikey = 0;
	int offlchan = 0;
	
	for (int i=0; i<ncolumns; ++i)
	  {
	    if (m_ColumnTypes.at(i) == 0)
	      {
	        linestream >> ci;
		dataline.push_back(ci);
		if (m_KeyColFlags.at(i))
		  {
		    mapkeys.at(ikey) = ci;
		    ++ikey;
		  }
		if (m_ColumnNames.at(i) == "offlchan")
		  {
		    offlchan = ci;
		  }
	      }
	    else if (m_ColumnTypes.at(i) == 1 )
	      {
	        linestream >> cs;
		dataline.push_back(cs);
	      }	    
	    else if (m_ColumnTypes.at(i) == 2)
	      {
	        linestream >> cf;
		dataline.push_back(cf);
	      }
	  }
	int mloc = (int) m_MapInfo.size();
	m_DetToChanInfo[mapkeys[0]][mapkeys[1]][mapkeys[2]][mapkeys[3]] = mloc;
	m_OfflToChanInfo[offlchan] = mloc;
	m_MapInfo.push_back(dataline);
      }
  }    
  inFile.close();
  if (m_KeyStrings.size() > 4)
    {
      std::cout << "DUNEGenChannelMapSP: Too Many Map Keys: " << m_KeyStrings.size() << std::endl << "  Names: ";
      for (size_t i=0; i< m_KeyStrings.size(); ++i)
	{
	  std::cout << "  " << m_KeyStrings.at(i);
	}
      std::cout << std::endl;
      throw std::domain_error("DUNEGenChannelMapSP: Too Many Map Keys");
    }
}

dune::DUNEGenChannelMapSP::ChanInfo_t dune::DUNEGenChannelMapSP::GetChanInfoFromDetectorElements(ChanInfo_t &detinfo) const {
  ChanInfo_t badInfo = {};
  badInfo["valid"] = false;
  ChanInfo_t Info = {};
  
  std::vector<int> mapkeys {0,0,0,0};
  for (const auto& i : detinfo)
    {
      auto fcim = m_CIMap.find(i.first);
      if (fcim == m_CIMap.end())
	{
	  throw std::logic_error("DUNEGenChannelMapSP ununderstood map key name " + i.first);
	}
      int icol = fcim->second;
      //std::cout << "lookup key: " << i.first << " " << icol << " " << m_WhichKey.at(icol) << " ";
      if (m_KeyColFlags.at(icol))
	{
	  mapkeys.at(m_WhichKey.at(icol)) = std::any_cast<int>(i.second);
	  //std::cout << mapkeys.at(m_WhichKey.at(icol));
	}
      //std::cout << std::endl;
    }

  auto fm1 = m_DetToChanInfo.find(mapkeys[0]);
  if (fm1 == m_DetToChanInfo.end()) return badInfo;
  auto& m1 = fm1->second;

  auto fm2 = m1.find(mapkeys[1]);
  if (fm2 == m1.end()) return badInfo;
  auto& m2 = fm2->second;

  auto fm3 = m2.find(mapkeys[2]);
  if (fm3 == m2.end()) return badInfo;
  auto& m3 = fm3->second;

  auto fm4 = m3.find(mapkeys[3]);
  if (fm4 == m3.end()) return badInfo;
  int infoindex = fm4->second;
  
  ChanInfo_t info={};
  for (size_t i=0; i<m_ColumnNames.size(); ++i)
    {
      if (m_ColumnNames.at(i) == "offlchan")
	{
	  info.offlchan = std::any_cast<int> (m_MapInfo.at(infoindex).at(i));
	}
      else
	{
          info[m_ColumnNames.at(i)] = m_MapInfo.at(infoindex).at(i);
	}
    }
  info["valid"] = true;
  return info;
}

dune::DUNEGenChannelMapSP::ChanInfo_t dune::DUNEGenChannelMapSP::GetChanInfoFromOfflChan(int offlchan) const {
  ChanInfo_t badInfo = {};
  badInfo["valid"] = false;
  ChanInfo_t info={};

  auto fm = m_OfflToChanInfo.find(offlchan);
  if (fm == m_OfflToChanInfo.end())
    {
      info = badInfo;
    }
  else
    {
      int infoindex = fm->second;
      info.offlchan = -1;
      bool infovalid = false;
      for (size_t i=0; i<m_ColumnNames.size(); ++i)
        {
	  if (m_ColumnNames.at(i) == "offlchan")
	    {
	      info.offlchan = std::any_cast<int> (m_MapInfo.at(infoindex).at(i));
	      infovalid = true;
	    }
	  else
	    {
              info[m_ColumnNames.at(i)] = m_MapInfo.at(infoindex).at(i);
	    }
        }
      info["valid"] = infovalid;
    }
  return info;  
}

