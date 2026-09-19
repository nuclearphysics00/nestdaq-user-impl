/**
 * @file FilterTimeFrameSliceByTOT_GR.h
 * @brief Class for filtering time frame slice by Time Over Threshold (TOT)
 * @date Created : 2024-05-04 12:27:57 JST
 *       Last Modified : 2026-09-19 16:10:59 JST
 *
 * author Fumiya Furukawa <fumiya@rcnp.osaka-u.ac.jp>
 * @comment Add Reduction-rate, Throughput per unit time
 */

#ifndef NESTDAQ_TIMEFRAMESLICERBYTOT_GR_H
#define NESTDAQ_TIMEFRAMESLICERBYTOT_GR_H

#include "fairmq/Device.h"
#include "KTimer.cxx"
#include "SubTimeFrameHeader.h"
#include "TimeFrameHeader.h"
#include "FilterHeader.h"
#include "HeartbeatFrameHeader.h"
#include "FrameContainer.h"
#include "FilterTimeFrameSliceABC.h"
#include <array>
#include <vector>
#include <memory>
#include <map>

namespace nestdaq {
   class FilterTimeFrameSliceByTOT_GR;
}

class nestdaq::FilterTimeFrameSliceByTOT_GR : public nestdaq::FilterTimeFrameSliceABC {
public:
   FilterTimeFrameSliceByTOT_GR();
   virtual ~FilterTimeFrameSliceByTOT_GR() override = default;

   virtual bool ProcessSlice(TTF& ) override;

private:
   int totalCalls;
   int totalAccepted;
   int DeterminePlane(uint64_t fem, int ch);
   bool Chargelogic(const std::map<int, std::tuple<int, int>>& chargeSums);
   static uint64_t eventID;
protected:
   virtual void InitTask() override;
   std::array<double, 6> fCutThresholds {{90.0, 90.0, 90.0, 90.0, 90.0, 90.0}};

};

#endif  // NESTDAQ_TIMEFRAMESLICERBYTOT_GR_H
