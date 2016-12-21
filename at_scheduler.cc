#include "scheduler.h"
#include <algorithm>

void ATScheduler::Init(uint numPaths, TunnelApp *tunnelApp) {
    MPScheduler::Init(numPaths, tunnelApp);
    for(uint i = 0; i < numPaths; i++) {
        NS_LOG_UNCOND(i << " " << rates[i] << " " << rates[i].CalculateBytesTxTime(1300).GetSeconds());
        path_queueing_times.push_back(rates[i].CalculateBytesTxTime(1300).GetSeconds());
        path_arrival_times.push_back(delays[i]);
    }
}

void ATScheduler::SchedulePacket(Ptr<Packet> packet) {
    uint path = -1;
    float min_arrival_time = -1;
    for(uint i = 0; i < path_queueing_times.size(); i++) {
        float at = ArrivalTime(i);
        //NS_LOG_UNCOND("Path " << i << " " << at << " " << path_queueing_times[i] << " " << path_arrival_times[i]);
        if(at < min_arrival_time || min_arrival_time == -1) {
            path = i;
            min_arrival_time = at;
        }
    }
    tunnelApp->TunSendIfe(packet, path);
}

void ATScheduler::OnSend(Ptr<Packet> packet, TunHeader &header) {
    MPScheduler::OnSend(packet, header);
    header.time = ATScheduler::ArrivalTime(header.path);
}

UnackPacket ATScheduler::OnAck(TunHeader ackHeader, bool &loss) {
    //NS_LOG_UNCOND("Before " << ArrivalTime(ackHeader.path) << " " << (int)ackHeader.path);
    UnackPacket pkt = MPScheduler::OnAck(ackHeader, loss);
    path_arrival_times[ackHeader.path] = ackHeader.arrival_time;
    //path_queueing_times[ackHeader.path] = ackHeader.queueing_time;
    if(!loss) {
    }
    else {
        //path_queueing_times[ackHeader.path] *= 2;
        NS_LOG_UNCOND("Loss " << (int)ackHeader.path);
    }
    return pkt;
}

float ATScheduler::ArrivalTime(uint path, uint packet) {
    float at_previous = packet == 0 ? path_arrival_times[path] : ArrivalTime(path, packet - 1);
    //at += path_queueing_times[path];
    //NS_LOG_UNCOND("AT: " << at << " " << path_queueing_times[path] << " " << path);
    float delivery_time = packet == unack[path].size() ? Simulator::Now().GetSeconds() : unack[path][packet].send_time;
    return fmax(at_previous, delays[path] + delivery_time) + path_queueing_times[path];
}

float ATScheduler::ArrivalTime(uint path) {
    return ArrivalTime(path, unack[path].size());
}