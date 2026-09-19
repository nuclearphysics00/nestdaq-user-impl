/**
 * @file FilterTimeFrameSliceByClusterMultiplicity_GR.h
 * @brief Header file for FilterTimeFrameSliceByClusterMultiplicity_GR class
 * @date Created : 2024-05-04 12:27:57 JST
 *       Last Modified : 2026-09-19 16:10:59 JST
 *
 * author Fumiya Furukawa <fumiya@rcnp.osaka-u.ac.jp>
 * @comment GR-specific cluster multiplicity filter
 */
#ifndef NESTDAQ_TIMEFRAMESLICERBYCLUSTERMULTIPLICITY_GR_H
#define NESTDAQ_TIMEFRAMESLICERBYCLUSTERMULTIPLICITY_GR_H

#include "fairmq/Device.h"
#include "KTimer.cxx"
#include "SubTimeFrameHeader.h"
#include "TimeFrameHeader.h"
#include "FilterHeader.h"
#include "HeartbeatFrameHeader.h"
#include "FrameContainer.h"
#include "FilterTimeFrameSliceABC.h"
#include <vector>
#include <memory>
#include <map>
#include <array>

namespace nestdaq {
   class FilterTimeFrameSliceByClusterMultiplicity_GR;
}

class nestdaq::FilterTimeFrameSliceByClusterMultiplicity_GR : public nestdaq::FilterTimeFrameSliceABC {
public:
   FilterTimeFrameSliceByClusterMultiplicity_GR();
   virtual ~FilterTimeFrameSliceByClusterMultiplicity_GR() override = default;

   virtual bool ProcessSlice(TTF& ) override;

   void SetMinClusterSize(int size) { minClusterSize = size; }
   void SetMinClusterCountPerPlane(int count) { minClusterCountPerPlane = count; }

private:
   struct GeofieldClusterInfo {
       int clusterCount;
       std::vector<int> clusterSizes;
   };

   int findWirenumber(const WireMapArray& wireMapArray, uint64_t geo, int ch, int *foundid, int *foundGeo, int &Geofield);
   std::vector<std::vector<int>> clusterNumbers(const std::vector<int>& numbers);
   std::map<int, GeofieldClusterInfo> analyzeGeofieldClusters(const std::map<int, std::vector<int>>& GeoIDs);
   bool allKeysHaveAtLeastOneCluster(const std::map<int, GeofieldClusterInfo>& geofieldClusterMap);

   std::map<int, std::vector<int>> GeoIDs;

   int minClusterSize;
   int minClusterCountPerPlane;
   int totalCalls;
   int totalAccepted;
};

#endif  // NESTDAQ_TIMEFRAMESLICERBYCLUSTERMULTIPLICITY_GR_H
