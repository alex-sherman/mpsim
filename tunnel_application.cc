
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
TunnelApp::Setup (MPScheduler *scheduler, Ptr<Node> node, Ipv4Address tun_address, Ipv4Address remote_address, bool log_packets)
{
    stream.open("packet_log");
    stream2.open("delivery_log");
    m_log_packets = log_packets;
    m_seq = 1;
    m_scheduler = scheduler;
    m_node = node;
    m_tun_address = tun_address;
    m_peer = remote_address;
    m_packetSize = 1000;
    m_nPackets = 10;
    m_dataRate = DataRate("100Mbps");

    // NS_LOG_UNCOND("***TunnelApp Setup***");
    // NS_LOG_UNCOND("n_devices = " << m_node->GetNDevices());

    // for(uint32_t i = 0; i < m_node->GetNDevices(); i++) {
    //     NS_LOG_UNCOND("Node " << i <<":  " << m_node->GetDevice(i)->GetAddress());
    // }

    // NS_LOG_UNCOND("***END***");

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
    m_scheduler->Init(m_sockets.size(), this);
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

void TunnelApp::TunSendIfe(Ptr<Packet> packet, uint interface) {
    TunHeader tunHeader;
    packet->RemoveHeader(tunHeader);
    tunHeader.path = interface;
    tunHeader.path_seq = m_path_ack[interface]++;
    tunHeader.time = Simulator::Now().GetSeconds();
    m_scheduler->OnSend(packet, tunHeader);
    packet->AddHeader(tunHeader);
    m_sockets[interface]->Send(packet);
}

bool TunnelApp::OnTunSend(Ptr<Packet> packet, const Address &src, const Address &dst, uint16_t proto) {
    TunHeader tunHeader;
    tunHeader.type = TunType::data;
    tunHeader.seq = m_seq++;
    tunHeader.arrival_time = Simulator::Now().GetSeconds();
    packet->AddHeader(tunHeader);
    m_scheduler->SchedulePacket(packet);
    return true;
}

void TunnelApp::DeliverPacket(Ptr<Packet> packet, Ptr<VirtualNetDevice> m_tun_device) {
    TunHeader tunHeader;
    packet->RemoveHeader(tunHeader);
    m_last_sent_seq = tunHeader.seq + 1;
    m_tun_device->Receive(packet, 0x0800, m_tun_device->GetAddress (), m_tun_device->GetAddress (), NetDevice::PACKET_HOST);
    if(m_log_packets)
        stream2 << "Deliver: " << Simulator::Now().GetSeconds() << " " << tunHeader << ",size=" << packet->GetSize()
            << ",travel_time=" << Simulator::Now().GetSeconds() - tunHeader.time << "\n";
}

void TunnelApp::ServiceReorderBuffer() {
    while(m_reorder_buffer.size() > 0) {
        TunHeader header;
        Ptr<Packet> packet = m_reorder_buffer.front();
        packet->PeekHeader(header);
        if(header.seq == m_last_sent_seq || Simulator::Now().GetSeconds() - header.arrival_time > reorder_latency) {
            //NS_LOG_UNCOND("Reorder head: " << header.seq << " " << " " << header.arrival_time << " " << m_last_sent_seq << " " << Simulator::Now().GetSeconds());
            m_reorder_buffer.pop_front();
            DeliverPacket(packet, m_tun_device);
        }
        else
            return;
    }
}

void TunnelApp::OnPacketRecv(Ptr<Socket> socket) {
    Ptr<Packet> packet = socket->Recv (65535, 0);
    TunHeader tunHeader;
    packet->RemoveHeader(tunHeader);
    tunHeader.arrival_time = Simulator::Now().GetSeconds();
    packet->AddHeader(tunHeader);
    if(tunHeader.type == TunType::data) {
        if(m_log_packets)
            stream << "Packet: " << Simulator::Now().GetSeconds() << " " << tunHeader << ",size=" << packet->GetSize()
                << ",travel_time=" << Simulator::Now().GetSeconds() - tunHeader.time << "\n";
        m_scheduler->OnRecv(tunHeader);
        if(tunHeader.seq == m_last_sent_seq)
            DeliverPacket(packet, m_tun_device);
        else if(tunHeader.seq > m_last_sent_seq) {
            auto it = m_reorder_buffer.begin();
            while (1) {
                if(it == m_reorder_buffer.end()) {
                    m_reorder_buffer.insert(it, packet);
                    break;
                }
                TunHeader testHeader;
                (*it)->PeekHeader(testHeader);
                if(testHeader.seq > tunHeader.seq) {
                    m_reorder_buffer.insert(it, packet);
                    break;
                }
                it++;
            }
            for (auto it = m_reorder_buffer.begin(); it != m_reorder_buffer.end(); it++) {
                TunHeader testHeader;
                (*it)->PeekHeader(testHeader);
            }
        }
        else {
            NS_LOG_UNCOND("DROP");
        }
        ServiceReorderBuffer();
        Ptr<Packet> ackPacket = Create<Packet>();
        TunHeader ackHeader;
        ackHeader.type = TunType::ack;
        ackHeader.path = tunHeader.path;
        ackHeader.seq = tunHeader.seq;
        ackHeader.path_seq = tunHeader.path_seq;
        ackHeader.time = Simulator::Now().GetSeconds() - tunHeader.time;
        ackHeader.queueing_time = tunHeader.queueing_time;
        ackHeader.arrival_time = Simulator::Now().GetSeconds();
        ackPacket->AddHeader(ackHeader);
        socket->Send(ackPacket);
    }
    else if(tunHeader.type == TunType::ack) {
        bool loss;
        m_scheduler->OnAck(tunHeader, loss);
    }
}
