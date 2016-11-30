#include "scheduler.h"
#include <algorithm>

void EDPFScheduler::Init(uint numPaths, TunnelApp *tunnelApp) {
    this->tunnelApp = tunnelApp;

    for(uint i=0; i<numPaths; i++) {

        //TODO: get real values
        bandwidths.push_back(DataRate("2Mbps"));
        delay.push_back(0.005);
        available.push_back(Time(0.0));
    }
}


void EDPFScheduler::SchedulePacket(Ptr<Packet> packet) {
    
    int path = -1;
    int min_arrival = -1;

    for (uint i=0; i<delay.size(); i++) {
        double tmp_time = arrival_time(Simulator::Now().GetSeconds(), i, packet->GetSize());
        if (path == -1 || tmp_time < min_arrival) {
            path = i;
            min_arrival = tmp_time;
        }
    }

    if (path != -1) {
        tunnelApp->TunSendIfe(packet, path);
    }

    //TODO what if link not ready for send?
}


void EDPFScheduler::OnAck(TunHeader ackHeader) {
}


void EDPFScheduler::OnSend(Ptr<Packet> packet, TunHeader header) {

    //update available time of link
    if ( Simulator::Now() > available[header.path] ) {
        // currently available
        available[header.path] = Simulator::Now() + bandwidths[header.path].CalculateBytesTxTime(packet->GetSize());
    } else {
        // notyet available
        available[header.path] = bandwidths[header.path].CalculateBytesTxTime(packet->GetSize());
    }
}


double EDPFScheduler::arrival_time(double packet_arrival, int path, int packet_size) {
    
    double transmit_time = bandwidths[path].CalculateBytesTxTime(packet_size).GetSeconds();
    // NS_LOG_UNCOND("transmit time:  " << transmit_time);

    return max(packet_arrival + delay[path], available[path].GetSeconds()) + transmit_time;
}

