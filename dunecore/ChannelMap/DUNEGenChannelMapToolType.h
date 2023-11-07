#include "art/Utilities/ToolMacros.h"
#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "dunecore/ChannelMap/DUNEGenChannelMapSP.h"

 class DUNEGenChannelMapToolType {
  public:
    virtual ~DUNEGenChannelMapToolType() noexcept = default;
    virtual dune::DUNEGenChannelMapSP::ChanInfo_t GetChanInfoFromDetectorElements(dune::DUNEGenChannelMapSP::ChanInfo_t &detinfo) = 0;
    virtual dune::DUNEGenChannelMapSP::ChanInfo_t GetChanInfoFromOfflChan(int offlchan) = 0;
  };
