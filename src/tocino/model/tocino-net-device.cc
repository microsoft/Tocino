/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include "ns3/log.h"
#include "ns3/data-rate.h"
#include "ns3/uinteger.h"
#include "ns3/node.h"
#include "ns3/channel.h"
#include "ns3/ethernet-header.h"
#include "ns3/ethernet-trailer.h"

#include "tocino-net-device.h"
#include "tocino-rx.h"
#include "tocino-tx.h"
#include "callback-queue.h"
#include "tocino-channel.h"
#include "tocino-flit-header.h"

NS_LOG_COMPONENT_DEFINE ("TocinoNetDevice");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TocinoNetDevice);

TypeId TocinoNetDevice::GetTypeId(void)
{
    static TypeId tid = TypeId( "ns3::TocinoNetDevice" )
        .SetParent<NetDevice>()
        .AddConstructor<TocinoNetDevice>()
        .AddAttribute ("Ports", 
            "Number of ports on net device.",
            UintegerValue (TocinoNetDevice::NPORTS),
            MakeUintegerAccessor (&TocinoNetDevice::m_nPorts),
            MakeUintegerChecker<uint32_t> ())
        .AddAttribute ("VirtualChannels", 
            "Number of virtual channels on each port.",
            UintegerValue (TocinoNetDevice::NVCS),
            MakeUintegerAccessor (&TocinoNetDevice::m_nVCs),
            MakeUintegerChecker<uint32_t> ());
    return tid;
}

TocinoNetDevice::TocinoNetDevice() :
    m_node( 0 ),
    m_ifIndex( 0 ),
    m_mtu( DEFAULT_MTU ),
    m_nPorts (NPORTS)
{}

TocinoNetDevice::~TocinoNetDevice()
{
    uint32_t i;
    for (i = 0; i < m_nPorts; i++)
    {
        delete m_receivers[i];
        delete m_transmitters[i];
    }
}

void
TocinoNetDevice::Initialize()
{
    uint32_t src, dst, vc, i, j, k;

    // size data structures
    m_queues.resize(m_nPorts*m_nPorts);
    m_receivers.resize(m_nPorts);
    m_transmitters.resize(m_nPorts);

    // create queues
    for (src = 0; src < m_nPorts; src++)
    {
        for (dst = 0; dst < m_nPorts; dst++)
        {
            for (vc = 0; vc < m_nVCs; vc++)
            {
                i = (src * m_nPorts) + (dst * m_nVCs) + vc;
                m_queues[i] = CreateObject<CallbackQueue>();
            }
        }
    }
  
    // create receivers and transmitters
    for (i = 0; i < m_nPorts; i++)
    {
        m_receivers[i] = new TocinoRx(m_nPorts, m_nVCs);
        m_receivers[i]->m_tnd = this;
        m_receivers[i]->m_portNumber = i;
        
        m_transmitters[i] = new TocinoTx(m_nPorts, m_nVCs);
        m_transmitters[i]->m_tnd = this;
        m_transmitters[i]->m_portNumber = i;
    }
  
    // build linkage between tx, rx, and q
    for (i = 0; i < m_nPorts; i++)
    {
        for (j = 0; j < m_nPorts; j++)
        {
            for (vc = 0; vc < m_nVCs; vc++)
            {
                k = (j * m_nVCs) + vc;
                m_receivers[i]->m_queues[k] = m_queues[(i * m_nPorts) + k];
                m_transmitters[i]->m_queues[k] = m_queues[i + (k * m_nPorts)];
            }
        }
    }
}

void TocinoNetDevice::SetIfIndex( const uint32_t index )
{
    m_ifIndex = index;
}

uint32_t TocinoNetDevice::GetIfIndex( void ) const
{ 
    return m_ifIndex;
}

Ptr<Channel> TocinoNetDevice::GetChannel( void ) const
{
    return 0;
}

bool TocinoNetDevice::SetMtu( const uint16_t mtu )
{
    m_mtu = mtu;
    return true;
}

uint16_t TocinoNetDevice::GetMtu( void ) const
{
    return m_mtu;
}

void TocinoNetDevice::SetAddress( Address address )
{
    m_address = TocinoAddress::ConvertFrom( address );
}

Address TocinoNetDevice::GetAddress( void ) const
{
    return m_address;
}

bool TocinoNetDevice::IsLinkUp( void ) const
{
    return true;
}

void TocinoNetDevice::AddLinkChangeCallback( Callback<void> callback )
{
    //Do nothing for now 
}

bool TocinoNetDevice::IsBroadcast( void ) const
{
    return true;
}

Address TocinoNetDevice::GetBroadcast( void ) const
{
    return TocinoAddress::ConvertFrom( Mac48Address ("ff:ff:ff:ff:ff:ff") );
}

bool TocinoNetDevice::IsMulticast( void ) const
{
    return true;
}

Address TocinoNetDevice::GetMulticast( Ipv4Address a ) const
{
    return TocinoAddress::ConvertFrom( Mac48Address::GetMulticast( a ) );
}

Address TocinoNetDevice::GetMulticast( Ipv6Address a ) const
{
    return TocinoAddress::ConvertFrom( Mac48Address::GetMulticast( a ) );
}

bool TocinoNetDevice::IsPointToPoint( void ) const
{
    return false;
}

bool TocinoNetDevice::IsBridge( void ) const
{
    return false;
}

std::deque< Ptr<Packet> >
TocinoNetDevice::Flitter( const Ptr<Packet> p, const TocinoAddress& src, const TocinoAddress& dst, const TocinoFlitHeader::Type type )
{
    uint32_t start = 0;
    bool isFirstFlit = true;
    
    std::deque< Ptr<Packet> > q;
   
    uint32_t remainder = p->GetSize(); // GetSerializedSize?

    do
    {
        Ptr<Packet> flit;
        
        const uint32_t LIMIT = isFirstFlit ? 
            TocinoFlitHeader::MAX_PAYLOAD_HEAD :
            TocinoFlitHeader::MAX_PAYLOAD_OTHER;
        
        const bool isLastFlit = (remainder <= LIMIT);
       
        const uint32_t LEN = isLastFlit ? remainder : LIMIT;
            
        flit = p->CreateFragment( start, LEN );
    
        TocinoFlitHeader h( src, dst );
            
        if( isFirstFlit )
        {
            h.SetHead();
            h.SetType( type );
        }
        
        if( isLastFlit )
        {
            h.SetTail();
        }

        h.SetLength( LEN );

        flit->AddHeader(h);
            
        q.push_back( flit );

        if( isFirstFlit )
        {
            isFirstFlit = 0;
        }
    
        start += LEN;
        remainder -= LEN;
    }
    while( remainder > 0 );

    NS_ASSERT_MSG( q.size() > 0, "Flitter must always produce at least one flit" );

    return q;
}

bool TocinoNetDevice::Send( Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber )
{
    return SendFrom( packet, m_address, dest, protocolNumber );
}

bool TocinoNetDevice::SendFrom( Ptr<Packet> packet, const Address& src, const Address& dest, uint16_t protocolNumber )
{
    // Avoid modifying the passed-in packet.
    Ptr<Packet> p = packet->Copy();

    TocinoAddress source = TocinoAddress::ConvertFrom(src);
    TocinoAddress destination = TocinoAddress::ConvertFrom(dest);

    EthernetHeader eh( false );
    EthernetTrailer et;

    eh.SetSource( source.AsMac48Address() );
    eh.SetDestination( destination.AsMac48Address() );
    eh.SetLengthType( protocolNumber );

    p->AddHeader( eh );
    p->AddTrailer( et );
    
    // FIXME: this can grow unbounded?
    m_packetQueue.push_back( p );
    NS_ASSERT_MSG( m_packetQueue.size() < 10000, "Crazy large packet queue?" );

    if( m_outgoingFlits.empty() )
    {
        NS_ASSERT( m_packetQueue.empty() == false );

        Ptr<Packet> currentPacket = m_packetQueue.front();
        m_packetQueue.pop_front();
    
        m_outgoingFlits = Flitter( currentPacket, source, destination, TocinoFlitHeader::ETHERNET );
   
        InjectFlits();
    }

    // Tocino is a lossless network
    return true;
}

Ptr<Node>
TocinoNetDevice::GetNode( void ) const {return m_node;}

void
TocinoNetDevice::SetNode( Ptr<Node> node ) {m_node = node;}

bool
TocinoNetDevice::NeedsArp( void ) const {return true;}

void
TocinoNetDevice::SetReceiveCallback( NetDevice::ReceiveCallback cb )
{
    m_rxCallback = cb;
}

void
TocinoNetDevice::SetPromiscReceiveCallback( PromiscReceiveCallback cb )
{
    m_promiscRxCallback = cb;
}

bool TocinoNetDevice::SupportsSendFrom( void ) const
{
    return true;
}

void
TocinoNetDevice::SetTxChannel(Ptr<TocinoChannel> c, uint32_t port)
{
  m_transmitters[port]->SetChannel(c);
}

void TocinoNetDevice::InjectFlits()
{
    while( !m_outgoingFlits.empty() &&
        !m_receivers[injectionPortNumber()]->IsBlocked() )
    {
        m_receivers[injectionPortNumber()]->Receive( m_outgoingFlits.front() );
        m_outgoingFlits.pop_front();
    }
}

void TocinoNetDevice::EjectFlit( Ptr<Packet> flit )
{
    NS_LOG_LOGIC("Flit ejected.");

    static Ptr<Packet> incomingPacket( NULL );
    static TocinoAddress src;

    TocinoFlitHeader h;
    flit->RemoveHeader( h );
    
    if( incomingPacket == NULL )
    {
        NS_ASSERT_MSG( h.IsHead(), "First flit must be head flit" );
        NS_ASSERT_MSG( h.GetDestination() == m_address,
            "Ejected packet for foreign address?" );

        //NS_ASSERT_MSG( h.GetType() == TocinoFlitHeader::ETHERNET,
        //    "Ejected packet type is not ethernet?" );
        
        incomingPacket = flit;
        src = h.GetSource();
    }
    else
    {
        NS_ASSERT( !h.IsHead() );
        incomingPacket->AddAtEnd( flit );
    }

    if( h.IsTail() )
    {
        NS_ASSERT( incomingPacket != NULL );
    
        EthernetHeader eh;
        EthernetTrailer et;

        incomingPacket->RemoveHeader( eh );
        incomingPacket->RemoveTrailer( et );

        NS_ASSERT_MSG( eh.GetSource() == src.AsMac48Address(),
            "Encapsulated Ethernet frame has a difference source than head flit" );
        
        NS_ASSERT_MSG( eh.GetDestination() == m_address.AsMac48Address(),
            "Encapsulated Ethernet frame has a foreign destination address?" );

        m_rxCallback( this, incomingPacket, eh.GetLengthType(), src );

        incomingPacket = NULL;
    }
}

void
TocinoNetDevice::SetRxChannel(Ptr<TocinoChannel> c, uint32_t port)
{
  // not sure we care about this - linkage is TocinoChannel invoking Receive
  //m_receivers[port]->SetChannel(c);
}

} // namespace ns3
