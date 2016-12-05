#include "scheduler.h"
#include <cmath>

#include "ns3/core-module.h"

void CWNDScheduler::Init(uint numPaths, TunnelApp *tunnelApp) {
    this->tunnelApp = tunnelApp;
    for(uint i = 0; i < numPaths; i++) {
        cwnd.push_back(fmax(3000, rates[i].GetBitRate() / 8 * delays[i] * 2));
        unack.push_back(vector<UnackPacket>());
    }
}

uint CWNDScheduler::MinDelay() {
    double m = delays[0];
    uint io = 0;
    for(uint i = 0; i < delays.size(); i++) {
        if(m > delays[i]) {
            m = delays[i];
            io = i;
        }
    }
    return io;
}

bool CWNDScheduler::PathAvailable(uint index) {
    return UnackSize(index) + 1368 < cwnd[index];
}

bool CWNDScheduler::TrySendPacket(Ptr<Packet> packet) {
    int minDelay = -1;
    int path = -1;
    for(uint i = 0; i < cwnd.size(); i++) {
        if(PathAvailable(i) && (minDelay == -1 || delays[i] < minDelay)) {
            path = i;
            minDelay = delays[i];
        }
    }
    if(path != -1) {
        //NS_LOG_UNCOND("Queue: " << tx_queues[path]->GetNBytes() << " " << path);
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

    delays[ackHeader.path] = ackHeader.time;
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
        cwnd[ackHeader.path] = fmax(3000, cwnd[ackHeader.path] * 0.5);
        NS_LOG_UNCOND("LOSS " << (int)ackHeader.path);
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
