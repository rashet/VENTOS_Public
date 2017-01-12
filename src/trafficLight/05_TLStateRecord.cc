/****************************************************************************/
/// @file    TLStateRecord.cc
/// @author  Mani Amoozadeh <maniam@ucdavis.edu>
/// @author
/// @date    April 2015
///
/****************************************************************************/
// VENTOS, Vehicular Network Open Simulator; see http:?
// Copyright (C) 2013-2015
/****************************************************************************/
//
// This file is part of VENTOS.
// VENTOS is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#include <iomanip>

#include "trafficLight/05_TLStateRecord.h"

namespace VENTOS {

Define_Module(VENTOS::TLStateRecord);

TLStateRecord::~TLStateRecord()
{

}


void TLStateRecord::initialize(int stage)
{
    super::initialize(stage);

    if(stage == 0)
    {
        minGreenTime = par("minGreenTime").doubleValue();
        maxGreenTime = par("maxGreenTime").doubleValue();
        yellowTime = par("yellowTime").doubleValue();
        redTime = par("redTime").doubleValue();
        maxCycleLength = par("maxCycleLength").doubleValue();

        if(minGreenTime <= 0)
            throw omnetpp::cRuntimeError("minGreenTime value is wrong!");
        if(maxGreenTime <= 0 || maxGreenTime < minGreenTime)
            throw omnetpp::cRuntimeError("maxGreenTime value is wrong!");
        if(yellowTime <= 0)
            throw omnetpp::cRuntimeError("yellowTime value is wrong!");
        if(redTime <= 0)
            throw omnetpp::cRuntimeError("redTime value is wrong!");
        if( maxCycleLength < (minGreenTime + yellowTime + redTime) )
            throw omnetpp::cRuntimeError("maxCycleLength value is wrong!");

        collectTLPhasingData = par("collectTLPhasingData").boolValue();

        phaseTL.clear();
        statusTL.clear();
    }
}


void TLStateRecord::finish()
{
    super::finish();

    if(collectTLPhasingData)
        saveTLPhasingData();
}


void TLStateRecord::handleMessage(omnetpp::cMessage *msg)
{
    super::handleMessage(msg);
}


void TLStateRecord::initialize_withTraCI()
{
    super::initialize_withTraCI();

}


void TLStateRecord::executeEachTimeStep()
{
    super::executeEachTimeStep();

}


void TLStateRecord::updateTLstate(std::string TLid, std::string stage, std::string currentInterval, bool cycleStart)
{
    if(stage == "init")
    {
        // initialize phase number in this TL
        phaseTL[TLid] = 1;

        // Initialize status in this TL
        currentStatusTL *entry = new currentStatusTL(1 /*cycle number*/, currentInterval, -1, omnetpp::simTime().dbl(), -1, -1, -1, laneListTL[TLid].first, -1);
        statusTL.insert( std::make_pair(std::make_pair(TLid,1), *entry) );
    }
    else
    {
        // get a reference to this TL
        auto location = statusTL.find( std::make_pair(TLid,phaseTL[TLid]) );
        if(location == statusTL.end())
            throw omnetpp::cRuntimeError("This TLid is not found!");

        if(stage == "yellow")
        {
            (location->second).yellowStart = omnetpp::simTime().dbl();
            (location->second).greenLength = (location->second).yellowStart - (location->second).greenStart;

            // get green duration
            double green_duration = (location->second).yellowStart - (location->second).greenStart;

            //            // todo: make sure green interval is above G_min
            //            if(green_duration - minGreenTime < 0)
            //                throw omnetpp::cRuntimeError("green interval is less than minGreenTime = %0.3f", minGreenTime);
            //
            //            // make sure green interval is below G_max
            //            if(green_duration - maxGreenTime > 0)
            //                throw omnetpp::cRuntimeError("green interval is greater than maxGreenTime = %0.3f", maxGreenTime);
        }
        else if(stage == "red")
        {
            (location->second).redStart = omnetpp::simTime().dbl();

            // get yellow duration
            double yellow_duration = (location->second).redStart - (location->second).yellowStart;

            // todo:
            //            if( fabs(yellow_duration - yellowTime) < 0.0001 )
            //                throw omnetpp::cRuntimeError("yellow interval is not %0.3f", yellowTime);
        }
        else if(stage == "phaseEnd")
        {
            // update TL status for this phase
            (location->second).phaseEnd = omnetpp::simTime().dbl();

            // get red duration
            double red_duration = (location->second).phaseEnd - (location->second).redStart;

            // todo:
            //            if(red_duration - redTime != 0)
            //                throw omnetpp::cRuntimeError("red interval is not %0.3f", redTime);

            // get all incoming lanes for this TLid
            std::list<std::string> lan = laneListTL[TLid].second;

            // for each incoming lane
            int totalQueueSize = 0;
            for(auto &it2 : lan)
            {
                totalQueueSize = totalQueueSize + laneQueueSize[it2].second;
            }

            (location->second).totalQueueSize = totalQueueSize;

            // get the current cycle number
            int cycleNumber = (location->second).cycle;

            // on a new cycle
            if(cycleStart)
            {
                // todo: check if cycle length is ok?

                // increase cycle number by 1
                cycleNumber++;

                // update traffic demand at the beginning of each cycle
                updateTrafficDemand();
            }

            // increase phase number by 1
            auto location2 = phaseTL.find(TLid);
            location2->second = location2->second + 1;

            // update status for the new phase
            currentStatusTL *entry = new currentStatusTL(cycleNumber, currentInterval, -1, omnetpp::simTime().dbl(), -1, -1, -1, lan.size(), -1);
            statusTL.insert( std::make_pair(std::make_pair(TLid,location2->second), *entry) );
        }
        else throw omnetpp::cRuntimeError("stage is not recognized!");
    }
}


void TLStateRecord::saveTLPhasingData()
{
    if(statusTL.empty())
        return;

    int currentRun = omnetpp::getEnvir()->getConfigEx()->getActiveRunNumber();

    std::ostringstream fileName;
    fileName << boost::format("%03d_TLphasingData.txt") % currentRun;

    boost::filesystem::path filePath ("results");
    filePath /= fileName.str();

    FILE *filePtr = fopen (filePath.c_str(), "w");
    if (!filePtr)
        throw omnetpp::cRuntimeError("Cannot create file '%s'", filePath.c_str());

    // write simulation parameters at the beginning of the file in CMD mode
    if(!omnetpp::cSimulation::getActiveEnvir()->isGUI())
    {
        // get the current config name
        std::string configName = omnetpp::getEnvir()->getConfigEx()->getVariable("configname");

        // get number of total runs in this config
        int totalRun = omnetpp::getEnvir()->getConfigEx()->getNumRunsInConfig(configName.c_str());

        // get the current run number
        int currentRun = omnetpp::getEnvir()->getConfigEx()->getActiveRunNumber();

        // get all iteration variables
        std::vector<std::string> iterVar = omnetpp::getEnvir()->getConfigEx()->unrollConfig(configName.c_str(), false);

        // write to file
        fprintf (filePtr, "configName      %s\n", configName.c_str());
        fprintf (filePtr, "totalRun        %d\n", totalRun);
        fprintf (filePtr, "currentRun      %d\n", currentRun);
        fprintf (filePtr, "currentConfig   %s\n\n\n", iterVar[currentRun].c_str());
    }

    // write header
    fprintf (filePtr, "%-12s", "TLid");
    fprintf (filePtr, "%-12s", "phase");
    fprintf (filePtr, "%-12s", "cycle");
    fprintf (filePtr, "%-35s", "allowedMovements");
    fprintf (filePtr, "%-15s", "greenLength");
    fprintf (filePtr, "%-15s", "greenStart");
    fprintf (filePtr, "%-15s", "yellowStart");
    fprintf (filePtr, "%-15s", "redStart");
    fprintf (filePtr, "%-15s", "phaseEnd");
    fprintf (filePtr, "%-15s", "#lanes");
    fprintf (filePtr, "%-15s\n\n", "totalqueueSize");

    // write body
    for(auto & y : statusTL)
    {
        currentStatusTL status = y.second;

        fprintf (filePtr, "%-12s", y.first.first.c_str());
        fprintf (filePtr, "%-12d", y.first.second);
        fprintf (filePtr, "%-12d", status.cycle);
        fprintf (filePtr, "%-35s", status.allowedMovements.c_str());
        fprintf (filePtr, "%-15.2f", status.greenLength);
        fprintf (filePtr, "%-15.2f", status.greenStart);
        fprintf (filePtr, "%-15.2f", status.yellowStart);
        fprintf (filePtr, "%-15.2f", status.redStart);
        fprintf (filePtr, "%-15.2f", status.phaseEnd);
        fprintf (filePtr, "%-15d", status.incommingLanes);
        fprintf (filePtr, "%-15d\n", status.totalQueueSize);
    }

    fclose(filePtr);
}

}
