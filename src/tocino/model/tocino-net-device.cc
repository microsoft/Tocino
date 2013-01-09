/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include <cstdio>

#include "ns3/log.h"
#include "ns3/data-rate.h"
#include "ns3/uinteger.h"
#include "ns3/node.h"
#include "ns3/channel.h"
#include "ns3/ethernet-header.h"
#include "ns3/ethernet-trailer.h"
#include "ns3/pointer.h"
#include "ns3/object-factory.h"

#include "tocino-net-device.h"
#include "tocino-rx.h"
#include "tocino-tx.h"
#include "callback-queue.h"
#include "tocino-channel.h"
#include "tocino-flit-header.h"
#include "tocino-dimension-order-router.h"
#include "tocino-simple-arbiter.h"
#include "tocino-flit-id-tag.h"

NS_LOG_COMPONENT_DEFINE ("TocinoNetDevice");

#ifdef NS_LOG_APPEND_CONTEXT
#pragma push_macro("NS_LOG_APPEND_CONTEXT")
#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT \
    { std::clog << "(" \
                << (int) m_address.GetX() << "," \
                << (int) m_address.GetY() << "," \
                << (int) m_address.GetZ() << ") "; }
#endif

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TocinoNetDevice);

TypeId TocinoNetDevice::GetTypeId(void)
{
    static TypeId tid = TypeId( "ns3::TocinoNetDevice" )
        .SetParent<NetDevice>()
        .AddConstructor<TocinoNetDevice>()
        .AddAttribute ("Ports", 
            "Number of ports on net device.",
            UintegerValue (TocinoNetDevice::DEFAULT_NPORTS),
            MakeUintegerAccessor (&TocinoNetDevice::m_nPorts),
            MakeUintegerChecker<uint32_t> ())
        .AddAttribute ("VirtualChannels", 
            "Number of virtual channels on each port.",
            UintegerValue (TocinoNetDevice::DEFAULT_NVCS),
            MakeUintegerAccessor (&TocinoNetDevice::m_nVCs),
            MakeUintegerChecker<uint32_t> ())
        .AddAttribute ("RouterType",
            "Name of type which implements routing algorithm.",
            TypeIdValue (TocinoDimensionOrderRouter::GetTypeId ()),
            MakeTypeIdAccessor (&TocinoNetDevice::m_routerTypeId),
            MakeTypeIdChecker ())
        .AddAttribute ("ArbiterType",
            "Name of type which implements arbitration algorithm.",
            TypeIdValue (TocinoSimpleArbiter::GetTypeId ()),
            MakeTypeIdAccessor (&TocinoNetDevice::m_arbiterTypeId),
            MakeTypeIdChecker ());
    return tid;
}

TocinoNetDevice::TocinoNetDevice()
    : m_node( NULL )
    , m_ifIndex( 0 )
    , m_mtu( DEFAULT_MTU )
    , m_address( 0 )
    , m_rxCallback( NULL )
    , m_promiscRxCallback( NULL )
    , m_nPorts( DEFAULT_NPORTS )
    , m_nVCs( DEFAULT_NVCS )
    , m_routerTypeId( TocinoDimensionOrderRouter::GetTypeId() )
    , m_arbiterTypeId( TocinoSimpleArbiter::GetTypeId() )
#ifdef TOCINO_VC_STRESS_MODE
    , m_flowCounter( 0 )
#endif
{}

TocinoNetDevice::~TocinoNetDevice()
{
    NS_ASSERT( AllQuiet() );

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
    ObjectFactory routerFactory;
    routerFactory.SetTypeId( m_routerTypeId );

    ObjectFactory arbiterFactory;
    arbiterFactory.SetTypeId( m_arbiterTypeId );

    // size data structures
    m_incomingPackets.resize(m_nVCs, NULL);
    m_incomingSources.resize(m_nVCs);
    m_receivers.resize(m_nPorts);
    m_transmitters.resize(m_nPorts);

    // create queues
    m_queues.resize( m_nPorts );
    for( uint32_t rx_port = 0; rx_port < m_nPorts; ++rx_port )
    {
        m_queues[rx_port].resize( m_nPorts );
        for( uint32_t tx_port = 0; tx_port < m_nPorts; ++tx_port )
        {
            m_queues[rx_port][tx_port].resize( m_nVCs );
            for( uint8_t rx_vc = 0; rx_vc < m_nVCs; ++rx_vc )
            {
                m_queues[rx_port][tx_port][rx_vc].resize( m_nVCs );
                for( uint8_t tx_vc = 0; tx_vc < m_nVCs; ++tx_vc )
                {
                    m_queues[rx_port][tx_port][rx_vc][tx_vc]
                        = CreateObject<CallbackQueue>();
                }
            }
        }    
    }
  
    // create receivers and routers
    // create transmitters and arbiters
    for ( uint32_t i = 0; i < m_nPorts; i++)
    {
        Ptr<TocinoArbiter> arbiter = arbiterFactory.Create<TocinoArbiter>();
        Ptr<TocinoRouter> router = routerFactory.Create<TocinoRouter>();

        // MAS - must create transmitter first
        m_transmitters[i] = new TocinoTx( i, this, arbiter );
        m_receivers[i] = new TocinoRx( i, this, router );

        arbiter->Initialize( this, m_transmitters[i] );
        router->Initialize( this, m_receivers[i] );
    }
  
    // build linkage between rx, tx and queues
    for( uint32_t rx_port = 0; rx_port < m_nPorts; ++rx_port )
    {
        for( uint32_t tx_port = 0; tx_port < m_nPorts; ++tx_port )
        {
            for( uint8_t rx_vc = 0; rx_vc < m_nVCs; ++rx_vc )
            {
                for( uint8_t tx_vc = 0; tx_vc < m_nVCs; ++tx_vc )
                {
                    Ptr<CallbackQueue> queue
                        = m_queues[rx_port][tx_port][rx_vc][tx_vc];

                    m_receivers[rx_port]->SetQueue( tx_port, rx_vc, tx_vc, queue );
                    m_transmitters[tx_port]->SetQueue( rx_port, rx_vc, tx_vc, queue );
                }
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

TocinoAddress TocinoNetDevice::GetTocinoAddress( void ) const
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

#ifdef TOCINO_VC_STRESS_MODE
        uint8_t vc = m_flowCounter % m_nVCs;
#endif

        if( isFirstFlit )
        {
            h.SetHead();
            h.SetType( type );
        }
        
        if( isLastFlit )
        {
            h.SetTail();
#ifdef TOCINO_VC_STRESS_MODE
            // Round-robin across all the VCs
            m_flowCounter++;
#endif
        }

        h.SetLength( LEN );

#ifdef TOCINO_VC_STRESS_MODE
        h.SetVirtualChannel( vc );
#endif

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

    const uint32_t ABS_PACKET_NUM = TocinoFlitIdTag::NextPacketNumber();
    const uint32_t TOTAL_FLITS = q.size();

    for( uint32_t i = 0; i < TOTAL_FLITS; ++i )
    {
        const uint32_t REL_FLIT_NUM = i+1;

        TocinoFlitIdTag tag( ABS_PACKET_NUM, REL_FLIT_NUM, TOTAL_FLITS );
        q[i]->AddPacketTag( tag );
    }

    return q;
}

bool TocinoNetDevice::Send( Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber )
{
    return SendFrom( packet, m_address, dest, protocolNumber );
}

bool TocinoNetDevice::SendFrom( Ptr<Packet> packet, const Address& src, const Address& dest, uint16_t protocolNumber )
{
    TocinoAddress source = TocinoAddress::ConvertFrom(src);
    TocinoAddress destination = TocinoAddress::ConvertFrom(dest);

    NS_LOG_FUNCTION( packet << src << dest << protocolNumber );

    // Avoid modifying the passed-in packet.
    Ptr<Packet> p = packet->Copy();


    EthernetHeader eh( false );
    EthernetTrailer et;

    eh.SetSource( source.AsMac48Address() );
    eh.SetDestination( destination.AsMac48Address() );
    eh.SetLengthType( protocolNumber );

    p->AddHeader( eh );
    p->AddTrailer( et );
        
    FlittizedPacket fp = Flitter( p, source, destination, TocinoFlitHeader::ETHERNET );

    // add the new flits to the end of the outgoing flit Q
    m_outgoingFlits.insert( m_outgoingFlits.end(), fp.begin(), fp.end() );

    // FIXME: this can grow unbounded?
    
    // FIXME: temporarily disable assert so we can run bigger sims.  Ultimately,
    // we will have "applications" that will hook into the event system and
    // call SendFrom() incrementally over the course of the sim.

    //NS_ASSERT_MSG( m_outgoingFlits.size() < 10000, "Crazy large packet queue?" );

    TrySendFlits();

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

void TocinoNetDevice::InjectFlit( Ptr<Packet> f ) const
{
    const bool head = IsTocinoFlitHead( f );
    const bool tail = IsTocinoFlitTail( f );

    if( head && tail )  
    {
        NS_LOG_LOGIC( GetTocinoFlitIdString( f ) << " singleton" );
    }
    else if( head ) 
    {
        NS_LOG_LOGIC( GetTocinoFlitIdString( f ) << " head" );
    }
    else if( tail )
    {
        NS_LOG_LOGIC( GetTocinoFlitIdString( f ) << " tail" );
    }
    else
    {
        NS_ASSERT( !head && !tail );
        NS_LOG_LOGIC( GetTocinoFlitIdString( f ) << " body" );
    }

    m_receivers[ GetHostPort() ]->Receive(f);
}

void TocinoNetDevice::TrySendFlits()
{
    NS_LOG_FUNCTION_NOARGS();
    
    if( m_outgoingFlits.empty() )
    {
        NS_LOG_LOGIC( "nothing to do" );
        return;
    }

    NS_ASSERT( !m_outgoingFlits.empty() );

    // ISSUE-REVIEW:
    // We should probably be checking IsVCBlocked() here
    // and passing the injection VC.

    while( !m_outgoingFlits.empty() &&
        !m_receivers[ GetHostPort() ]->IsAnyQueueBlocked() )
    {
        // must pop prior to calling InjectFlit; InjectFlit can indirectly generate
        // a call to TrySendFlits which can cause a flit to be sent twice if pop
        // occurs after InjectFlits
        Ptr<Packet> f = m_outgoingFlits.front();
        m_outgoingFlits.pop_front();
        InjectFlit(f);
    }
}

void TocinoNetDevice::EjectFlit( Ptr<Packet> f )
{
    NS_LOG_FUNCTION( GetTocinoFlitIdString( f ) );
    
    TocinoFlitHeader h;
    f->RemoveHeader( h );
  
    NS_ASSERT( m_incomingPackets.size() == m_nVCs );
    NS_ASSERT( m_incomingSources.size() == m_nVCs );

    const uint8_t vc = h.GetVirtualChannel();

    Ptr< Packet >& pkt = m_incomingPackets[vc];
    TocinoAddress& src = m_incomingSources[vc];

    if( pkt == NULL )
    {
        NS_ASSERT_MSG( h.IsHead(), "First flit must be head flit" );
        NS_ASSERT_MSG( h.GetDestination() == m_address,
            "Ejected packet for foreign address?" );

        NS_ASSERT_MSG( h.GetType() == TocinoFlitHeader::ETHERNET,
            "Ejected packet type is not ethernet?" );
        
        pkt = f;
        src = h.GetSource();
    }
    else
    {
        NS_ASSERT( !h.IsHead() );
        pkt->AddAtEnd( f );
    }
        
    NS_ASSERT( pkt != NULL );

    if( h.IsTail() )
    {
        EthernetHeader eh;
        EthernetTrailer et;

        pkt->RemoveHeader( eh );
        pkt->RemoveTrailer( et );

        NS_ASSERT_MSG( eh.GetSource() == src.AsMac48Address(),
            "Encapsulated Ethernet frame has a difference source than head flit" );
        
        NS_ASSERT_MSG( eh.GetDestination() == m_address.AsMac48Address(),
            "Encapsulated Ethernet frame has a foreign destination address?" );

        TocinoFlitIdTag tag;
        bool success = pkt->RemovePacketTag( tag );
        NS_ASSERT_MSG( success == true, "Expected TocinoFlitIdTag" );

        m_rxCallback( this, pkt, eh.GetLengthType(), src );
        pkt = NULL;
    }
}

TocinoRx*
TocinoNetDevice::GetReceiver(uint32_t p) const
{
    NS_ASSERT( p < m_receivers.size() );
    return m_receivers[p];
}

TocinoTx*
TocinoNetDevice::GetTransmitter(uint32_t p) const
{
    NS_ASSERT( p < m_receivers.size() );
    return m_transmitters[p];
}

uint32_t
TocinoNetDevice::GetNPorts() const
{ 
    return m_nPorts;
}

uint32_t
TocinoNetDevice::GetNVCs() const
{
    return m_nVCs;
}

uint32_t
TocinoNetDevice::GetNQueues() const
{
    return m_nPorts * m_nVCs;
}

uint32_t
TocinoNetDevice::GetHostPort() const
{
    return m_nPorts - 1;
}

bool TocinoNetDevice::AllQuiet() const
{
    bool quiet = true;

   
    if( !m_outgoingFlits.empty() )
    {
        NS_LOG_LOGIC( "Not quiet: TrySendFlits() in progress?" );
        quiet = false;
    }

    for (unsigned i = 0; i < m_incomingPackets.size(); i++)
    {
        if( m_incomingPackets[i] != NULL )
        {
            NS_LOG_LOGIC( "Not quiet: EjectFlits() in progress?" );
            quiet = false;
        }
    }

    for (unsigned i = 0; i < m_nPorts; i++)
    {
        if( m_transmitters[i]->IsAnyVCPaused() )
        {
            NS_LOG_LOGIC( "Not quiet: m_transmitters[" << i << "] is XOFF" );
            quiet = false;
        }
    }
    
    for( uint32_t rx_port = 0; rx_port < m_nPorts; ++rx_port )
    {
        for( uint32_t tx_port = 0; tx_port < m_nPorts; ++tx_port )
        {
            for( uint32_t rx_vc = 0; rx_vc < m_nVCs; ++rx_vc )
            {
                for( uint32_t tx_vc = 0; tx_vc < m_nVCs; ++tx_vc )
                {
                    Ptr<CallbackQueue> queue
                        = m_queues[rx_port][tx_port][rx_vc][tx_vc];

                    if( !queue->IsEmpty() )
                    {
                        NS_LOG_LOGIC( "Not quiet: "
                                << "m_queue" 
                                << "[" << rx_port << "]" 
                                << "[" << tx_port << "]" 
                                << "[" << rx_vc << "]" 
                                << "[" << tx_vc << "]" 
                                << " not empty" );

                        quiet = false;
                    }
                }
            }
        }
    }
	
    return quiet;
}

void
TocinoNetDevice::DumpState() const
{
    for (unsigned i = 0; i < m_nPorts; i++)
    {
        m_transmitters[i]->DumpState();
    }

    for (unsigned i = 0; i < m_nPorts; i++)
    {
        m_receivers[i]->DumpState();
    }
}

#pragma pop_macro("NS_LOG_APPEND_CONTEXT")

} // namespace ns3
