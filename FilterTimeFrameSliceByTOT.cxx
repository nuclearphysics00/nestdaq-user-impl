/**
 * @file FilterTimeFrameSliceByTOT.cxx 
 * @brief Slice TimeFrame by Time Over Threshold (TOT) for NestDAQ
 * @date Created : 2024-05-04 12:27:57 JST
 *       Last Modified : 2024-07-15 00:47:34 JST
 * 
 * @comment Modify FilterTimeFrameSliceBySomething.cxx for MultiHit 
 * 
 */

#include <fstream>
#include <iostream>
#include <vector>
#include <memory>
#include <map>
#include <tuple>
#include "FilterTimeFrameSliceByTOT.h"
#include "FilterTimeFrameSliceABC.icxx"
#include "fairmq/runDevice.h"
#include "utility/MessageUtil.h"
#include "UnpackTdc.h"
#include "SubTimeFrameHeader.h"
#include "TimeFrameHeader.h"

#define DEBUG 0
#define DEBUG_LOGIC 0
#define OUTPUT 0
#define OUTPUT_Charge 0
#define OUTPUT_Filtered_Charge 0
#define Plastic_signal 1
#define OUTPUT_Filtered_Events 0
#define OUTPUT_Filtered_Events_all 0
#define OUTPUT_Checking_ALL_Charge 0
#define OUTPUT_Checking_Filtered_Charge 0

using nestdaq::FilterTimeFrameSliceByTOT;
namespace bpo = boost::program_options;
uint64_t nestdaq::FilterTimeFrameSliceByTOT::eventID = 0; 


FilterTimeFrameSliceByTOT::FilterTimeFrameSliceByTOT()
    : totalCalls(0), totalAccepted(0) // Initialize only once
{
        #if DEBUG
        std::cout << "[DEBUG] Constructor called. TotalCalls=" << totalCalls 
                << ", TotalAccepted=" << totalAccepted << std::endl;
        #endif
}

void FilterTimeFrameSliceByTOT::InitTask()
{
    // Call the parent class's Initask
    FilterTimeFrameSliceABC::InitTask();

    if (fConfig->Count("CutThreshold")) {
        std::string threshold_str = fConfig->GetValue<std::string>("CutThreshold");
        fCutThreshold = std::stod(threshold_str);
    } else {
        fCutThreshold = 90.0; // default value
    }

    std::cout << "[FilterTimeFrameSliceByTOT] Dynamic CutThreshold is set to: " << fCutThreshold << std::endl;
}

bool FilterTimeFrameSliceByTOT::ProcessSlice(TTF& tf)
{
    if (!ValidateSliceFrames(tf)) {
        return false;
    }

    #if DEBUG
    std::cout << "[DEBUG] ProcessSlice called. TotalCalls: " << totalCalls << std::endl;
    #endif

    auto start_time = std::chrono::high_resolution_clock::now();
    size_t total_size = 0;
    eventID++;
    std::cout  << "eventID: " << eventID << std::endl;

    std::map<int, std::tuple<int, int>> chargeSums;
    std::vector<std::tuple<uint64_t, int, int, int, int>> eventDetails;

    auto tfHeader = tf.GetHeader();
    #if DEBUG
    std::cout << "totalCalls: " << totalCalls << std::endl;
    #endif

    totalCalls++; // Increment total calls per TimeFrame

    for (auto* subTimeFrame : tf) {
        auto* header = subTimeFrame->GetHeader();

        for (auto* hbf : *subTimeFrame) {
            uint64_t nData = hbf->GetNumData();

            #if DEBUG
            std::cout << "[DEBUG] SubTimeFrame Header FEM Type: " << header->femType
                      << ", NumData: " << nData << std::endl;
            #endif

            for (uint64_t i = 0; i < nData; ++i) {
                if (header->femType == SubTimeFrame::TDC64H) {
                    TDC64H::tdc64 tdc;
                    TDC64H::Unpack(hbf->UncheckedAt(i), &tdc);
                
                } else if (header->femType == SubTimeFrame::TDC64L) {
                    TDC64L::tdc64 tdc;
                    TDC64L::Unpack(hbf->UncheckedAt(i), &tdc);
                
                } else if (header->femType == SubTimeFrame::TDC64H_V3) {
                    TDC64H_V3::tdc64 tdc{};
                    TDC64H_V3::Unpack(hbf->UncheckedAt(i), &tdc);
                    if (tdc.ch < 0 || tdc.tot < 0) {
                        continue;
                    }
                    int charge = tdc.tot;
                    // Plastic_str_1
                    if ((tdc.ch == 10) || (tdc.ch == 11)){
                    //Pla _att_1
                    //if ((tdc.ch == 42) || (tdc.ch == 43)){
                    int planeId = DeterminePlane(header->femId, tdc.ch);
                

                    #if DEBUG
                    std::cout << "[DEBUG] Processed TDC64H_V3: Charge=" << charge
                              << ", PlaneID=" << planeId << std::endl;
                    #endif
                
                    #if Plastic_signal
                        if (planeId != -1) {
                            auto& [sum, count] = chargeSums[planeId];
                            sum += charge;
                            count++;
                            std::cout << "[DEBUG] Updated chargeSums for PlaneID_High=" << planeId
                                    << ": Sum=" << sum << ", Count=" << count << std::endl;
                        }
                    }
                    #endif
                } else if (header->femType == SubTimeFrame::TDC64L_V3) {
                    TDC64L_V3::tdc64 tdc{};
                    TDC64L_V3::Unpack(hbf->UncheckedAt(i), &tdc);
                    if (tdc.ch < 0 || tdc.tot < 0) {
                        continue;
                    }
                    int charge = tdc.tot;
                    int planeId = DeterminePlane(header->femId, tdc.ch);
                    int ch = tdc.ch;
                    int timing = tdc.tdc;
                

                    #if DEBUG
                    std::cout << "[DEBUG] Processed TDC64L_V3: Charge=" << charge
                              << ", PlaneID=" << planeId << std::endl;
                    #endif

                    if (planeId != -1) {
                        auto& [sum, count] = chargeSums[planeId];
                        sum += charge;
                        count++;

                        #if DEBUG
                        std::cout << "[DEBUG] Updated chargeSums for PlaneID_Low=" << planeId
                                  << ": Sum=" << sum << ", Count=" << count << std::endl;
                        #endif

                        eventDetails.emplace_back(eventID,header->femId,ch,timing,charge);
                    }
                }
            }
        }
    }

    #if DEBUG
    std::cout << "[DEBUG] Finished processing all SubTimeFrames." << std::endl;
    for (const auto& [planeId, chargeData] : chargeSums) {
        auto [sum, count] = chargeData;
        std::cout << "[DEBUG] PlaneID=" << planeId 
                  << ", TotalCharge=" << sum 
                  << ", Count=" << count << std::endl;
    }
    #endif

    //std::cout << "Logic: " << Chargelogic(chargeSums) << std::endl;
    #if OUTPUT_Filtered_Events_all
        std::ofstream eventFile("filtered_events.all.txt",std::ios::app);
        if (eventFile.is_open()) {
            for (const auto& [evtID, femId, ch, timing, charge] : eventDetails) {
                eventFile << std::dec << evtID << "," << std::hex << "0x" <<  femId << "," << std::dec << ch << "," << timing << "," << charge << "\n";
            }
            eventFile.close();
        }
    #endif

    if (Chargelogic(chargeSums)) {
        totalAccepted++; // Increment total accepted if true
        #if DEBUG
        std::cout << "[DEBUG] Filtere accepted. TotalAccepted: " << totalAccepted << std::endl;
        #endif

        #if OUTPUT_Filtered_Events
            std::ofstream eventFile("filtered_events.txt",std::ios::app);
            if (eventFile.is_open()) {
                for (const auto& [evtID, femId, ch, timing, charge] : eventDetails) {
                    eventFile << std::dec << evtID << "," << std::hex << "0x" <<  femId << "," << std::dec << ch << "," << timing << "," << charge << "\n";
                }
                eventFile.close();
            }
        #endif
        return true;
    }

    return false;
}

int FilterTimeFrameSliceByTOT::DeterminePlane(uint64_t fem, int ch) {
    int planeId = -1;

    if ((fem == 0xc0a802a1) || (fem == 0xc0a802a2)) {
        planeId = 1; // plane1_vdc_x1
    } else if ((fem == 0xc0a802a3) || (fem == 0xc0a802a4)) {
        planeId = 2; // plane2_vdc_u1
    } else if ((fem == 0xc0a802a5) || (fem == 0xc0a802a6)) {
        planeId = 3; // plane3_vdc_x2
    } else if ((fem == 0xc0a802a7) || (fem == 0xc0a802a8)) {
        planeId = 4; // plane4_vdc_u2
    }
    else if ((fem == 0xc0a802a9) && ((ch == 10) || (ch == 11))) {
    //else if ((fem == 0xc0a802a9) && ((ch == 42) || (ch == 43))) {
        planeId = 5; // plane5_plastic1_attnueate
    }

    #if DEBUG
    if (planeId == -1) {
        std::cerr << "[DEBUG] Invalid FEM ID or Channel: FEM=" << fem 
                  << ", Channel=" << ch << std::endl;
    } else {
        std::cout << "[DEBUG] Determined PlaneID=" << planeId 
                  << " for FEM=" << fem << ", Channel=" << ch << std::endl;
    }
    #endif

    return planeId;
}

bool FilterTimeFrameSliceByTOT::Chargelogic(const std::map<int, std::tuple<int, int>>& chargeSums) {
    try {
        auto [chargeSum1, n1] = chargeSums.at(1); // plane1x1
        auto [chargeSum2, n2] = chargeSums.at(2); // plane2u2
        auto [chargeSum3, n3] = chargeSums.at(3); // plane3x2
        auto [chargeSum4, n4] = chargeSums.at(4); // plane4u2
        #if Plastic_signal
        auto [chargeSum5, n5] = chargeSums.at(5); // plane5
        #endif

        #if DEBUG_LOGIC
        std::cout << "[DEBUG] Charge Sums and Counts: "
                  << "Plane1(Sum=" << chargeSum1 << ", Count=" << n1 << "), "
                  << "Plane2(Sum=" << chargeSum2 << ", Count=" << n2 << "), "
                  << "Plane3(Sum=" << chargeSum3 << ", Count=" << n3 << "), "
                  << "Plane4(Sum=" << chargeSum4 << ", Count=" << n4 << "), "
                  << "Plane5(Sum=" << chargeSum5 << ", Count=" << n5 << ")"
                  << std::endl;
        #endif

        #if OUTPUT_Charge
            // Write TOT distribution
                std::ofstream outFile("normalized_charges.txt", std::ios::app); // Append mode
                if (!outFile.is_open()) {
                    std::cerr << "Failed to open file for writing." << std::endl;
                }
                double normCharge1 = (double)chargeSum1 / n1;
                double normCharge2 = (double)chargeSum2 / n2;
                double normCharge3 = (double)chargeSum3 / n3;
                double normCharge4 = (double)chargeSum4 / n4;
                // double normCharge5 = (double)chargeSum5 / n5;


                outFile << std::fixed << std::setprecision(4) << std::setw(10) << normCharge1 << ","
                        << std::setw(10) << normCharge2 << ","
                        << std::setw(10) << normCharge3 << ","
                        << std::setw(10) << normCharge4 
//                        << std::setw(10) << normCharge5  
                        << "\n";
                outFile.close();
            #endif
            
            #if OUTPUT
            std::ofstream reduction_file("data_reduction_rate_tot.txt", std::ios_base::app);
            if (reduction_file.is_open()) {
                if (reduction_file.tellp() == 0) {
                    reduction_file << "#DataReductionRate,TotalCalls,TotalAccepted\n";
                }
                double reductionRate = 100.0 * (1.0 - static_cast<double>(totalAccepted) / static_cast<double>(totalCalls));
                reduction_file << std::fixed << std::setprecision(2) << reductionRate << ","
                                << totalCalls << ","
                                << totalAccepted << "\n";
                reduction_file.close();
            } else {
                std::cerr << "Failed to open file for writing data reduction rate." << std::endl;
            }
            #endif


/*         if (n1 > 0 &&
            n2 > 0 &&
            ((static_cast<double>(chargeSum3) / n3) + (static_cast<double>(chargeSum2) / n2)) / 2. > 50 &&
            ((static_cast<double>(chargeSum1) / n1) + (static_cast<double>(chargeSum2) / n2)) / 2. > 50 &&
            -0.934 * (static_cast<double>(chargeSum3) / n3) + (static_cast<double>(chargeSum4) / n4) > -36 &&
            n5 > 0 &&
            ((static_cast<double>(chargeSum5) / n5) +
            (static_cast<double>(chargeSum1) / n1) +
            (static_cast<double>(chargeSum2) / n2)) / 2. > 100) { */

// Miss 
/*         if (n1 > 1 &&
            n2 > 1 &&
            n3 > 1 &&
            n4 > 1 &&
            n5 > 0 &&
            ((static_cast<double>(chargeSum3) / n3) + (static_cast<double>(chargeSum2) / n2))  > 65 &&
            ((static_cast<double>(chargeSum1) / n1) + (static_cast<double>(chargeSum2) / n3))  > 65 &&
            ((static_cast<double>(chargeSum3) / n3) + (static_cast<double>(chargeSum4) / n4))  > 65 &&
            ((static_cast<double>(chargeSum1) / n1) + (static_cast<double>(chargeSum2) / n4))  > 65 &&
            ((static_cast<double>(chargeSum1) / n2) + (static_cast<double>(chargeSum2) / n4))  > 65 
) */ 


#if OUTPUT_Checking_ALL_Charge
    std::ofstream filteredFile("all_tot_dump.txt", std::ios::app); 
    if (!filteredFile.is_open()) {
        std::cerr << "Failed to open all_tot_dump.txt for writing." << std::endl;
    } else {
        double normCharge1 = (n1 > 0) ? (double)chargeSum1  : 0;
        double normCharge2 = (n2 > 0) ? (double)chargeSum2  : 0;
        double normCharge3 = (n3 > 0) ? (double)chargeSum3  : 0;
        double normCharge4 = (n4 > 0) ? (double)chargeSum4  : 0;

        double avgCharge1 = (n1 > 0) ? (double)chargeSum1 / n1 : 0;
        double avgCharge2 = (n2 > 0) ? (double)chargeSum2 / n2 : 0;
        double avgCharge3 = (n3 > 0) ? (double)chargeSum3 / n3 : 0;
        double avgCharge4 = (n4 > 0) ? (double)chargeSum4 / n4 : 0;

        filteredFile << std::fixed << std::setprecision(4)
                     << std::setw(10) << eventID << ","  // eventID
                     << std::setw(10) << normCharge1 << ","  //  chargeSum
                     << std::setw(10) << normCharge2 << ","
                     << std::setw(10) << normCharge3 << ","
                     << std::setw(10) << normCharge4 << ","
                     << std::setw(5)  << n1 << ","  //  n
                     << std::setw(5)  << n2 << ","
                     << std::setw(5)  << n3 << ","
                     << std::setw(5)  << n4 << ","
                     << std::setw(10) << avgCharge1 << ","  // chargeSum / n
                     << std::setw(10) << avgCharge2 << ","
                     << std::setw(10) << avgCharge3 << ","
                     << std::setw(10) << avgCharge4 << "\n";

        filteredFile.close();
    }
#endif



// Modify
// only modify cut value 100->75 2025/09/12
        if (
             n1 > 1 &&
           n2 > 1 &&
           n3 > 1 &&
           n4 > 1 &&
           n5 > 0 && 
           ((static_cast<double>(chargeSum3) / n3) + (static_cast<double>(chargeSum2) / n2))  > 90 &&
           ((static_cast<double>(chargeSum1) / n1) + (static_cast<double>(chargeSum2) / n2))  > 90 &&
           ((static_cast<double>(chargeSum3) / n3) + (static_cast<double>(chargeSum4) / n4))  > 90 &&
           ((static_cast<double>(chargeSum1) / n1) + (static_cast<double>(chargeSum4) / n4))  > 90 &&
           ((static_cast<double>(chargeSum2) / n2) + (static_cast<double>(chargeSum4) / n4))  > 90 &&
           ((static_cast<double>(chargeSum1) / n1) + (static_cast<double>(chargeSum3) / n3))  > 90  
) 
/*         if (
            n2 > 1 &&
            n4 > 1 &&
            ((static_cast<double>(chargeSum2) / n2) + (static_cast<double>(chargeSum4) / n4))  > 40 
) */

        {
            #if OUTPUT_Checking_Filtered_Charge
                std::ofstream filteredFile("filtered_tot_dump.txt", std::ios::app); 
                if (!filteredFile.is_open()) {
                    std::cerr << "Failed to open filtered_tot_dump..txt for writing." << std::endl;
                } else {
                    double normCharge1 = (n1 > 0) ? (double)chargeSum1  : 0;
                    double normCharge2 = (n2 > 0) ? (double)chargeSum2  : 0;
                    double normCharge3 = (n3 > 0) ? (double)chargeSum3  : 0;
                    double normCharge4 = (n4 > 0) ? (double)chargeSum4  : 0;

                    double avgCharge1 = (n1 > 0) ? (double)chargeSum1 / n1 : 0;
                    double avgCharge2 = (n2 > 0) ? (double)chargeSum2 / n2 : 0;
                    double avgCharge3 = (n3 > 0) ? (double)chargeSum3 / n3 : 0;
                    double avgCharge4 = (n4 > 0) ? (double)chargeSum4 / n4 : 0;

                    filteredFile << std::fixed << std::setprecision(4)
                                << std::setw(10) << eventID << ","  // eventID
                                << std::setw(10) << normCharge1 << ","  //  chargeSum
                                << std::setw(10) << normCharge2 << ","
                                << std::setw(10) << normCharge3 << ","
                                << std::setw(10) << normCharge4 << ","
                                << std::setw(5)  << n1 << ","  //  n
                                << std::setw(5)  << n2 << ","
                                << std::setw(5)  << n3 << ","
                                << std::setw(5)  << n4 << ","
                                << std::setw(10) << avgCharge1 << ","  // chargeSum / n
                                << std::setw(10) << avgCharge2 << ","
                                << std::setw(10) << avgCharge3 << ","
                                << std::setw(10) << avgCharge4 << "\n";

                    filteredFile.close();
                }
            #endif
            

            std::cout << "success" << std::endl;
            return true;
        }
/*             if (n1 > 0 & n2 > 0) {
                return true;
            } */
        

    } catch (const std::out_of_range& e) {
        #if DEBUG_LOGIC
        std::cerr << "[DEBUG] Out of range exception in Chargelogic: " 
                  << e.what() << std::endl;
        #endif
        return false; 
    }


    
    return false;
}

void addCustomOptions(bpo::options_description& options)
{
    using opt = FilterTimeFrameSliceByTOT::OptionKey;

    options.add_options()
        (opt::InputChannelName.data(),
         bpo::value<std::string>()->default_value("in"),
         "Name of the input channel")
        (opt::OutputChannelName.data(),
         bpo::value<std::string>()->default_value("out"),
         "Name of the output channel")
        (opt::DQMChannelName.data(),
         bpo::value<std::string>()->default_value("dqm"),
         "Name of the data quality monitoring channel")
        (opt::PollTimeout.data(),
         bpo::value<std::string>()->default_value("1"),
         "Timeout of polling (in msec)")
        (opt::SplitMethod.data(),
         bpo::value<std::string>()->default_value("1"),
         "STF split method");

        // 20260312
        ( "CutThreshold",                                    // option name 
                bpo::value<std::string>()->default_value("90.0"),  // type, default value
                "Threshold value for TOT cut"                      // explanation
                );
}

std::unique_ptr<fair::mq::Device> getDevice(fair::mq::ProgOptions& /*config*/)
{
    return std::make_unique<FilterTimeFrameSliceByTOT>();
}
