#include <iostream>
#include <fstream>
#include <string>
#include <cassert>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/applications-module.h"
#include "ns3/virtual-net-device.h"

#include "tunnel_application.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SocketBoundRoutingExample");

NetDeviceContainer AddInterface(NodeContainer nc, const char* rate, const char* delay);
Ptr<VirtualNetDevice> AddTunInterface(Ptr<Node> node, const char*addr);

static void SinkRx (std::string path, Ptr<const Packet> p, const Address &address) {
    //NS_LOG_UNCOND("Sink rx: " << *p);
}


static void
CwndTracer (uint32_t oldval, uint32_t newval)
{
  NS_LOG_UNCOND(Simulator::Now ().GetSeconds () << " " << newval << std::endl);
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

    NetDeviceContainer i1 = AddInterface(nSrcDst, ".1Mbps", "500ms");
    NetDeviceContainer i2 = AddInterface(nSrcDst, ".2Mbps", "500ms");

    MPScheduler *scheduler1 = new CWNDScheduler();
    Ptr<TunnelApp> app = CreateObject<TunnelApp> ();
    app->Setup(scheduler1, nSrc, Ipv4Address("172.1.1.1"), Ipv4Address("10.1.1.2"));
    nSrc->AddApplication (app);
    app->SetStartTime (Seconds (1.));
    app->SetStopTime (Seconds (20.));

    MPScheduler *scheduler2 = new CWNDScheduler();
    Ptr<TunnelApp> app2 = CreateObject<TunnelApp>();
    app2->Setup(scheduler2, nDst, Ipv4Address("172.1.1.2"), Ipv4Address("10.1.1.1"), true);
    nDst->AddApplication (app2);
    app2->SetStartTime (Seconds (1.));
    app2->SetStopTime (Seconds (20.));

    // Create TCP server sink
    uint16_t port = 50000;
    Address sinkLocalAddress (InetSocketAddress (Ipv4Address("172.1.1.2"), port));
    PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", sinkLocalAddress);
    ApplicationContainer sinkApp = sinkHelper.Install (nDst);
    sinkApp.Start (Seconds (1.0));
    sinkApp.Stop (Seconds (10.0));

    // Create TCP sender
    OnOffHelper clientHelper ("ns3::UdpSocketFactory", Address ());
    clientHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    clientHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    clientHelper.SetAttribute ("Remote", AddressValue(sinkLocalAddress));
    clientHelper.SetConstantRate(DataRate("1Mbps"), 1300);
    ApplicationContainer clientApp = clientHelper.Install(nSrc);
    clientApp.Start (Seconds (1.0));
    clientApp.Stop (Seconds (8.0));


    LogComponentEnableAll (LOG_PREFIX_TIME);
    LogComponentEnable ("SocketBoundRoutingExample", LOG_LEVEL_ALL);

    Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx",
                    MakeCallback (&SinkRx));
    Config::Connect ("/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/CongestionWindow", MakeCallback (&CwndTracer));

    Simulator::Run ();
    Simulator::Destroy ();

    return 0;
}