#include "scheduler.h"
#include <algorithm>

void ATScheduler::Init(uint numPaths, TunnelApp *tunnelApp) {
    MPScheduler::Init(numPaths, tunnelApp);
    for(uint i = 0; i < numPaths; i++) {
        path_queueing_times.push_back(0);
        path_arrival_times.push_back(0);
    }
}

void ATScheduler::SchedulePacket(Ptr<Packet> packet) {
    uint path = -1;
    float min_arrival_time = -1;
    for(uint i = 0; i < path_queueing_times.size(); i++) {
        float at = ArrivalTime(i);
        if(at < min_arrival_time || min_arrival_time == -1) {
            path = i;
            min_arrival_time = at;
        }
    }
    tunnelApp->TunSendIfe(packet, path);
}

float ATScheduler::QueueTime(uint path) {
    float qt = 0;
    for(auto &unack_pkt : unack[path]) {
        //NS_LOG_UNCOND("PKT IDLE TIME: " << path_queueing_times[path] << " " << " " << unack_pkt.idle_time << path);
        qt += path_queueing_times[path];
    }
    return qt;
}

void ATScheduler::OnSend(Ptr<Packet> packet, TunHeader &header) {
    MPScheduler::OnSend(packet, header);
    header.time = ATScheduler::ArrivalTime(header.path);
}

UnackPacket ATScheduler::OnAck(TunHeader ackHeader, bool &loss) {
    //NS_LOG_UNCOND("Before " << ArrivalTime(ackHeader.path) << " " << (int)ackHeader.path);
    UnackPacket pkt = MPScheduler::OnAck(ackHeader, loss);
    path_arrival_times[ackHeader.path] = ackHeader.arrival_time;
    path_queueing_times[ackHeader.path] = ackHeader.queueing_time;
    if(!loss) {
    }
    else {
        //path_queueing_times[ackHeader.path] *= 2;
        NS_LOG_UNCOND("Loss " << (int)ackHeader.path);
    }
    return pkt;
}

float ATScheduler::ArrivalTime(uint path) {
    float at = path_arrival_times[path];
    at += QueueTime(path);
    //at += path_queueing_times[path];
    //NS_LOG_UNCOND("AT: " << at << " " << path_queueing_times[path] << " " << path);
    return fmax(at, delays[path] + Simulator::Now().GetSeconds());
}