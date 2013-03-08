/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "tocino-traffic-matrix-application.h"

#include "ns3/node-container.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/log.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED( TocinoTrafficMatrixApplication );

TypeId 
TocinoTrafficMatrixApplication::GetTypeId()
{
    static TypeId tid = TypeId( "ns3::TocinoTrafficMatrixApplication" )
        .SetParent<Application>()
        .AddConstructor<TocinoTrafficMatrixApplication>()
        .AddAttribute(
                "MeanTimeBetweenSends", 
                "Mean time between Send operations, exponential distribution.",
                TimeValue( Time( "1ms" ) ),
                MakeTimeAccessor(
                    &TocinoTrafficMatrixApplication::m_meanTimeBetweenSends ),
                MakeTimeChecker() )
        .AddAttribute(
                "MaxTimeBetweenSends",
                "Max delay between Send operations.",
                TimeValue( Time( "100ms" ) ),
                MakeTimeAccessor(
                    &TocinoTrafficMatrixApplication::m_maxTimeBetweenSends ),
                MakeTimeChecker() )
        .AddAttribute(
                "PacketSize",
                "Send size in bytes.",
                UintegerValue( 0 ),
                MakeUintegerAccessor(
                    &TocinoTrafficMatrixApplication::m_packetSize ),
                MakeUintegerChecker< uint32_t >() )
        .AddAttribute( "EnableValiantLoadBalancing", 
            "Bounce every flow through an intermediate node.",
            BooleanValue( false ),
            MakeBooleanAccessor( &TocinoTrafficMatrixApplication::m_doVLB),
            MakeBooleanChecker() )
        ;
    return tid;
}

TocinoTrafficMatrixApplication::TocinoTrafficMatrixApplication()
    : m_meanTimeBetweenSends( Time( "1ms" ) )
    , m_maxTimeBetweenSends( Time( "100ms" ) )
    , m_packetSize( 0 )
    , m_nodeNumber( 0 )
    , m_totalNodes( 0 )
    , m_packetsSent( 0 )
    , m_packetsReceived( 0 )
    , m_netDevice( NULL )
    , m_nodeContainer( NULL )
    , m_receiveCallback( NULL )
    , m_doVLB( false )
{};

void 
TocinoTrafficMatrixApplication::Initialize(
        const uint32_t nodeNumber,
        const NodeContainer* nodeContainer,
        const TocinoTrafficMatrix& trafficMatrix )
{
    NS_ASSERT( nodeContainer != NULL );
   
    m_nodeNumber = nodeNumber;
    m_nodeContainer = nodeContainer;
    m_trafficVector = &trafficMatrix[m_nodeNumber];

    m_totalNodes = nodeContainer->GetN();

    NS_ASSERT( trafficMatrix.size() == m_totalNodes );
    NS_ASSERT( m_nodeNumber < m_totalNodes );

    // Validate that traffic matrix is square
    for( uint32_t i = 0; i < m_totalNodes; ++i )
    {
        NS_ASSERT( trafficMatrix[i].size() == m_totalNodes );
    }
    
    Ptr<Node> node = m_nodeContainer->Get( m_nodeNumber );
    NS_ASSERT( node->GetNDevices() == 1 );

    // N.B.
    // This will throw an exception if the NetDevice aggregated
    // to the node is not actually a TocinoNetDevice.  This is
    // by design.  We want to call private APIs.  -MAS

    m_netDevice = DynamicCast<TocinoNetDevice>( node->GetDevice(0) );

    m_netDevice->SetReceiveCallback( 
            MakeCallback( &TocinoTrafficMatrixApplication::AcceptPacket, this ) );

    m_sendIntervalRandomVariable =
        CreateObject<ExponentialRandomVariable>();

    m_destinationRandomVariable =
        CreateObject<UniformRandomVariable>();
}

void
TocinoTrafficMatrixApplication::SetPacketSize(
        uint32_t packetSize )
{
    m_packetSize = packetSize;
}

void
TocinoTrafficMatrixApplication::SetReceiveCallback(
        ReceiveCallback receiveCallback )
{
    m_receiveCallback = receiveCallback;
}

uint32_t
TocinoTrafficMatrixApplication::GetPacketsSent() const
{
    return m_packetsSent;
}

uint32_t
TocinoTrafficMatrixApplication::GetPacketsReceived() const
{
    return m_packetsReceived;
}

void
TocinoTrafficMatrixApplication::ResetStatistics()
{
    m_packetsSent = 0;
    m_packetsReceived = 0;
}

void
TocinoTrafficMatrixApplication::StartApplication()
{
    NS_ASSERT( m_totalNodes > 0 );
    NS_ASSERT( m_packetSize > 0 );

    ScheduleSend();
}

bool
TocinoTrafficMatrixApplication::AcceptPacket(
        Ptr<NetDevice> nd,
        Ptr<const Packet> p,
        uint16_t t,
        const Address& src )
{
    m_packetsReceived++;

    if( !m_receiveCallback.IsNull() )
    {
        m_receiveCallback( nd, p, t, src );
    }

    return true;
}

const uint32_t TocinoTrafficMatrixApplication::DO_NOT_SEND =
    std::numeric_limits< uint32_t >::max();

uint32_t
TocinoTrafficMatrixApplication::SelectRandomDestination()
{
    uint32_t rand =
        m_destinationRandomVariable->GetInteger( 0, TOCINO_TOTAL_TRAFFIC );

    uint32_t lowerBound = 0;
    uint32_t upperBound = 0;

    for( uint32_t destNum = 0; destNum < m_trafficVector->size(); ++destNum )
    {
        upperBound += (*m_trafficVector)[destNum];
        
        NS_ASSERT( upperBound <= TOCINO_TOTAL_TRAFFIC );

        if( ( rand >= lowerBound ) && 
            ( rand < upperBound ) )
        {
            return destNum;
        }

        lowerBound = upperBound;
    }

    return DO_NOT_SEND;
}

void
TocinoTrafficMatrixApplication::ScheduleSend()
{
    NS_ASSERT( m_sendEvent.IsExpired() );

    const uint32_t destNum = SelectRandomDestination();
        
    if( destNum != DO_NOT_SEND )
    {
        Ptr<Node> destNode = m_nodeContainer->Get( destNum );
        Address destAddress = destNode->GetDevice(0)->GetAddress();

        Ptr<Packet> packet = Create<Packet>( m_packetSize );

        if( !m_doVLB )
        {
            m_netDevice->Send( packet, destAddress, 0 );
        }
        else
        {
            Ptr<Node> viaNode =
                m_nodeContainer->Get( 
                    m_destinationRandomVariable->GetInteger( 
                        0, m_trafficVector->size()-1 ) );

            Address viaAddress = viaNode->GetDevice(0)->GetAddress();

            m_netDevice->SendVia( packet, destAddress, viaAddress, 0 );
        }

        m_packetsSent++;
    }

    Time dt = Time(
            m_sendIntervalRandomVariable->GetValue(
                m_meanTimeBetweenSends.GetDouble(),
                m_maxTimeBetweenSends.GetDouble() ) );
    
    m_sendEvent = Simulator::Schedule(
            dt,
            &TocinoTrafficMatrixApplication::ScheduleSend,
            this );
}

void
TocinoTrafficMatrixApplication::StopApplication()
{
    Simulator::Cancel( m_sendEvent );
}

}
