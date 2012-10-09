/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include "ns3/log.h"
#include "ns3/data-rate.h"
#include "ns3/uinteger.h"
#include "ns3/node.h"
#include "ns3/channel.h"

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
                   UintegerValue (1),
                   MakeUintegerAccessor (&TocinoNetDevice::m_nVCs),
                   MakeUintegerChecker<uint32_t> ());
  return tid;
}

TocinoNetDevice::TocinoNetDevice() :
    m_node( 0 ),
    m_ifIndex( 0 ),
    m_mtu( DEFAULT_MTU ),
    m_nPorts (NPORTS)
{
    m_nEjectedFlits = 0;
}

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
    uint32_t src, dst, i, j;

    // size data structures
    m_queues.resize(m_nPorts*m_nPorts);
    m_receivers.resize(m_nPorts);
    m_transmitters.resize(m_nPorts);

    // create queues - right now 1 per s/d pair
    // rewrite when we add virtual channels
    for (src = 0; src < m_nPorts; src++)
    {
        for (dst = 0; dst < m_nPorts; dst++)
        {
            i = (src * m_nPorts) + dst;
            m_queues[i] = CreateObject<CallbackQueue>();
        }
    }
  
    // create receivers and transmitters
    for (i = 0; i < m_nPorts; i++)
    {
        m_receivers[i] = new TocinoRx(m_nPorts);
        m_receivers[i]->m_tnd = this;
        m_receivers[i]->m_portNumber = i;
        
        m_transmitters[i] = new TocinoTx(m_nPorts);
        m_transmitters[i]->m_tnd = this;
        m_transmitters[i]->m_portNumber = i;
    }
  
    // build linkage between tx, rx, and q
    for (i = 0; i < m_nPorts; i++)
    {
        for (j = 0; j < m_nPorts; j++)
        {
            m_receivers[i]->m_queues[j] = m_queues[(i * m_nPorts) + j];
            m_transmitters[i]->m_queues[j] = m_queues[i + (j * m_nPorts)];
        }
    }

    // configure injection port callback
    //m_injectionPortCallback = MakeCallback(your function goes here);
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

bool TocinoNetDevice::Send( Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber )
{
    return SendFrom( packet, m_address, dest, protocolNumber );
}

std::vector< Ptr<Packet> >
TocinoNetDevice::Flitter( const Ptr<Packet> p, const TocinoAddress& src, const TocinoAddress& dst )
{
    uint32_t start = 0;
    bool isFirstFlit = true;
    
    std::vector< Ptr<Packet> > v;
   
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
        }
        
        if( isLastFlit )
        {
            h.SetTail();
        }

        h.SetLength( LEN );

        flit->AddHeader(h);
            
        v.push_back( flit );

        if( isFirstFlit )
        {
            isFirstFlit = 0;
        }
    
        start += LEN;
        remainder -= LEN;
    }
    while( remainder > 0 );

    NS_ASSERT_MSG( v.size() > 0, "Flitter must always produce at least one flit" );

    return v;
}

Ptr<Packet>
TocinoNetDevice::Deflitter( const std::vector< Ptr<Packet> >& v, TocinoAddress& src, TocinoAddress& dst )
{
    NS_ASSERT_MSG( v.size() > 0, "Can't call deflitter on empty vector" );

    Ptr<Packet> p = Create<Packet>();

    for( unsigned i = 0; i < v.size(); ++i )
    {
        Ptr<Packet> currentFlit = v[i];

        TocinoFlitHeader h;
        currentFlit->RemoveHeader( h );

        const bool isFirst = (i == 0);
        const bool isLast = (i == v.size());

        if( isFirst )
        {
            NS_ASSERT_MSG( h.IsHead(), "First flit must be head flit" );
            src = h.GetSource();
            dst = h.GetDestination();
        }

        if( isLast )
        {
            NS_ASSERT_MSG( h.IsTail(), "Last flit must be tail flit" );
        }

        if( !isFirst && !isLast )
        {
            NS_ASSERT_MSG( h.IsHead() == false, "Body flit cannot be head flit" );
            NS_ASSERT_MSG( h.IsTail() == false, "Body flit cannot be tail flit" );
        }

        p->AddAtEnd( currentFlit );
    }

    return p;
}

bool TocinoNetDevice::SendFrom( Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber )
{
    bool success = m_packetQueue->Enqueue( packet );

    if( !success )
    {
        //FIXME: lossless network is dropping?
        return false;
    }

    if( !m_packetQueue->IsEmpty() )
    {
        Ptr<Packet> currentPacket = m_packetQueue->Dequeue();
        
        std::vector< Ptr<Packet> > flitVector;
        
        flitVector = Flitter( currentPacket,
                TocinoAddress::ConvertFrom( source ), 
                TocinoAddress::ConvertFrom( dest ) );

        // NOW WHAT?
    }
    
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

bool
TocinoNetDevice::InjectFlit(Ptr<Packet> p)
{
  uint32_t injectionPort = m_nPorts-1;
 
  if (m_receivers[injectionPort]->IsBlocked()) return false;
  m_receivers[injectionPort]->Receive(p);
  return true;
}

bool
TocinoNetDevice::EjectFlit(Ptr<Packet> p)
{
    m_nEjectedFlits++;
    NS_LOG_LOGIC("Flit ejected.");
    return true;
}

void
TocinoNetDevice::SetRxChannel(Ptr<TocinoChannel> c, uint32_t port)
{
  // not sure we care about this - linkage is TocinoChannel invoking Receive
  //m_receivers[port]->SetChannel(c);
}

} // namespace ns3
