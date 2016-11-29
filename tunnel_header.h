#ifndef MPSIM_TUN_HEADER_H
#define MPSIM_TUN_HEADER_H

#include "ns3/ptr.h"
#include "ns3/packet.h"
#include "ns3/header.h"
#include <iostream>

using namespace ns3;

/* A sample Header implementation
 */
enum TunType { data, ack };
class TunHeader : public Header 
{
public:

  TunHeader ();
  virtual ~TunHeader ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual uint32_t GetSerializedSize (void) const;
public:
  TunType  type;
  uint8_t path;
  uint32_t seq;
  uint32_t path_seq;
  float send_time;
};

#endif