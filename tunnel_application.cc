
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/virtual-net-device.h"
#include "tunnel_application.h"
#include "tunnel_header.h"

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
TunnelApp::Setup (MPScheduler *scheduler, Ptr<Node> node, Ipv4Address tun_address, Ipv4Address remote_address)
{
    m_seq = 1;
    m_scheduler = scheduler;
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
        socket->Bind(InetSocketAddress(Ipv4Address::GetAny(), 49000 + i));
        socket->BindToNetDevice(m_node->GetDevice(i));
        socket->Connect (InetSocketAddress(m_peer, 49000 + i));
        socket->SetRecvCallback(MakeCallback(&TunnelApp::OnPacketRecv, this));
        m_sockets.push_back(socket);
        m_path_ack.push_back(0);
    }
    m_scheduler->Init(m_sockets.size());
    m_tun_device = AddTunInterface(m_node, m_tun_address);
    m_tun_device->SetSendCallback(MakeCallback(&TunnelApp::OnTunSend, this));
}

void
TunnelApp::StopApplication (void)
{
    m_running = false;

    if (m_sendEvent.IsRunning ())
        Simulator::Cancel (m_sendEvent);

}

bool TunnelApp::OnTunSend(Ptr<Packet> packet, const Address &src, const Address &dst, uint16_t proto) {
    uint index = m_scheduler->SchedulePacket(packet);
    TunHeader tunHeader;
    tunHeader.type = TunType::data;
    tunHeader.path = index;
    tunHeader.seq = m_seq++;
    tunHeader.path_seq = m_path_ack[index]++;
    packet->AddHeader(tunHeader);
    m_scheduler->OnSend(packet, tunHeader);
    m_sockets[index]->Send (packet);
    return true;
}

void TunnelApp::OnPacketRecv(Ptr<Socket> socket) {
    Ptr<Packet> packet = socket->Recv (65535, 0);
    TunHeader tunHeader;
    packet->RemoveHeader(tunHeader);
    if(tunHeader.type == TunType::data) {
        m_tun_device->Receive(packet, 0x0800, m_tun_device->GetAddress (), m_tun_device->GetAddress (), NetDevice::PACKET_HOST);
        Ptr<Packet> ackPacket = Create<Packet>();
        TunHeader ackHeader;
        ackHeader.type = TunType::ack;
        ackHeader.path = tunHeader.path;
        ackHeader.seq = tunHeader.seq;
        ackHeader.path_seq = tunHeader.path_seq;
        ackPacket->AddHeader(ackHeader);
        socket->Send(ackPacket);
    }
    else if(tunHeader.type == TunType::ack) {
        m_scheduler->OnAck(tunHeader);
    }
}