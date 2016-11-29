#include "scheduler.h"
#include <cmath>

#include "ns3/core-module.h"

double vectorMin(vector<double> delays) {
    double m = delays[0];
    for(uint i = 0; i < delays.size(); i++) {
        m = fmin(delays[i], m);
    }
    return m;
}

void FDBSScheduler::Init(uint numPaths, TunnelApp *tunnelApp) {
    CWNDScheduler::Init(numPaths, tunnelApp);
    for(uint i = 0; i < numPaths; i++) {
        delays.push_back(0);
    }
}

void FDBSScheduler::OnAck(TunHeader ack) {
    delays[ack.path] = ack.time;
    CWNDScheduler::OnAck(ack);
}

bool FDBSScheduler::TrySendPacket(Ptr<Packet> packet) {
    return CWNDScheduler::TrySendPacket(packet);
}

void FDBSScheduler::ServiceQueue() {
    double bws[] = {192,192};
    double minDelay = vectorMin(delays);
    while(m_packet_queue.size() > 0) {
        uint i = 0;
        for(; i < cwnd.size(); i++) {
            if(PathAvailable(i))
                break;
        }
        if(i == cwnd.size())
            return;
        uint qi = fmin(m_packet_queue.size() - 1, (delays[i] - minDelay) * bws[i]);
        if(qi == m_packet_queue.size() - 1)
            NS_LOG_UNCOND("Queue not full enough");
        TunHeader tunHeader;
        m_packet_queue[qi]->PeekHeader(tunHeader);
        tunnelApp->TunSendIfe(m_packet_queue[qi], i);
        m_packet_queue.erase(m_packet_queue.begin() + qi);
    }
}