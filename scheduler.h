#ifndef MPSIM_SCHEDULER_H
#define MPSIM_SCHEDULER_H

#include "ns3/ptr.h"
#include "ns3/packet.h"
#include "ns3/header.h"
#include "tunnel_header.h"
#include <vector>

using namespace ns3;
using namespace std;

class MPScheduler {
public:
    virtual void Init(uint numPaths) = 0;
    virtual int SchedulePacket(Ptr<Packet> packet) = 0;
    virtual void OnSend(Ptr<Packet> packet, TunHeader header) = 0;
    virtual void OnAck(TunHeader ackHeader) = 0;
};

class UnackPacket {
public:
    UnackPacket(uint32_t path_seq, uint32_t size) : path_seq(path_seq), size(size) { }
    uint32_t path_seq;
    uint32_t size;
};

class CWNDScheduler : public MPScheduler {
public:
    void Init(uint numPaths);
    int SchedulePacket(Ptr<Packet> packet);
    void OnSend(Ptr<Packet> packet, TunHeader header);
    void OnAck(TunHeader ackHeader);
    vector<double> cwnd;
    vector<vector<UnackPacket>> unack;
private:
    uint UnackSize(uint path);
};

#endif