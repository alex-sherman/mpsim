
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
#include "ns3/virtual-net-device.h"

using namespace ns3;


PointToPointHelper p2p;
uint mac_i = 0;
NetDeviceContainer AddInterface(NodeContainer nc, const char* rate, const char* delay, const char* ip1, const char* ip2) {
    
    p2p.SetDeviceAttribute ("DataRate", StringValue (rate));
    p2p.SetChannelAttribute ("Delay", StringValue (delay));
    NetDeviceContainer ndc = p2p.Install (nc);
    p2p.EnablePcapAll ("socket-bound-static-routing");
    Ptr<Ipv4> ipv4Src = nc.Get(0)->GetObject<Ipv4> ();
    Ptr<Ipv4> ipv4Dst = nc.Get(1)->GetObject<Ipv4> ();
    int32_t interfaceSrc = ipv4Src->AddInterface(ndc.Get(0));
    ipv4Src->AddAddress(interfaceSrc, Ipv4InterfaceAddress(Ipv4Address(ip1), Ipv4Mask("255.255.255.0")));
    ipv4Src->SetMetric(interfaceSrc, 1);
    ipv4Src->SetUp(interfaceSrc);

    int32_t interfaceDst = ipv4Dst->AddInterface(ndc.Get(1));
    ipv4Dst->AddAddress(interfaceDst, Ipv4InterfaceAddress(Ipv4Address(ip2), Ipv4Mask("255.255.255.0")));
    ipv4Dst->SetMetric(interfaceDst, 1);
    ipv4Dst->SetUp(interfaceDst);

    return ndc;
}
NetDeviceContainer AddInterface(NodeContainer nc, const char* rate, const char* delay) {
    return AddInterface(nc, rate, delay, "10.1.1.1", "10.1.1.2");
}

Ptr<VirtualNetDevice> AddTunInterface(Ptr<Node> node, Ipv4Address tun_address) {
    Ptr<VirtualNetDevice> tun = CreateObject<VirtualNetDevice>();
    char macbuf[100];
    snprintf(macbuf, sizeof(macbuf), "11:00:01:02:03:0%d", mac_i++);
    tun->SetAddress(Mac48Address(macbuf));
    node->AddDevice(tun);
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();

    int32_t interface_index = ipv4->AddInterface(tun);
    ipv4->AddAddress(interface_index, Ipv4InterfaceAddress(tun_address, Ipv4Mask("255.255.255.0")));
    ipv4->SetUp(interface_index);
    return tun;
}