#include "scheduler.h"
#include <cmath>

#include "ns3/core-module.h"

void CWNDScheduler::Init(uint numPaths, TunnelApp *tunnelApp) {
    MPScheduler::Init(numPaths, tunnelApp);
    for(uint i = 0; i < numPaths; i++) {
        cwnd.push_back(500);//fmax(500, rates[i].GetBitRate() / 8 * delays[i] * 2));
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
    return UnackSize(index) < cwnd[index];
}

bool CWNDScheduler::TrySendPacket(Ptr<Packet> packet) {
    double maxUnackDiff = 2;
    int path = -1;
    double curUnackDiff;
    for(uint i = 0; i < cwnd.size(); i++) {
        curUnackDiff = UnackSize(i) * 1.0 / cwnd[i];
        //if(!tunnelApp->m_log_packets)
        //    NS_LOG_UNCOND("unackdiff: " << i << " " << curUnackDiff << " " << maxUnackDiff);
        if(!PathAvailable(i)) {
            continue;
        }
        if(curUnackDiff < maxUnackDiff) {
            path = i;
            maxUnackDiff = curUnackDiff;
        }
    }
    if(path != -1) {
        //if(!tunnelApp->m_log_packets)
        //    NS_LOG_UNCOND("CHOOSE: " << path);
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


UnackPacket CWNDScheduler::OnAck(TunHeader ackHeader, bool &loss) {
    UnackPacket pkt = MPScheduler::OnAck(ackHeader, loss);
    delays[ackHeader.path] = ackHeader.time;
    //if(!tunnelApp->m_log_packets)
    //    NS_LOG_UNCOND("CWND: " << (int)ackHeader.path << " " << cwnd[ackHeader.path] << " " << UnackSize(ackHeader.path));
    if(loss) {
        cwnd[ackHeader.path] = fmax(500, cwnd[ackHeader.path] * 0.5);
        if(!tunnelApp->m_log_packets)
            NS_LOG_UNCOND("LOSS " << (int)ackHeader.path << " " << cwnd[ackHeader.path]);
    }
    else {
        cwnd[ackHeader.path] += pkt.size * pkt.size / cwnd[ackHeader.path];
        //NS_LOG_UNCOND((int)ackHeader.path << " " << cwnd[ackHeader.path]);
    }
    ServiceQueue();
    return pkt;
    //NS_LOG_UNCOND("Path: " << (int)ackHeader.path << " cwnd: " << cwnd[ackHeader.path] << " unack: " << UnackSize(ackHeader.path));
}
void CWNDScheduler::ServiceQueue() {
    // Attempt to send any queued packets
    while(m_packet_queue.size() > 0 && TrySendPacket(m_packet_queue[0]))
        m_packet_queue.erase(m_packet_queue.begin());
}
