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
            MakeTypeIdChecker ());
    return tid;
}

TocinoNetDevice::TocinoNetDevice() :
    m_node( NULL ),
    m_ifIndex( 0 ),
    m_mtu( DEFAULT_MTU ),
    m_address( 0 ),
    m_incomingPacket( NULL ),
    m_incomingSource( 0 ),
    m_rxCallback( NULL ),
    m_promiscRxCallback( NULL ),
    m_nPorts( DEFAULT_NPORTS ),
    m_nVCs( DEFAULT_NVCS ),
    m_routerTypeId( TocinoDimensionOrderRouter::GetTypeId() ),
    m_router( NULL )
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
    uint32_t vc, i, j, k, base;

    // size data structures
    m_queues.resize(m_nPorts*m_nPorts*m_nVCs);
    m_receivers.resize(m_nPorts);
    m_transmitters.resize(m_nPorts);

    // create queues
    // each port has nVCs queues to each port
    for (i = 0; i < (m_nPorts * m_nPorts * m_nVCs); i++)
    {
        m_queues[i] = CreateObject<CallbackQueue>();
    }
  
    // create receivers and transmitters
    for (i = 0; i < m_nPorts; i++)
    {
        m_receivers[i] = new TocinoRx( this );
        m_receivers[i]->m_portNumber = i;
        
        m_transmitters[i] = new TocinoTx( this );
        m_transmitters[i]->m_portNumber = i;
    }
  
    // build linkage between rx and q
    // each rx uses a block of nPorts*nVCs queues
    // block for rx i is based at i*nPorts*nVCs
    for (i = 0; i < m_nPorts; i++)
    {
        base = i * m_nPorts * m_nVCs;
        for (j = 0; j < (m_nPorts * m_nVCs); j++)
        {
            m_receivers[i]->m_queues[j] = m_queues[base + j];
        }
    }

    // build linkage between tx and q
    // queues for tx are a set of nPorts blocks
    // blocks for tx i are based at (i*nVCs)+(j*nPorts*nVCs); 0 < j < nPorts
    // each block is nVCs in size
    for (i = 0; i < m_nPorts; i++)
    {
        for (j = 0; j < m_nPorts; j++)
        {
            base = (i * m_nVCs) + (j * m_nPorts * m_nVCs);
            k = (j * m_nVCs);
            for (vc = 0; vc < m_nVCs; vc++)
            {
                // assign names to queues to help debug
                // better name would include TocinoNetDevice id
                char str[32];
                sprintf(str,"%d:%d_%d", j, vc, i); // q name - src:vc_dst
                m_queues[base + vc]->SetName(str);

                m_transmitters[i]->m_queues[k + vc] = m_queues[base + vc];
            }
        }
    }

    ObjectFactory routerFactory;
    routerFactory.SetTypeId( m_routerTypeId );

    m_router = routerFactory.Create<TocinoRouter>();
    m_router->Initialize( this );

    NS_ASSERT( m_router != NULL );
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
    NS_LOG_FUNCTION(this->m_node << this->m_ifIndex);

    while( !m_outgoingFlits.empty() &&
        !m_receivers[ GetHostPort() ]->IsBlocked() )
    {
        Ptr<Packet> p;
        TocinoFlitHeader h;
        m_outgoingFlits.front()->PeekHeader(h);
        
        if (h.IsHead() && h.IsTail())
        {
            NS_LOG_LOGIC("flit injected: singleton");
        }
        else if (h.IsHead())
        {
            NS_LOG_LOGIC("flit injected: head");
        }
        else if (h.IsTail())
        {
            NS_LOG_LOGIC("flit injected: tail");
        }
        else
        {
            NS_LOG_LOGIC("flit injected:body");
        }

        // must pop prior to calling Receive; Receive can indirectly generate
        // a call to InjectFlits which can cause a flit to be sent twice if pop
        // occurs after Receive
        p = m_outgoingFlits.front();
        m_outgoingFlits.pop_front();
        m_receivers[ GetHostPort() ]->Receive(p);
    }
}

void TocinoNetDevice::EjectFlit( Ptr<Packet> f )
{
    NS_LOG_FUNCTION((uint32_t)PeekPointer(f));

    TocinoFlitHeader h;
    f->RemoveHeader( h );
    
    if( m_incomingPacket == NULL )
    {
        NS_ASSERT_MSG( h.IsHead(), "First flit must be head flit" );
        NS_ASSERT_MSG( h.GetDestination() == m_address,
            "Ejected packet for foreign address?" );

        NS_ASSERT_MSG( h.GetType() == TocinoFlitHeader::ETHERNET,
            "Ejected packet type is not ethernet?" );
        
        m_incomingPacket = f;
        m_incomingSource = h.GetSource();
    }
    else
    {
        NS_ASSERT( !h.IsHead() );
        m_incomingPacket->AddAtEnd( f );
    }

    if( h.IsTail() )
    {
        NS_ASSERT( m_incomingPacket != NULL );
    
        EthernetHeader eh;
        EthernetTrailer et;

        m_incomingPacket->RemoveHeader( eh );
        m_incomingPacket->RemoveTrailer( et );

        NS_ASSERT_MSG( eh.GetSource() == m_incomingSource.AsMac48Address(),
            "Encapsulated Ethernet frame has a difference source than head flit" );
        
        NS_ASSERT_MSG( eh.GetDestination() == m_address.AsMac48Address(),
            "Encapsulated Ethernet frame has a foreign destination address?" );
        m_rxCallback( this, m_incomingPacket, eh.GetLengthType(), m_incomingSource );
        m_incomingPacket = NULL;
    }
}

TocinoRx*
TocinoNetDevice::GetReceiver(uint32_t p) const
{
    return m_receivers[p];
}

TocinoTx*
TocinoNetDevice::GetTransmitter(uint32_t p) const
{
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
TocinoNetDevice::PortToQueue( uint32_t port ) const
{
    return port * m_nVCs;
}

uint32_t
TocinoNetDevice::QueueToPort( uint32_t queue ) const
{
    return queue / m_nVCs;
}

uint32_t
TocinoNetDevice::GetHostPort() const
{
    return m_nPorts - 1;
}

Ptr<TocinoRouter>
TocinoNetDevice::GetRouter() const
{
    return m_router;
}

void
TocinoNetDevice::SetRouter( Ptr<TocinoRouter> r )
{
    m_router = r;
}

bool TocinoNetDevice::AllQuiet() const
{
    bool quiet = true;

    if( !m_packetQueue.empty() ) 
    {
        NS_LOG_LOGIC( "Not quiet: SendFrom() in progress?" );
        quiet = false;
    }

    if( !m_outgoingFlits.empty() )
    {
        NS_LOG_LOGIC( "Not quiet: InjectFlits() in progress?" );
        quiet = false;
    }

    if( m_incomingPacket != NULL )
    {
        NS_LOG_LOGIC( "Not quiet: EjectFlits() in progress?" );
        quiet = false;
    }
    
    for (unsigned i = 0; i < m_nPorts; i++)
    {
        if( m_transmitters[i]->GetXState() == TocinoFlowControl::XOFF )
        {
            NS_LOG_LOGIC( "Not quiet: m_transmitters[" << i << "] is XOFF" );
            quiet = false;
        }
    }
    
    for (int i = 0; i < (m_nPorts * m_nPorts * m_nVCs); i++)
    {
        if( !m_queues[i]->IsEmpty() )
        {
            NS_LOG_LOGIC( "Not quiet: m_queue[" << i << "] not empty" );
            quiet = false;
        }
    }

    return quiet;
}

} // namespace ns3
