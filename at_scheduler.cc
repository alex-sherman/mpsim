#include "scheduler.h"
#include <algorithm>

void ATScheduler::SchedulePacket(Ptr<Packet> packet) {
    tunnelApp->TunSendIfe(packet, 0);
}

UnackPacket ATScheduler::OnAck(TunHeader ackHeader, bool &loss) {
    UnackPacket pkt = MPScheduler::OnAck(ackHeader, loss);
    NS_LOG_UNCOND("Time " << ackHeader.queueing_time);
    if(!loss) {

    }
    return pkt;
}
