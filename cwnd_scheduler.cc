#include "scheduler.h"
#include <cmath>

#include "ns3/core-module.h"

void CWNDScheduler::Init(uint numPaths, TunnelApp *tunnelApp) {
    this->tunnelApp = tunnelApp;
    for(uint i = 0; i < numPaths; i++) {
        cwnd.push_back(3000);
        unack.push_back(vector<UnackPacket>());
    }
}
bool CWNDScheduler::PathAvailable(uint index) {
    return UnackSize(index) + 1368 < cwnd[index];
}

bool CWNDScheduler::TrySendPacket(Ptr<Packet> packet) {
    int delays[] = {400, 500};
    int minDelay = -1;
    int path = -1;
    for(uint i = 0; i < cwnd.size(); i++) {
        if(PathAvailable(i) && (minDelay == -1 || delays[i] < minDelay)) {
            path = i;
            minDelay = delays[i];
        }
    }
    if(path != -1) {
        tunnelApp->TunSendIfe(packet, path);
        return true;
    }
    return false;
}
void CWNDScheduler::SchedulePacket(Ptr<Packet> packet) {
    if(m_packet_queue.size() > 0 || !TrySendPacket(packet)) {
        // If no interface can be found with capacity, queue the packet
        m_packet_queue.push_back(packet);
    }
}
void CWNDScheduler::OnAck(TunHeader ackHeader) {
    bool loss = false;
    uint size = 0;
    while(unack[ackHeader.path].size() > 0 && unack[ackHeader.path][0].path_seq <= ackHeader.path_seq) {
        if(unack[ackHeader.path][0].path_seq == ackHeader.path_seq)
            size = unack[ackHeader.path][0].size;

        if(unack[ackHeader.path][0].path_seq != ackHeader.path_seq)
            loss = true;

        unack[ackHeader.path].erase(unack[ackHeader.path].begin());
    }
    if(loss) {
        cwnd[ackHeader.path] = fmax(3000, cwnd[ackHeader.path] / 2);
        NS_LOG_UNCOND("LOSS");
    }
    else
        cwnd[ackHeader.path] += size * size / cwnd[ackHeader.path];
    ServiceQueue();
    //NS_LOG_UNCOND("Path: " << (int)ackHeader.path << " cwnd: " << cwnd[ackHeader.path] << " unack: " << UnackSize(ackHeader.path));
}
void CWNDScheduler::ServiceQueue() {
    // Attempt to send any queued packets
    while(m_packet_queue.size() > 0 && TrySendPacket(m_packet_queue[0]))
        m_packet_queue.erase(m_packet_queue.begin());
}
void CWNDScheduler::OnSend(Ptr<Packet> packet, TunHeader header) {
    //NS_LOG_UNCOND(Simulator::Now ().GetSeconds () << "Send on " << header << " what " << unack[header.path].size());
    unack[header.path].push_back(UnackPacket(header.path_seq, packet->GetSize()));
}
uint CWNDScheduler::UnackSize(uint path) {
    uint size = 0;
    for(UnackPacket p : unack[path]) {
        size += p.size;
    }
    return size;
}

bool FDBSScheduler::TrySendPacket(Ptr<Packet> packet) {
    return CWNDScheduler::TrySendPacket(packet);
}

void FDBSScheduler::ServiceQueue() {
    NS_LOG_UNCOND("queue size: " << m_packet_queue.size());
    double delays[] = {0.012, 0.012};
    double bws[] = {9.6,19.2};
    while(m_packet_queue.size() > 0) {
        uint i = 0;
        for(; i < cwnd.size(); i++) {
            if(PathAvailable(i))
                break;
        }
        if(i == cwnd.size())
            return;
        uint qi = fmin(m_packet_queue.size() - 1, (delays[i] - delays[0]) * bws[i]);
        if(qi == m_packet_queue.size() - 1)
            NS_LOG_UNCOND("Queue not full enough");
        TunHeader tunHeader;
        m_packet_queue[qi]->PeekHeader(tunHeader);
        NS_LOG_UNCOND("Sending "<<tunHeader<< " "<<qi);
        tunnelApp->TunSendIfe(m_packet_queue[qi], i);
        m_packet_queue.erase(m_packet_queue.begin() + qi);
    }
}