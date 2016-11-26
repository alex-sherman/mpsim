
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "tunnel_application.h"

using namespace ns3;

TunnelApp::TunnelApp ()
    : m_node(),
        m_sockets(),
        m_peer(),
        m_packetSize(0),
        m_nPackets(0),
        m_dataRate(0),
        m_sendEvent(),
        m_running(false),
        m_packetsSent(0)
{
}

TunnelApp::~TunnelApp ()
{
}

/* static */
TypeId TunnelApp::GetTypeId (void)
{
    static TypeId tid = TypeId ("TunnelApp")
        .SetParent<Application> ()
        .SetGroupName ("Tutorial")
        .AddConstructor<TunnelApp> ()
        ;
    return tid;
}

void
TunnelApp::Setup (Ptr<Node> node, Ipv4Address address)
{
    m_node = node;
    m_peer = address;
    m_packetSize = 1000;
    m_nPackets = 10;
    m_dataRate = DataRate("100Mbps");
}

void
TunnelApp::StartApplication (void)
{
    m_running = true;
    m_packetsSent = 0;
    for(uint32_t i = 1; i < m_node->GetNDevices(); i++) {
        Ptr<Socket> socket = Socket::CreateSocket (m_node, TypeId::LookupByName ("ns3::UdpSocketFactory"));
        socket->Bind(InetSocketAddress(Ipv4Address::GetAny(), 50000 + i));
        socket->BindToNetDevice(m_node->GetDevice(i));
        socket->Connect (InetSocketAddress(m_peer, 50000 + i));
        m_sockets.push_back(socket);
    }
}

void
TunnelApp::StopApplication (void)
{
    m_running = false;

    if (m_sendEvent.IsRunning ())
        Simulator::Cancel (m_sendEvent);

}

void
TunnelApp::SendPacket (void)
{
    Ptr<Packet> packet = Create<Packet> (m_packetSize);
    for(Ptr<Socket> socket : m_sockets) {
        socket->Send (packet);
    }

    if (++m_packetsSent < m_nPackets)
        {
         ScheduleTx ();
        }
}

void
TunnelApp::ScheduleTx (void)
{
    if (m_running)
        {
            Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
            m_sendEvent = Simulator::Schedule (tNext, &TunnelApp::SendPacket, this);
        }
}