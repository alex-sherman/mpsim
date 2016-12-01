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
class MPScheduler {
public:
    MPScheduler(vector<DataRate> rates, vector<double> delays, vector<Ptr<Queue>> queues) :
        rates(rates), delays(delays), tx_queues(queues) { }
    virtual void Init(uint numPaths, TunnelApp *tunnelApp) = 0;
    virtual void SchedulePacket(Ptr<Packet> packet) = 0;
    virtual void OnSend(Ptr<Packet> packet, TunHeader header) = 0;
    virtual void OnAck(TunHeader ackHeader) = 0;
    vector<DataRate> rates;
    vector<double> delays;
    vector<Ptr<Queue>> tx_queues;
};

class UnackPacket {
public:
    UnackPacket(uint32_t path_seq, uint32_t size) : path_seq(path_seq), size(size) { }
    uint32_t path_seq;
    uint32_t size;
};

class CWNDScheduler : public MPScheduler {
public:
    CWNDScheduler(vector<DataRate> rates, vector<double> delays, vector<Ptr<Queue>> queues) :
        MPScheduler(rates, delays, queues) { }
    virtual void Init(uint numPaths, TunnelApp *tunnelApp);
    bool TrySendPacket(Ptr<Packet> packet);
    void SchedulePacket(Ptr<Packet> packet);
    void OnSend(Ptr<Packet> packet, TunHeader header);
    virtual void OnAck(TunHeader ackHeader);
    bool PathAvailable(uint index);
    virtual void ServiceQueue();
    vector<double> cwnd;
    vector<vector<UnackPacket>> unack;
protected:
    uint UnackSize(uint path);
    TunnelApp *tunnelApp;
    vector<Ptr<Packet>> m_packet_queue;
};

class FDBSScheduler : public CWNDScheduler {
public:
    FDBSScheduler(vector<DataRate> rates, vector<double> delays, vector<Ptr<Queue>> queues) :
        CWNDScheduler(rates, delays, queues) { }
    void Init(uint numPaths, TunnelApp *tunnelApp);
    bool TrySendPacket(Ptr<Packet> packet);
    void ServiceQueue();
    void OnAck(TunHeader ackHeader);
};

class EDPFScheduler : public MPScheduler {
 private:
    TunnelApp *tunnelApp;
    /* vector<Time> available; */

 public:
    EDPFScheduler(vector<DataRate> rates, vector<double> delays, vector<Ptr<Queue>> queues) :
        MPScheduler(rates, delays, queues) { }
    void Init(uint numPaths, TunnelApp *tunnelApp);
    void SchedulePacket(Ptr<Packet> packet);
    void OnSend(Ptr<Packet> packet, TunHeader header);
    void OnAck(TunHeader ackHeader);

 private:
    double arrival_time(double packet_arrival, int path, int packet_size);
    int TrySendPacket(Ptr<Packet> packet);
    Time next_available(int link);
};

#endif
