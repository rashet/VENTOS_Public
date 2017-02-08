/****************************************************************************/
/// @file    Base.cc
/// @author  Mani Amoozadeh <maniam@ucdavis.edu>
/// @author  second author name
/// @date    August 2013
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

#include "rsu/01_Beacon.h"

namespace VENTOS {

Define_Module(VENTOS::ApplRSUBeacon);

ApplRSUBeacon::~ApplRSUBeacon()
{

}


void ApplRSUBeacon::initialize(int stage)
{
    super::initialize(stage);

    if (stage==0)
    {
        // get a pointer to the TrafficLight module
        TLptr = omnetpp::getSimulation()->getSystemModule()->getSubmodule("TrafficLight");
        ASSERT(TLptr);

        TLControlMode = TLptr->par("TLControlMode").longValue();
        minGreenTime = TLptr->par("minGreenTime").doubleValue();

        signalBeaconing = par("signalBeaconing").boolValue();

        myTLid = getParentModule()->par("myTLid").stringValue();

        Coord rsu_pos_omnet = Coord(this->getParentModule()->getSubmodule("mobility")->par("x").doubleValue(), this->getParentModule()->getSubmodule("mobility")->par("y").doubleValue());
        rsu_pos = TraCI->omnet2traciCoord(rsu_pos_omnet);
    }
}


void ApplRSUBeacon::finish()
{
    super::finish();
}


void ApplRSUBeacon::handleSelfMsg(omnetpp::cMessage* msg)
{
    super::handleSelfMsg(msg);
}


void ApplRSUBeacon::executeEachTimeStep()
{

}


void ApplRSUBeacon::sendBeacon()
{
    BeaconRSU* beaconMsg = generateBeacon();

    // send the beacon as a signal. Any module registered to this signal can
    // receive a copy of the beacon (for now, only RSUs are registered)
    if(signalBeaconing)
    {
        omnetpp::simsignal_t Signal_beaconSignaling = registerSignal("beaconSignaling");
        this->getParentModule()->emit(Signal_beaconSignaling, beaconMsg);
    }
    // broadcast the beacon wirelessly using IEEE 802.11p
    else
        send(beaconMsg, lowerLayerOut);
}


BeaconRSU* ApplRSUBeacon::generateBeacon()
{
    BeaconRSU* wsm = new BeaconRSU("beaconRSU", TYPE_BEACON_RSU);

    // add header length
    wsm->addBitLength(headerLength);

    // add payload length
    wsm->addBitLength(beaconLengthBits);

    wsm->setWsmVersion(1);
    wsm->setSecurityType(1);

    wsm->setChannelNumber(Veins::Channels::CCH);

    wsm->setDataRate(1);
    wsm->setPriority(beaconPriority);
    wsm->setPsid(0);

    // wsm->setSerial(serial);
    // wsm->setTimestamp(simTime());

    // fill in the sender/receiver fields
    wsm->setSender(myFullId);
    wsm->setSenderType("notSpecified");
    wsm->setRecipient("broadcast");

    // set my current position
    wsm->setPos(rsu_pos);

    return wsm;
}

}