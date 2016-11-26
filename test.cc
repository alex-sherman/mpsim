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
#include "ns3/applications-module.h"

#include "tunnel_application.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SocketBoundRoutingExample");

NetDeviceContainer AddInterface(NodeContainer nc, const char* rate, const char* delay);
void SendStuff (Ptr<TunnelApp> app);
void BindSock (Ptr<Socket> sock, Ptr<NetDevice> netdev);
void srcSocketRecv (Ptr<Socket> socket);
void dstSocketRecv (Ptr<Socket> socket);
static const uint32_t totalTxBytes = 200000;
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

    // Point-to-point links
    NodeContainer nSrcDst = NodeContainer (nSrc, nDst);

    InternetStackHelper internet;
    internet.Install (nSrcDst);

    NetDeviceContainer i1 = AddInterface(nSrcDst, "0.1Mbps", "2ms");
    NetDeviceContainer i2 = AddInterface(nSrcDst, "0.1Mbps", "2ms");

    Ptr<TunnelApp> app = CreateObject<TunnelApp> ();
    app->Setup(nSrc, InetSocketAddress(Ipv4Address("10.1.1.2"), 50000), 1040, 1000, DataRate ("100Mbps"));
    nSrc->AddApplication (app);
    app->SetStartTime (Seconds (1.));
    app->SetStopTime (Seconds (20.));
    Simulator::Schedule(Seconds(1.1), &SendStuff, app);

    Ptr<TunnelApp> app2 = CreateObject<TunnelApp> ();
    app2->Setup(nDst, InetSocketAddress(Ipv4Address("10.1.1.1"), 50000), 1040, 1000, DataRate ("100Mbps"));
    nDst->AddApplication (app2);
    app2->SetStartTime (Seconds (1.));
    app2->SetStopTime (Seconds (20.));

    LogComponentEnableAll (LOG_PREFIX_TIME);
    LogComponentEnable ("SocketBoundRoutingExample", LOG_LEVEL_INFO);


    Simulator::Run ();
    Simulator::Destroy ();

    return 0;
}

void SendStuff (Ptr<TunnelApp> app)
{
    app->SendPacket();
}