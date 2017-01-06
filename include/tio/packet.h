// Copyright: 2016 Twinleaf LLC
// Author: gilberto@tersatech.com
// License: Proprietary

// Platform-independent packet-related structures and constants for native
// packets in TL firmwares. Packets are comprised of a header, followed
// by a payload, followed by optional routing information (used to dispatch
// messages to/from a network of sensors).
// The header conveys the packet type and the sizes of the two following
// sections.

#ifndef TL_PACKET_H
#define TL_PACKET_H

#include <stddef.h>
#include <stdint.h>

struct tl_packet_header {
  uint8_t type;
  uint8_t routing_size;
  uint16_t payload_size;

#ifdef __cplusplus
  inline size_t total_size() const;
  inline uint8_t *payload_data();
  inline const uint8_t *payload_data() const;
  inline uint8_t *routing_data();
  inline const uint8_t *routing_data() const;
  inline int stream_id() const;
#endif

} __attribute__((__packed__));
typedef struct tl_packet_header tl_packet_header;

// Max size of a complete packet
#define TL_PACKET_MAX_SIZE 512

// Size at the end reserved for routing information
#define TL_PACKET_MAX_ROUTING_SIZE   8

// Maximum payload length (inferred)
#define TL_PACKET_MAX_PAYLOAD_SIZE \
  (TL_PACKET_MAX_SIZE - sizeof(tl_packet_header) - TL_PACKET_MAX_ROUTING_SIZE)

struct tl_packet {
  // Packet header
  tl_packet_header hdr;
  // Payload
  uint8_t payload[TL_PACKET_MAX_PAYLOAD_SIZE];
  // Reserved space for routing. Note: use tl_packet_routing_data() to get
  // a pointer to the routing data, since it will be at the end of the payload
  // and not at a fixed offset.
  uint8_t __routing_reserved[TL_PACKET_MAX_ROUTING_SIZE];
};
typedef struct tl_packet tl_packet;

// Packet types
#define TL_PTYPE_INVALID     0
#define TL_PTYPE_LOG         1 // Log messages
#define TL_PTYPE_RPC_REQ     2 // RPC request
#define TL_PTYPE_RPC_REP     3 // RPC reply
#define TL_PTYPE_RPC_ERROR   4 // RPC error
#define TL_PTYPE_STREAMDESC  5 // Description of data in a stream
#define TL_PTYPE_USER        6

#define TL_PTYPE_STREAM0   128 // First data stream
#define TL_PTYPE_STREAM(N) (TL_PTYPE_STREAM0 + (N))

// Return the total packet size given a valid header.
static inline size_t tl_packet_total_size(const tl_packet_header *pkt);

// Return a pointer to the start of the payload
static inline uint8_t *tl_packet_payload_data(tl_packet_header *pkt);

// Return a pointer to the start of the routing data
static inline uint8_t *tl_packet_routing_data(tl_packet_header *pkt);

// Return the next hop for a packet, removing it from the routing data.
// Returns >= 0 on success, with the hop ID, otherwise -1 and pkt unchanged.
static inline int tl_packet_pop_hop(tl_packet_header *pkt);

// Append a hop to the routing data.
// Returns 0 on success, otherwise -1 and pkt unchanged.
static inline int tl_packet_push_hop(tl_packet_header *pkt, uint8_t hop);

// Return the stream ID from the packet type, or -1 if the packet type
// is not that of a stream data packet
static inline int tl_packet_stream_id(const tl_packet_header *pkt);

// Parse a null terminated string of the form "/3/1/" into a binary routing
// encoding (which can be written directly to the routing data of a packet).
// Leading and trailing '/' optional. routing must point to at least
// TL_PACKET_MAX_ROUTING_SIZE bytes. Returns the number of hops that were
// parsed, or -1 in case of failure.
int tl_parse_routing(uint8_t *routing, const char *routing_path);

#define TL_ROUTING_FMT_BUF_SIZE (TL_PACKET_MAX_ROUTING_SIZE * 4 + 2)
int tl_format_routing(uint8_t *routing, size_t routing_size,
                      char *buf, size_t buf_size);

//////////////////////////////////////
// Implementation of inline methods

static inline size_t tl_packet_total_size(const tl_packet_header *pkt)
{
  return sizeof(*pkt) + pkt->payload_size + pkt->routing_size;
}

static inline uint8_t *tl_packet_payload_data(tl_packet_header *pkt)
{
  return ((uint8_t*) pkt) + sizeof(*pkt);
}

static inline uint8_t *tl_packet_routing_data(tl_packet_header *pkt)
{
  return ((uint8_t*) pkt) + sizeof(*pkt) + pkt->payload_size;
}

static inline int tl_packet_pop_hop(tl_packet_header *pkt)
{
  return (pkt->routing_size == 0) ? -1 :
    tl_packet_routing_data(pkt)[--pkt->routing_size];
}

static inline int tl_packet_push_hop(tl_packet_header *pkt, uint8_t hop)
{
  if (pkt->routing_size >= TL_PACKET_MAX_ROUTING_SIZE)
    return -1;
  tl_packet_routing_data(pkt)[pkt->routing_size++] = hop;
  return 0;
}

static inline int tl_packet_stream_id(const tl_packet_header *pkt)
{
  return (pkt->type >= TL_PTYPE_STREAM0) ? (pkt->type - TL_PTYPE_STREAM0) : -1;
}

#ifdef __cplusplus

inline size_t tl_packet_header::total_size() const
{
  return tl_packet_total_size(this);
}

inline uint8_t *tl_packet_header::payload_data()
{
  return reinterpret_cast<uint8_t*>(this) + sizeof(*this);
}

inline const uint8_t *tl_packet_header::payload_data() const
{
  return reinterpret_cast<const uint8_t*>(this) + sizeof(*this);
}

inline uint8_t *tl_packet_header::routing_data()
{
  return reinterpret_cast<uint8_t*>(this) + sizeof(*this) + payload_size;
}

inline const uint8_t *tl_packet_header::routing_data() const
{
  return reinterpret_cast<const uint8_t*>(this) + sizeof(*this) + payload_size;
}

inline int tl_packet_header::stream_id() const
{
  return tl_packet_stream_id(this);
}

#endif

#endif // TL_PACKET_H