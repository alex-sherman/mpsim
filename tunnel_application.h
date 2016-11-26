#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include <vector>

using namespace ns3;
using namespace std;

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
  void Setup (Ptr<Node> node, Ipv4Address address);
  void SendPacket (void);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);

  Ptr<Node>       m_node;
  vector<Ptr<Socket>> m_sockets;
  Ipv4Address     m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
};