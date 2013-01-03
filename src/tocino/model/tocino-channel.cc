/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include <cstdio>

#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/data-rate.h"
#include "ns3/packet.h"
#include "ns3/node.h"

#include "tocino-channel.h"
#include "tocino-rx.h"
#include "tocino-tx.h"
#include "tocino-net-device.h"
#include "tocino-flit-header.h"
#include "tocino-flow-control.h"

NS_LOG_COMPONENT_DEFINE ("TocinoChannel");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED( TocinoChannel );

TypeId TocinoChannel::GetTypeId( void )
{
  static TypeId tid = TypeId("ns3::TocinoChannel")
    .SetParent<Channel>()
    .AddConstructor<TocinoChannel>()
    .AddAttribute ("DataRate", 
                   "The data transmission rate of the channel",
                   DataRateValue (DataRate ("10Gbps")),
                   MakeDataRateAccessor (&TocinoChannel::m_bps),
                   MakeDataRateChecker ())
    .AddAttribute ("Delay", "Transmission delay through the channel",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&TocinoChannel::m_delay),
                   MakeTimeChecker ());
  return tid;
}

TocinoChannel::TocinoChannel()
{
    m_state = IDLE;
}

TocinoChannel::~TocinoChannel()
{};

Time TocinoChannel::GetTransmissionTime(Ptr<Packet> p)
{
    return Seconds(m_bps.CalculateTxTime(p->GetSize()*8));
}

void TocinoChannel::SetNetDevice(Ptr<TocinoNetDevice> tnd)
{
    m_tnd = tnd;
}

void TocinoChannel::SetTransmitter(TocinoTx* tx)
{
    m_tx = tx;
}

void TocinoChannel::SetReceiver(TocinoRx* rx)
{
    m_rx = rx;
}

uint32_t TocinoChannel::GetNDevices() const
{
    return 2;
}

Ptr<NetDevice> 
TocinoChannel::GetDevice(uint32_t i) const
{
    if (i == TX_DEV)
    {
        if (m_tx == NULL) return 0;
        return m_tx->GetNetDevice();
    }
    if (m_rx == NULL) return 0;
    return m_rx->GetNetDevice();
}

bool
TocinoChannel::TransmitStart (Ptr<Packet> p)
{
    Time transmit_time;
    
    if (m_state != IDLE)
    {
        NS_LOG_ERROR ("TocinoChannel::TransmitStart(): m_state is not IDLE");
        return false;
    }
    m_packet = p;
    
    transmit_time = GetTransmissionTime(p);
    Simulator::Schedule(transmit_time, &TocinoChannel::TransmitEnd, this);
    return true;
}

void 
TocinoChannel::TransmitEnd ()
{
    m_state = IDLE; // wire can be pipelined
    
    Simulator::ScheduleWithContext(m_rx->GetNetDevice()->GetNode()->GetId(),
                                   m_delay,
                                   &TocinoRx::Receive,
                                   m_rx,
                                   m_packet);
}

uint32_t
TocinoChannel::FlitBuffersRequired() const
{
    // Compute the worst-case delay between 
    //  - local detection of almost full
    // and
    //  - remote receipt of LLC flit

    // This occurs when the the almost-full condition
    // is detected *just* after the local transmitter
    // has begun sending a max size flit.
    //
    // Note that, similarly, the remote receiver may 
    // receive the LLC flit *just* after his
    // corresponding transmitter begins sending a flit.
    // We ignore that here, but take it into account later.
    
    const int SIZE_MAX_FLIT = TocinoFlitHeader::FLIT_LENGTH;
    const int SIZE_LLC_FLIT = GetTocinoFlowControlFlit(0)->GetSize();

    Time window = 
        Seconds( m_bps.CalculateTxTime( (SIZE_MAX_FLIT+SIZE_LLC_FLIT)*8 ) + m_delay );

    // What's the worst case number of flits that could
    // have been sent over a single VC during this
    // vulnerable period?
    //
    // We need to determine the most *dense* flit
    // pattern (the most flits in the fewest bytes).
    //
    // There are two obvious possibilities.  Both start
    // with the minimum size flit, which is a tail flit
    // carrying a 1-byte payload. After that, we repeat
    // some dense, but *legal*, flit pattern.
    
   
    // N.B.
    //
    // Remember that buffers are reserved per-VC.
    // So while you might see back-to-back tail flits
    // on different VCs, this cannot happen on a single
    // VC (unless they are larget head+tail flits).
    // 
    // We intentionally ignore illegal patterns like
    // repetition of MIN_TAIL_FLIT, because this can
    // never happen on a single VC and would require
    // an unreasonable about of reserve buffer space.
    
    // Non-repeating part, a single min-tail flit
    
    const int SIZE_MIN_TAIL_FLIT = TocinoFlitHeader::SIZE_OTHER+1;
    
    Time noRep =
        Seconds( m_bps.CalculateTxTime( SIZE_MIN_TAIL_FLIT*8 ) );

    // Repeating pattern A is simply:
    //  - 1 minimum payload head-tail flit
    
    const int SIZE_MIN_HEADTAIL_FLIT = TocinoFlitHeader::SIZE_HEAD+1;

    Time patA = 
        Seconds( m_bps.CalculateTxTime( SIZE_MIN_HEADTAIL_FLIT*8 ) );
    
    // Repeating pattern B is:
    //  - 1 full head flit
    //  - 1 minimum-size tail flit
   
    Time patB = 
        Seconds( m_bps.CalculateTxTime( (SIZE_MAX_FLIT+SIZE_MIN_TAIL_FLIT)*8 ) );

    // Worst case is whichever pattern takes less time

    Time rep = std::min( patA, patB );

    // Compute number of flits to buffer
    
    // (The +1 here is to account for the non-repeating
    // part who's time we subtract out of the window.)
    uint32_t flits = ceil( (((window-noRep)/rep)+1).GetDouble() );

    // Add one to account for possibility that receiver
    // may have put a flit on the wire *just* prior to
    // receipt of LLC flit
    flits++;

    NS_LOG_LOGIC( flits << " buffers required" );

    return flits;
}

} // namespace ns3

