#include "scheduler.h"
#include <algorithm>

EDPFScheduler::EDPFScheduler(vector<Ptr<Queue>> queues) :
    tx_queues(queues)
{
}

void EDPFScheduler::Init(uint numPaths, TunnelApp *tunnelApp) {
    this->tunnelApp = tunnelApp;

    //TODO: get real values
    bandwidths.push_back(DataRate("0.1Mbps"));
    delay.push_back(0.5);
    // available.push_back(Time(0.0));

    bandwidths.push_back(DataRate("0.2Mbps"));
    delay.push_back(0.5);
    // available.push_back(Time(0.0));

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

    if (path != -1 ) {
        tunnelApp->TunSendIfe(packet, path);
    }
}


void EDPFScheduler::OnAck(TunHeader ackHeader) {
    // shouldnt matter
}

void EDPFScheduler::OnSend(Ptr<Packet> packet, TunHeader header) {

    // //update available time of link
    // if ( Simulator::Now() > available[header.path] ) {
    //     // currently available
    //     available[header.path] = Simulator::Now() + bandwidths[header.path].CalculateBytesTxTime(packet->GetSize());
    // } else {
    //     // not yet available
    //     available[header.path] = bandwidths[header.path].CalculateBytesTxTime(packet->GetSize());
    // }

    // NS_LOG_UNCOND("Next available:  " << available[header.path].GetSeconds() << "  @" << Simulator::Now().GetSeconds());
}


double EDPFScheduler::arrival_time(double packet_arrival, int path, int packet_size) {
    
    double transmit_time = bandwidths[path].CalculateBytesTxTime(packet_size).GetSeconds();
    // NS_LOG_UNCOND("transmit time:  " << transmit_time << "  (" << packet_size << " bytes)");

    return max(packet_arrival + delay[path], next_available(path).GetSeconds()) + transmit_time;
    // return max(packet_arrival, available[path].GetSeconds()) + transmit_time;
}

Time EDPFScheduler::next_available(int path) {

    return Simulator::Now() + bandwidths[path].CalculateBytesTxTime(tx_queues[path]->GetNBytes());
}
