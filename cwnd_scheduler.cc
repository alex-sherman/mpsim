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
bool CWNDScheduler::TrySendPacket(Ptr<Packet> packet) {
    for(uint i = 0; i < cwnd.size(); i++) {
        if(UnackSize(i) + packet->GetSize() < cwnd[i]) {
            tunnelApp->TunSendIfe(packet, i);
            return true;
        }
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
    // Attempt to send any queued packets
    while(m_packet_queue.size() > 0 && TrySendPacket(m_packet_queue[0]))
        m_packet_queue.erase(m_packet_queue.begin());
    //NS_LOG_UNCOND("Path: " << (int)ackHeader.path << " cwnd: " << cwnd[ackHeader.path] << " unack: " << UnackSize(ackHeader.path));
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