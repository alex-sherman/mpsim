/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/* Test program for multi-interface host, static routing

         Destination host (10.20.1.2)
                 |
                 | 10.20.1.0/24
              DSTRTR 
  10.10.1.0/24 /   \  10.10.2.0/24
              /     \
           Rtr1    Rtr2
 10.1.1.0/24 |      | 10.1.2.0/24
             |      /
              \    /
             Source
*/

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

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SocketBoundRoutingExample");

void SendStuff (Ptr<Socket> sock, Ipv4Address dstaddr, uint16_t port);
void BindSock (Ptr<Socket> sock, Ptr<NetDevice> netdev);
void srcSocketRecv (Ptr<Socket> socket);
void dstSocketRecv (Ptr<Socket> socket);
static const uint32_t totalTxBytes = 200000;
static uint32_t currentTxBytes = 0;
static const uint32_t writeSize = 1040;
uint8_t data[writeSize];
void StartFlow (Ptr<Socket>, Ipv4Address, uint16_t);
void WriteUntilBufferFull (Ptr<Socket>, uint32_t);

int 
main (int argc, char *argv[])
{

    // Allow the user to override any of the defaults and the above
    // DefaultValue::Bind ()s at run-time, via command-line arguments
    CommandLine cmd;
    cmd.Parse (argc, argv);

    Ptr<Node> nSrc = CreateObject<Node> ();
    Ptr<Node> nDst = CreateObject<Node> ();
    Ptr<Node> nRtr1 = CreateObject<Node> ();
    Ptr<Node> nRtr2 = CreateObject<Node> ();
    Ptr<Node> nDstRtr = CreateObject<Node> ();

    NodeContainer c = NodeContainer (nSrc, nDst, nRtr1, nRtr2, nDstRtr);

    InternetStackHelper internet;
    internet.Install (c);

    // Point-to-point links
    NodeContainer nSrcDst1 = NodeContainer (nSrc, nDst);
    NodeContainer nSrcDst2 = NodeContainer (nSrc, nDst);

    // We create the channels first without any IP addressing information
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute ("DataRate", StringValue ("0.1Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
    PointToPointHelper p2p2;
    p2p2.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
    p2p2.SetChannelAttribute ("Delay", StringValue ("2ms"));
    NetDeviceContainer dSrcDst1 = p2p.Install (nSrcDst1);
    NetDeviceContainer dSrcDst2 = p2p2.Install (nSrcDst2);

    Ptr<NetDevice> SrcToRtr1=dSrcDst1.Get (0);
    Ptr<NetDevice> SrcToRtr2=dSrcDst2.Get (0);

    Ptr<Ipv4> ipv4Src = nSrc->GetObject<Ipv4> ();
    Ptr<Ipv4> ipv4Dst = nDst->GetObject<Ipv4> ();

    int32_t interface1 = ipv4Src->GetInterfaceForDevice(SrcToRtr1);
    int32_t interface2 = ipv4Src->GetInterfaceForDevice(SrcToRtr2);

    interface1 = ipv4Src->AddInterface(SrcToRtr1);
    interface2 = ipv4Src->AddInterface(SrcToRtr2);
    int32_t interface3 = ipv4Dst->AddInterface(dSrcDst1.Get(1));
    int32_t interface4 = ipv4Dst->AddInterface(dSrcDst2.Get(1));
    ipv4Src->AddAddress(interface1, Ipv4InterfaceAddress(Ipv4Address("10.1.1.1"), Ipv4Mask("255.255.255.0")));
    ipv4Src->SetMetric(interface1, 1);
    ipv4Src->SetUp(interface1);
    ipv4Src->AddAddress(interface2, Ipv4InterfaceAddress(Ipv4Address("10.1.1.2"), Ipv4Mask("255.255.255.0")));
    ipv4Src->SetMetric(interface2, 1);
    ipv4Src->SetUp(interface2);

    ipv4Dst->AddAddress(interface3, Ipv4InterfaceAddress(Ipv4Address("10.1.1.3"), Ipv4Mask("255.255.255.0")));
    ipv4Dst->SetMetric(interface3, 1);
    ipv4Dst->SetUp(interface3);
    ipv4Dst->AddAddress(interface4, Ipv4InterfaceAddress(Ipv4Address("10.1.1.3"), Ipv4Mask("255.255.255.0")));
    ipv4Dst->SetMetric(interface4, 1);
    ipv4Dst->SetUp(interface4);

    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> staticRoutingSrc = ipv4RoutingHelper.GetStaticRouting (ipv4Src);
    Ptr<Ipv4StaticRouting> staticRoutingDst = ipv4RoutingHelper.GetStaticRouting (ipv4Dst);

    // Create static routes from Src to Dst

    // Two routes to same destination - setting separate metrics. 
    // You can switch these to see how traffic gets diverted via different routes
    //staticRoutingSrc->AddHostRouteTo (Ipv4Address ("10.20.1.2"), Ipv4Address ("10.20.1.2"), 1,5);
//
    //staticRoutingDst->AddHostRouteTo (Ipv4Address ("10.1.1.1"), Ipv4Address ("10.1.1.1"), 2);
    //staticRoutingDst->AddHostRouteTo (Ipv4Address ("10.1.1.2"), Ipv4Address ("10.1.1.2"), 2);
    // There are no apps that can utilize the Socket Option so doing the work directly..
    // Taken from tcp-large-transfer example

    Ptr<Socket> srcSocket = Socket::CreateSocket (nSrc, TypeId::LookupByName ("ns3::UdpSocketFactory"));
    srcSocket->Bind ();
    srcSocket->SetRecvCallback (MakeCallback (&srcSocketRecv));

    Ptr<Socket> dstSocket = Socket::CreateSocket (nDst, TypeId::LookupByName ("ns3::UdpSocketFactory"));
    uint16_t dstport = 12345;
    Ipv4Address dstaddr ("10.1.1.3");
    InetSocketAddress dst = InetSocketAddress (dstaddr, dstport);
    dstSocket->Bind (dst);
    dstSocket->SetRecvCallback (MakeCallback (&dstSocketRecv));

    AsciiTraceHelper ascii;
    p2p.EnableAsciiAll (ascii.CreateFileStream ("socket-bound-static-routing.tr"));
    p2p.EnablePcapAll ("socket-bound-static-routing");

    LogComponentEnableAll (LOG_PREFIX_TIME);
    LogComponentEnable ("SocketBoundRoutingExample", LOG_LEVEL_INFO);

    // First packet as normal (goes via Rtr1)
    //Simulator::Schedule (Seconds (0.1),&StartFlow, srcSocket, dstaddr, dstport);
    //// Second via Rtr1 explicitly
    Simulator::Schedule (Seconds (1.0),&BindSock, srcSocket, SrcToRtr1);
    Simulator::Schedule (Seconds ( 1.1),&StartFlow, srcSocket, dstaddr, dstport);
    // Third via Rtr2 explicitly
    //Simulator::Schedule (Seconds (2.0),&BindSock, srcSocket, SrcToRtr2);
    //Simulator::Schedule (Seconds (2.1),&StartFlow, srcSocket, dstaddr, dstport);
    // Fourth again as normal (goes via Rtr1)
    //Simulator::Schedule (Seconds (3.0),&BindSock, srcSocket, Ptr<NetDevice>(0));
    //Simulator::Schedule (Seconds (3.1),&StartFlow, srcSocket, dstaddr, dstport);
    // If you uncomment what's below, it results in ASSERT failing since you can't 
    // bind to a socket not existing on a node
    // Simulator::Schedule(Seconds(4.0),&BindSock, srcSocket, dDstRtrdDst.Get(0)); 
    Simulator::Run ();
    Simulator::Destroy ();

    return 0;
}

void SendStuff (Ptr<Socket> sock, Ipv4Address dstaddr, uint16_t port)
{
    Ptr<Packet> p = Create<Packet> ();
    p->AddPaddingAtEnd (100);
    sock->SendTo (p, 0, InetSocketAddress (dstaddr,port));
    return;
}

void StartFlow (Ptr<Socket> localSocket,
                                Ipv4Address servAddress,
                                uint16_t servPort)
{
    NS_LOG_INFO ("Starting flow at time " <<  Simulator::Now ().GetSeconds ());
    //currentTxBytes = 0;
    localSocket->Bind ();
    localSocket->Connect (InetSocketAddress (servAddress, servPort)); //connect

    // tell the tcp implementation to call WriteUntilBufferFull again
    // if we blocked and new tx buffer space becomes available
    localSocket->SetSendCallback (MakeCallback (&WriteUntilBufferFull));
    WriteUntilBufferFull (localSocket, localSocket->GetTxAvailable ());
}

void WriteUntilBufferFull (Ptr<Socket> localSocket, uint32_t txSpace)
{
    while (currentTxBytes < totalTxBytes && localSocket->GetTxAvailable () > 0)
        {
            uint32_t left = totalTxBytes - currentTxBytes;
            uint32_t dataOffset = currentTxBytes % writeSize;
            uint32_t toWrite = writeSize - dataOffset;
            toWrite = std::min (toWrite, left);
            toWrite = std::min (toWrite, localSocket->GetTxAvailable ());
            int amountSent = localSocket->Send (&data[dataOffset], toWrite, 0);
            if(amountSent < 0)
                {
                    // we will be called again when new tx space becomes available.
                    return;
                }
            currentTxBytes += amountSent;
        }
    localSocket->Close ();
}

void BindSock (Ptr<Socket> sock, Ptr<NetDevice> netdev)
{
    sock->BindToNetDevice (netdev);
    return;
}

void
srcSocketRecv (Ptr<Socket> socket)
{
    Address from;
    Ptr<Packet> packet = socket->RecvFrom (from);
    packet->RemoveAllPacketTags ();
    packet->RemoveAllByteTags ();
    NS_LOG_INFO ("Source Received " << packet->GetSize () << " bytes from " << InetSocketAddress::ConvertFrom (from).GetIpv4 ());
    if (socket->GetBoundNetDevice ())
        {
            NS_LOG_INFO ("Socket was bound");
        } 
    else
        {
            NS_LOG_INFO ("Socket was not bound");
        }
}

void
dstSocketRecv (Ptr<Socket> socket)
{
    Address from;
    Ptr<Packet> packet = socket->RecvFrom (from);
    packet->RemoveAllPacketTags ();
    packet->RemoveAllByteTags ();
    InetSocketAddress address = InetSocketAddress::ConvertFrom (from);
    NS_LOG_INFO ("Destination Received " << packet->GetSize () << " bytes from " << address.GetIpv4 ());
    NS_LOG_INFO ("Triggering packet back to source node's interface 1");
    SendStuff (socket, Ipv4Address ("10.1.1.1"), address.GetPort ());
}
