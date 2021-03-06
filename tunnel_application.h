#ifndef MPSIM_TUN_APPLICATION_H
#define MPSIM_TUN_APPLICATION_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/virtual-net-device.h"
#include "scheduler.h"
#include <list>

using namespace ns3;
using namespace std;

class MPScheduler;
class TunnelApp : public Application
{
public:
  TunnelApp ();
  virtual ~TunnelApp ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  void Setup (MPScheduler *scheduler, Ptr<Node> node, Ipv4Address tun_address, Ipv4Address remote_address, bool log_packets = false);
  bool OnTunSend(Ptr<Packet> packet, const Address &src, const Address &dst, uint16_t proto);
  void TunSendIfe(Ptr<Packet> packet, uint interface);
  void OnPacketRecv(Ptr<Socket> socket);
  void ServiceReorderBuffer();
  void DeliverPacket(Ptr<Packet> packet, Ptr<VirtualNetDevice> m_tun_device);

  double reorder_latency = 0.08;
  bool            m_log_packets;

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  std::ofstream   stream;
  std::ofstream   stream2;
  uint32_t        m_seq;
  MPScheduler *   m_scheduler;
  Ptr<Node>       m_node;
  Ptr<VirtualNetDevice> m_tun_device;
  vector<Ptr<Socket>> m_sockets;
  vector<uint32_t> m_path_ack;
  Ipv4Address     m_tun_address;
  Ipv4Address     m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
  uint            m_last_sent_seq = 1;
  list<Ptr<Packet>> m_reorder_buffer;
};

#endif