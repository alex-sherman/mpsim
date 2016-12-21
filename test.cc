#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <tuple>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/applications-module.h"
#include "ns3/virtual-net-device.h"

#include "tunnel_application.h"
#include "tcp_sender.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SocketBoundRoutingExample");

NetDeviceContainer AddInterface(NodeContainer nc, const char* rate, const char* delay);
Ptr<VirtualNetDevice> AddTunInterface(Ptr<Node> node, const char*addr);

static void SinkRx (std::string path, Ptr<const Packet> p, const Address &address) {
    //NS_LOG_UNCOND("Sink rx: " << *p);
}


void
CwndChange (uint32_t oldval, uint32_t newval)
{
    if(newval < oldval)
        NS_LOG_UNCOND(Simulator::Now ().GetSeconds () << " " << newval);
}

int main (int argc, char *argv[])
{
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));
    Packet::EnablePrinting();

    // Allow the user to override any of the defaults and the above
    // DefaultValue::Bind ()s at run-time, via command-line arguments
    CommandLine cmd;
    cmd.Parse (argc, argv);

    Ptr<Node> nSrc = CreateObject<Node> ();
    Ptr<Node> nDst = CreateObject<Node> ();

    // Point-to-point links
    NodeContainer nSrcDst = NodeContainer (nSrc, nDst);

    InternetStackHelper internet;
    internet.Install (nSrcDst);

    vector<tuple<const char*, const char*>> interfaces;
    interfaces.push_back(make_tuple("1Mbps", "50ms"));
    interfaces.push_back(make_tuple("2Mbps", "50ms"));
    interfaces.push_back(make_tuple("2Mbps", "150ms"));

    vector<Ptr<Queue>> queues;
    vector<DataRate> rates;
    vector<double> delays;

    for(auto interface : interfaces) {
        NetDeviceContainer ndc = AddInterface(nSrcDst, get<0>(interface), get<1>(interface));
        PointerValue ptr;
        ndc.Get(0)->GetAttribute ("TxQueue", ptr);
        queues.push_back(ptr.Get<Queue>());
        rates.push_back(DataRate(get<0>(interface)));
        delays.push_back(Time(get<1>(interface)).GetSeconds());
    }
    MPScheduler *scheduler1 = new ATScheduler(rates, delays, queues);
    Ptr<TunnelApp> app = CreateObject<TunnelApp> ();
    app->Setup(scheduler1, nSrc, Ipv4Address("172.1.1.1"), Ipv4Address("10.1.1.2"));
    nSrc->AddApplication (app);
    app->SetStartTime (Seconds (1.));
    app->SetStopTime (Seconds (30.));


    MPScheduler *scheduler2 = new ATScheduler(rates, delays, queues);
    Ptr<TunnelApp> app2 = CreateObject<TunnelApp>();
    app2->Setup(scheduler2, nDst, Ipv4Address("172.1.1.2"), Ipv4Address("10.1.1.1"), true);
    nDst->AddApplication (app2);
    app2->SetStartTime (Seconds (1.));
    app2->SetStopTime (Seconds (30.));

    // Create UDP server sink
    uint16_t port = 50000;
    Address sinkLocalAddress (InetSocketAddress (Ipv4Address("172.1.1.2"), port));
    PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", sinkLocalAddress);
    ApplicationContainer sinkApp = sinkHelper.Install (nDst);
    sinkApp.Start (Seconds (1.0));
    sinkApp.Stop (Seconds (30.0));

    // Create UDP sender
    /*OnOffHelper clientHelper ("ns3::UdpSocketFactory", Address ());
    clientHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    clientHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    clientHelper.SetAttribute ("Remote", AddressValue(sinkLocalAddress));
    clientHelper.SetConstantRate(DataRate("1Mbps"), 1300);
    ApplicationContainer clientApp = clientHelper.Install(nSrc);
    clientApp.Start (Seconds (1.0));
    clientApp.Stop (Seconds (20.0));*/


    // Create TCP server sink
    uint16_t port2 = 50001;
    Address sinkLocalAddress2 (InetSocketAddress (Ipv4Address("172.1.1.2"), port2));
    PacketSinkHelper sinkHelper2 ("ns3::TcpSocketFactory", sinkLocalAddress2);
    ApplicationContainer sinkApp2 = sinkHelper2.Install (nDst);
    sinkApp2.Start (Seconds (1.0));
    sinkApp2.Stop (Seconds (30.0));

    // Create TCP sender
    Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (nSrc, TcpSocketFactory::GetTypeId ());
    ns3TcpSocket->TraceConnectWithoutContext("CongestionWindow", MakeCallback (&CwndChange));

    Ptr<MyApp> tcp_app = CreateObject<MyApp>();
    tcp_app->Setup (ns3TcpSocket, sinkLocalAddress2, 1040, DataRate ("10Mbps"));
    nSrc->AddApplication (tcp_app);
    tcp_app->SetStartTime (Seconds (1.));
    tcp_app->SetStopTime (Seconds (10.));


    LogComponentEnableAll (LOG_PREFIX_TIME);
    LogComponentEnable ("SocketBoundRoutingExample", LOG_LEVEL_ALL);

    Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx",
                    MakeCallback (&SinkRx));
    Config::ConnectWithoutContext("/NodeList/0/$ns3::TcpL4Protocol/SocketList/1/CongestionWindow", MakeCallback (&CwndChange));

    Simulator::Run ();
    Simulator::Destroy ();

    return 0;
}
