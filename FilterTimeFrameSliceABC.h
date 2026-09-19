/*
 * @file FilterTimeFrameSliceABC.icxx
 * @brief Slice Timeframe by Logic timing for NestDAQ
 * @date Created : 2024-05-04 12:31:55 JST
 *       Last Modified : 2024-07-21 02:23:44 JST (furukawa)
 *
 * @author Shinsuke OTA <ota@rcnp.osaka-u.ac.jp>
 * @comment Added geoToIndex function for VDC channel map loading
 */
#ifndef NESTDAQ_FILTERTIMEFRAMESLICEABC_H
#define NESTDAQ_FILTERTIMEFRAMESLICEABC_H

#include "fairmq/Device.h"
#include "KTimer.cxx"
#include "SubTimeFrameHeader.h"
#include "TimeFrameHeader.h"
#include "FilterHeader.h"
#include "HeartbeatFrameHeader.h"
#include "FrameContainer.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace nestdaq {
   class FilterTimeFrameSliceABC;
}

struct Wire_map {
    int catid = -1;
    int id = -1;
    int sh = -1;
    int fp = -1;
    int det = -1;
    uint64_t geo = -1;
    int ch = -1;
};

class nestdaq::FilterTimeFrameSliceABC : public fair::mq::Device {
public:
   FilterTimeFrameSliceABC();
   virtual ~FilterTimeFrameSliceABC() override = default;
   
   virtual void PreRun() override;
   virtual void InitTask() override;
   virtual bool ConditionalRun() override;
   virtual void PostRun() override;

   struct OptionKey {
      static constexpr std::string_view InputChannelName {"in-chan-name"};
      static constexpr std::string_view OutputChannelName {"out-chan-name"};
      static constexpr std::string_view DQMChannelName {"out-chan-name"};
      static constexpr std::string_view PollTimeout        {"poll-timeout"};
      static constexpr std::string_view SplitMethod        {"split"};
      static constexpr std::string_view EvaluationMode     {"evaluation-mode"};
   };

protected:
   virtual bool ParseMessages(FairMQParts& inParts);
   virtual bool ProcessSlice(TTF& ) { return true; }
   
   std::string fInputChannelName;
   std::string fOutputChannelName;
   std::string fName;
   uint32_t fId;

   // control
   uint32_t fNextIdx;

   std::vector<KTimer> fKTimer;
   bool fDoCheck;

   // time frame
   std::vector<TTF> fTFs; 

   // Maximum channel number in the wire map.
   static constexpr int maxCh = 112;

   struct FemConfig {
      uint64_t geo;
      int plane;
   };

   // Fixed online lookup order. Keep the FEM address and plane assignment in
   // one table so the wire-map slot and the detector plane cannot diverge.
   static constexpr std::size_t kNumFem = 8;
   inline static constexpr std::array<FemConfig, kNumFem> kFemConfigs {{
      {0xc0a802a1, 1},
      {0xc0a802a2, 1},
      {0xc0a802a3, 2},
      {0xc0a802a4, 2},
      {0xc0a802a5, 3},
      {0xc0a802a6, 3},
      {0xc0a802a7, 4},
      {0xc0a802a8, 4},
   }};

   using WireMapArray =
      std::array<std::array<Wire_map, maxCh + 1>, kNumFem>;
   WireMapArray wireMapArray {};


   int geoToIndex(uint64_t geo);

   // output
   int fNumDestination {0};  
   uint32_t fDirection {0};  
   int fPollTimeoutMS  {0}; 
   int fSplitMethod    {0};

   // Evaluation mode flag
   bool EvaluationMode {false};
};

#endif  // NESTDAQ_FILTERTIMEFRAMESLICEABC_H
