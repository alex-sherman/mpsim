#ifndef MPSIM_SCHEDULER_H
#define MPSIM_SCHEDULER_H

#include "ns3/ptr.h"
#include "ns3/packet.h"
#include "ns3/header.h"
#include "ns3/core-module.h"
#include "tunnel_header.h"
#include "tunnel_application.h"
#include <vector>

using namespace ns3;
using namespace std;

class TunnelApp;

class UnackPacket {
public:
    UnackPacket(uint32_t path_seq, uint32_t size, float send_time) : path_seq(path_seq), size(size), send_time(send_time) { }
    uint32_t path_seq;
    uint32_t size;
    float send_time;
};

class MPScheduler {
public:
    MPScheduler(vector<DataRate> rates, vector<double> delays, vector<Ptr<Queue>> queues) :
        rates(rates), delays(delays), tx_queues(queues) { }
    virtual void Init(uint numPaths, TunnelApp *tunnelApp) {
        this->tunnelApp = tunnelApp;
        for(uint i = 0; i < numPaths; i++) {
            unack.push_back(vector<UnackPacket>());
            path_send_times.push_back(0);
            path_recv_times.push_back(0);
        }
    }
    virtual void SchedulePacket(Ptr<Packet> packet) = 0;
    virtual void OnSend(Ptr<Packet> packet, TunHeader &header) {
        double now = Simulator::Now().GetSeconds();
        double idle_time = now - path_send_times[header.path];
        header.queueing_time = idle_time;
        unack[header.path].push_back(UnackPacket(header.path_seq, packet->GetSize(), now));
        path_send_times[header.path] = now;
    }
    virtual void OnRecv(TunHeader &header) {
        header.queueing_time = Simulator::Now().GetSeconds() - path_recv_times[header.path];
        path_recv_times[header.path] = Simulator::Now().GetSeconds();
    }
    uint UnackSize(uint path) {
        uint size = 0;
        for(UnackPacket p : unack[path]) {
            size += p.size;
        }
        return size;
    }
    virtual UnackPacket OnAck(TunHeader ackHeader, bool &loss) {
        loss = false;
        UnackPacket unack_packet(0, 0, 0);
        while(unack[ackHeader.path].size() > 0 && unack[ackHeader.path][0].path_seq <= ackHeader.path_seq) {
            if(unack[ackHeader.path][0].path_seq == ackHeader.path_seq)
                unack_packet = unack[ackHeader.path][0];

            if(unack[ackHeader.path][0].path_seq != ackHeader.path_seq)
                loss = true;

            unack[ackHeader.path].erase(unack[ackHeader.path].begin());
        }
        return unack_packet;
    }
    vector<double> path_send_times;
    vector<double> path_recv_times;

    vector<DataRate> rates;
    vector<double> delays;
    vector<Ptr<Queue>> tx_queues;
    vector<vector<UnackPacket>> unack;
    TunnelApp *tunnelApp;
};

class CWNDScheduler : public MPScheduler {
public:
    CWNDScheduler(vector<DataRate> rates, vector<double> delays, vector<Ptr<Queue>> queues) :
        MPScheduler(rates, delays, queues) { }
    virtual void Init(uint numPaths, TunnelApp *tunnelApp);
    bool TrySendPacket(Ptr<Packet> packet);
    void SchedulePacket(Ptr<Packet> packet);
    UnackPacket OnAck(TunHeader ackHeader, bool &loss);
    bool PathAvailable(uint index);
    virtual void ServiceQueue();
    
    vector<double> cwnd;
    uint MinDelay();
protected:
    vector<Ptr<Packet>> m_packet_queue;
};

class FDBSScheduler : public CWNDScheduler {
public:
    FDBSScheduler(vector<DataRate> rates, vector<double> delays, vector<Ptr<Queue>> queues) :
        CWNDScheduler(rates, delays, queues) { }
    bool TrySendPacket(Ptr<Packet> packet);
    void ServiceQueue();
};

class EDPFScheduler : public MPScheduler {
 public:
    EDPFScheduler(vector<DataRate> rates, vector<double> delays, vector<Ptr<Queue>> queues) :
        MPScheduler(rates, delays, queues) { }
    void Init(uint numPaths, TunnelApp *tunnelApp);
    void SchedulePacket(Ptr<Packet> packet);

 private:
    double arrival_time(double packet_arrival, int path, int packet_size);
    int TrySendPacket(Ptr<Packet> packet);
    Time next_available(int link);
};
class ATScheduler : public MPScheduler {
private:
    vector<double> path_queueing_times;
    vector<double> path_arrival_times;
public:
    ATScheduler(vector<DataRate> rates, vector<double> delays, vector<Ptr<Queue>> queues) :
        MPScheduler(rates, delays, queues) { }
    void Init(uint numPaths, TunnelApp *tunnelApp);
    void SchedulePacket(Ptr<Packet> packet);
    UnackPacket OnAck(TunHeader ackHeader, bool &loss);
    float ArrivalTime(uint path);
    float ArrivalTime(uint path, uint packet);
    void OnSend(Ptr<Packet> packet, TunHeader &header);
};

#endif
