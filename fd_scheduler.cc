#include "scheduler.h"
#include <cmath>

#include "ns3/core-module.h"

bool FDBSScheduler::TrySendPacket(Ptr<Packet> packet) {
    return CWNDScheduler::TrySendPacket(packet);
}

void FDBSScheduler::ServiceQueue() {
    uint pi = MinDelay();
    double bw = rates[pi].GetBitRate() / (1300 * 8);
    double minDelay = delays[pi];
    while(m_packet_queue.size() > 0) {
        uint i = 0;
        for(; i < cwnd.size(); i++) {
            if(PathAvailable(i))
                break;
        }
        if(i == cwnd.size())
            return;
        uint qi = fmin(m_packet_queue.size() - 1, (delays[i] - minDelay) * bw);
        //if(qi == m_packet_queue.size() - 1)
        //    NS_LOG_UNCOND("Queue not full enough");
        TunHeader tunHeader;
        m_packet_queue[qi]->PeekHeader(tunHeader);
        tunnelApp->TunSendIfe(m_packet_queue[qi], i);
        m_packet_queue.erase(m_packet_queue.begin() + qi);
    }
}