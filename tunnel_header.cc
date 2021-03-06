#include "tunnel_header.h"

TunHeader::TunHeader ()
{
  // we must provide a public default constructor, 
  // implicit or explicit, but never private.
}
TunHeader::~TunHeader ()
{
}

TypeId
TunHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TunHeader")
    .SetParent<Header> ()
    .AddConstructor<TunHeader> ()
  ;
  return tid;
}
TypeId
TunHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
TunHeader::Print (std::ostream &os) const
{
  // This method is invoked by the packet printing
  // routines to print the content of my header.
  //os << "data=" << m_data << std::endl;
  os << "type=" << type << ",path=" << (int)path << ",seq=" << seq << ",path_seq=" << path_seq << ",send_time=" << time << ",arrival_time=" << arrival_time;
}
uint32_t
TunHeader::GetSerializedSize (void) const
{
  return 22;
}
void
TunHeader::Serialize (Buffer::Iterator start) const
{
  // we can serialize two bytes at the start of the buffer.
  // we write them in network byte order.
  start.WriteU8(type);
  start.WriteU8(path);
  start.WriteHtonU32(seq);
  start.WriteHtonU32(path_seq);
  start.WriteHtonU32(time * 100000);
  start.WriteHtonU32(arrival_time * 100000);
  start.WriteHtonU32(queueing_time * 100000);
}
uint32_t
TunHeader::Deserialize (Buffer::Iterator start)
{
  // we can deserialize two bytes from the start of the buffer.
  // we read them in network byte order and store them
  // in host byte order.
  type = (TunType)start.ReadU8();
  path = start.ReadU8();
  seq = start.ReadNtohU32();
  path_seq = start.ReadNtohU32();
  time = start.ReadNtohU32() / 100000.0;
  arrival_time = start.ReadNtohU32() / 100000.0;
  queueing_time = start.ReadNtohU32() / 100000.0;

  return 22;
}