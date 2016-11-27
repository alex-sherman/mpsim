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

int main (int argc, char *argv[])
{
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

    NetDeviceContainer i1 = AddInterface(nSrcDst, ".01Mbps", "1ms");
    NetDeviceContainer i2 = AddInterface(nSrcDst, "1Mbps", "1ms");

    MPScheduler *scheduler1 = new CWNDScheduler();
    Ptr<TunnelApp> app = CreateObject<TunnelApp> ();
    app->Setup(scheduler1, nSrc, Ipv4Address("172.1.1.1"), Ipv4Address("10.1.1.2"));
    nSrc->AddApplication (app);
    app->SetStartTime (Seconds (1.));
    app->SetStopTime (Seconds (20.));

    MPScheduler *scheduler2 = new CWNDScheduler();
    Ptr<TunnelApp> app2 = CreateObject<TunnelApp> ();
    app2->Setup(scheduler2, nDst, Ipv4Address("172.1.1.2"), Ipv4Address("10.1.1.1"));
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
    clientHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1000]"));
    clientHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    clientHelper.SetAttribute ("Remote", AddressValue(sinkLocalAddress));
    clientHelper.SetConstantRate(DataRate("0.9Mbps"));
    ApplicationContainer clientApp = clientHelper.Install(nSrc);
    clientApp.Start (Seconds (1.0));
    clientApp.Stop (Seconds (10.0));


    LogComponentEnableAll (LOG_PREFIX_TIME);
    LogComponentEnable ("SocketBoundRoutingExample", LOG_LEVEL_ALL);


    Simulator::Run ();
    Simulator::Destroy ();

    return 0;
}