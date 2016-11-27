
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/virtual-net-device.h"
#include "tunnel_application.h"

using namespace ns3;

Ptr<VirtualNetDevice> AddTunInterface(Ptr<Node> node, Ipv4Address tun_address);



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
TunnelApp::Setup (Ptr<Node> node, Ipv4Address tun_address, Ipv4Address remote_address)
{
    m_node = node;
    m_tun_address = tun_address;
    m_peer = remote_address;
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
        socket->SetRecvCallback(MakeCallback(&TunnelApp::OnPacketRecv, this));
        m_sockets.push_back(socket);
    }
    m_tun_device = AddTunInterface(m_node, m_tun_address);
    m_tun_device->SetSendCallback(MakeCallback(&TunnelApp::OnTunSend, this));
    //m_tun_device->Set(MakeCallback(&TunnelApp::OnTunSend, this));
}

void
TunnelApp::StopApplication (void)
{
    m_running = false;

    if (m_sendEvent.IsRunning ())
        Simulator::Cancel (m_sendEvent);

}

//Todo: Put packet scheduling in here
int i = 0;
bool TunnelApp::OnTunSend(Ptr<Packet> packet, const Address &src, const Address &dst, uint16_t proto) {
    m_sockets[i]->Send (packet);
    i++;
    i %= m_sockets.size();
    return true;
}

void TunnelApp::OnPacketRecv(Ptr<Socket> socket) {
    Ptr<Packet> packet = socket->Recv (65535, 0);
    m_tun_device->Receive(packet, 0x0800, m_tun_device->GetAddress (), m_tun_device->GetAddress (), NetDevice::PACKET_HOST);
}