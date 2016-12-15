#include "scheduler.h"
#include <algorithm>

void EDPFScheduler::SchedulePacket(Ptr<Packet> packet) {
    
    int path = -1;
    double min_arrival = -1;

    for (uint i=0; i<delays.size(); i++) {
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

double EDPFScheduler::arrival_time(double packet_arrival, int path, int packet_size) {
    
    double transmit_time = rates[path].CalculateBytesTxTime(packet_size).GetSeconds();
    // NS_LOG_UNCOND("transmit time:  " << transmit_time << "  (" << packet_size << " bytes)");

    return next_available(path).GetSeconds() + delays[path] + transmit_time;
    // return max(packet_arrival, available[path].GetSeconds()) + transmit_time;
}

Time EDPFScheduler::next_available(int path) {
    return Simulator::Now() + rates[path].CalculateBytesTxTime(tx_queues[path]->GetNBytes());
}
