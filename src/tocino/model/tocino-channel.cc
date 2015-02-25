/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include <sstream>

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
    .AddAttribute ("Delay", "Transmission delay through the channel",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&TocinoChannel::m_delay),
                   MakeTimeChecker ())
    .AddAttribute ("DataRate", 
                   "The data transmission rate of the channel",
                   DataRateValue (DataRate ("10Gbps")),
                   MakeDataRateAccessor (&TocinoChannel::m_bps),
                   MakeDataRateChecker ())
    ;
  return tid;
}

TocinoChannel::TocinoChannel()
    : m_delay( Seconds( 0 ) )
    , m_bps( DataRate( "10Gbps" ) )
    , m_flit( NULL )
    , m_tx( NULL )
    , m_rx( NULL )
    , m_state( IDLE )
    , m_totalBytesTransmitted( 0 )
    , m_totalFlitsTransmitted( 0 )
    , m_totalTransmitTime( Seconds( 0 ) )
    , m_LLCBytesTransmitted( 0 )
    , m_LLCFlitsTransmitted( 0 )
    , m_LLCTransmitTime( Seconds( 0 ) )
{}

TocinoChannel::~TocinoChannel()
{};

Time TocinoChannel::GetTransmissionTime(Ptr<Packet> p)
{
    return Seconds( m_bps.CalculateTxTime( p->GetSize() ) );
}

void TocinoChannel::SetTransmitter(TocinoTx* tx)
{
    m_tx = tx;

    std::ostringstream oss;
    
    Ptr<TocinoNetDevice> txNetDevice = m_tx->GetTocinoNetDevice();
    TocinoAddress txAdd = txNetDevice->GetTocinoAddress();

    oss << "("
        << static_cast<unsigned>( txAdd.GetX() )
        << ","
        << static_cast<unsigned>( txAdd.GetY() )
        << ","
        << static_cast<unsigned>( txAdd.GetZ() )
        << ")";

    m_txString = oss.str();

    m_vcUsageHistogram.resize( m_tx->GetTocinoNetDevice()->GetNVCs(), 0 );
}


void TocinoChannel::SetReceiver(TocinoRx* rx)
{
    m_rx = rx;
    
    std::ostringstream oss;
    
    Ptr<TocinoNetDevice> rxNetDevice = m_rx->GetTocinoNetDevice();
    TocinoAddress rxAdd = rxNetDevice->GetTocinoAddress();

    oss << "("
        << static_cast<unsigned>( rxAdd.GetX() )
        << ","
        << static_cast<unsigned>( rxAdd.GetY() )
        << ","
        << static_cast<unsigned>( rxAdd.GetZ() )
        << ")";

    m_rxString = oss.str();
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
        return Ptr<NetDevice>( m_tx->GetNetDevice() );
    }
    if (m_rx == NULL) return 0;
    return Ptr<NetDevice>( m_rx->GetNetDevice() );
}

Ptr<TocinoNetDevice> 
TocinoChannel::GetTocinoDevice(uint32_t i) const
{
    if (i == TX_DEV)
    {
        if (m_tx == NULL) return 0;
        return Ptr<TocinoNetDevice>( m_tx->GetTocinoNetDevice() );
    }
    if (m_rx == NULL) return 0;
    return Ptr<TocinoNetDevice>( m_rx->GetTocinoNetDevice() );
}

bool
TocinoChannel::TransmitStart (Ptr<Packet> flit)
{
    Time transmit_time;
  
    NS_ASSERT( m_state == IDLE );

    m_flit = flit;
    
    transmit_time = GetTransmissionTime( flit );
  
    m_totalBytesTransmitted += flit->GetSize();
    m_totalFlitsTransmitted++;

    m_totalTransmitTime += transmit_time;

    if( IsTocinoFlowControlFlit( flit ) )
    {
        m_LLCBytesTransmitted += flit->GetSize();
        m_LLCFlitsTransmitted++;

        m_LLCTransmitTime += transmit_time;
    }

    TocinoOutputVC vc = GetTocinoFlitVirtualChannel( flit );
    m_vcUsageHistogram[ vc.AsUInt32() ]++;

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
                                   m_flit);
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
    uint32_t flits =
        ceil( ( ( (window-noRep).GetDouble() / rep.GetDouble() ) + 1 ) );

    // Add one to account for possibility that receiver
    // may have put a flit on the wire *just* prior to
    // receipt of LLC flit
    flits++;

    //NS_LOG_LOGIC( flits << " buffers required" );

    return flits;
}

uint32_t
TocinoChannel::GetTotalBytesTransmitted() const
{
    return m_totalBytesTransmitted;
}

uint32_t
TocinoChannel::GetTotalFlitsTransmitted() const
{
    return m_totalFlitsTransmitted;
}

Time
TocinoChannel::GetTotalTransmitTime() const
{
    return m_totalTransmitTime;
}

uint32_t
TocinoChannel::GetLLCBytesTransmitted() const
{
    return m_LLCBytesTransmitted;
}

uint32_t
TocinoChannel::GetLLCFlitsTransmitted() const
{
    return m_LLCFlitsTransmitted;
}

Time
TocinoChannel::GetLLCTransmitTime() const
{
    return m_LLCTransmitTime;
}

void
TocinoChannel::ReportStatistics() const
{
    std::ostringstream prefix;
    
    prefix << m_txString << " --> " << m_rxString << ": ";

    // Bytes
    uint32_t dataBytesTransmitted = m_totalBytesTransmitted - m_LLCBytesTransmitted;

    double dataBytesPercentage =
        static_cast<double>(dataBytesTransmitted) / m_totalBytesTransmitted * 100;

    double LLCBytesPercentage =
        static_cast<double>(m_LLCBytesTransmitted) / m_totalBytesTransmitted * 100;

    NS_LOG_LOGIC( prefix.str()
            << "total bytes: "
            << m_totalBytesTransmitted );
    
    NS_LOG_LOGIC( prefix.str() 
            << "data bytes: " 
            << dataBytesTransmitted
            << " ("
            << dataBytesPercentage
            << "%)" );

    NS_LOG_LOGIC( prefix.str()
            << "LLC bytes: "
            << m_LLCBytesTransmitted
            << " ("
            << LLCBytesPercentage
            << "%)" );

    // Flits
    uint32_t dataFlitsTransmitted = m_totalFlitsTransmitted - m_LLCFlitsTransmitted;
    
    double dataFlitsPercentage =
        static_cast<double>(dataFlitsTransmitted) / m_totalFlitsTransmitted * 100;

    double LLCFlitsPercentage =
        static_cast<double>(m_LLCFlitsTransmitted) / m_totalFlitsTransmitted * 100;

    NS_LOG_LOGIC( prefix.str()
            << "total flits: "
            << m_totalFlitsTransmitted );
    
    NS_LOG_LOGIC( prefix.str() 
            << "data flits: " 
            << dataFlitsTransmitted
            << " ("
            << dataFlitsPercentage
            << "%)" );

    NS_LOG_LOGIC( prefix.str()
            << "LLC flits: "
            << m_LLCFlitsTransmitted
            << " ("
            << LLCFlitsPercentage
            << "%)" );

    // Utilization
    Time dataTransmitTime = m_totalTransmitTime - m_LLCTransmitTime;
    Time idleTime = Simulator::Now() - m_totalTransmitTime;
  
    const double totalSeconds = Simulator::Now().GetSeconds();

    double percentTimeBusy = 
        m_totalTransmitTime.GetSeconds() / totalSeconds * 100;

    double percentTimeIdle =
        idleTime.GetSeconds() / totalSeconds * 100;

    double percentTimeXmitData =
        dataTransmitTime.GetSeconds() / totalSeconds * 100;

    double percentTimeXmitLLC =
        m_LLCTransmitTime.GetSeconds() / totalSeconds * 100;

    NS_LOG_LOGIC( prefix.str()
            << "percent busy time: "
            << percentTimeBusy
            << "%" );
    
    NS_LOG_LOGIC( prefix.str()
            << "percent idle time: "
            << percentTimeIdle
            << "%" );
    
    NS_LOG_LOGIC( prefix.str()
            << "percent data time: "
            << percentTimeXmitData
            << "%" );
    
    NS_LOG_LOGIC( prefix.str()
            << "percent LLC time: "
            << percentTimeXmitLLC
            << "%" );

    // Throughput
    double totalMbps =
        static_cast<double>(m_totalBytesTransmitted) * 8 / totalSeconds / 1024 / 1024;
    
    double dataMbps =
        static_cast<double>(dataBytesTransmitted) * 8 / totalSeconds / 1024 / 1024;
    
    double LLCMbps =
        static_cast<double>(m_LLCBytesTransmitted) * 8 / totalSeconds / 1024 / 1024;

    NS_LOG_LOGIC( prefix.str()
            << "total throughput: "
            << totalMbps
            << " Mbps" );
    
    NS_LOG_LOGIC( prefix.str()
            << "data throughput: "
            << dataMbps
            << " Mbps" );
    
    NS_LOG_LOGIC( prefix.str()
            << "LLC throughput: "
            << LLCMbps
            << " Mbps" );
    
    for( uint32_t vc = 0; vc < m_vcUsageHistogram.size(); vc++ )
    {
        NS_LOG_LOGIC( prefix.str()
                << "vc " 
                << vc
                << " "
                << m_vcUsageHistogram[ vc ] );
    }
    
    NS_LOG_LOGIC( prefix.str() );
}

} // namespace ns3
