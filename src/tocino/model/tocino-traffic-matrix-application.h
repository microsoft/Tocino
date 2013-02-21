/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef __TOCINO_TRAFFIC_MATRIX_APPLICATION_H__
#define __TOCINO_TRAFFIC_MATRIX_APPLICATION_H__

#include <vector>
#include <limits>

#include "ns3/ptr.h"
#include "ns3/application.h"
#include "ns3/random-variable.h"
#include "ns3/random-variable-stream.h"
#include "ns3/net-device.h"

namespace ns3
{

class NodeContainer;

typedef std::vector< uint32_t > TocinoTrafficVector;
typedef std::vector< TocinoTrafficVector > TocinoTrafficMatrix;
   
typedef NetDevice::ReceiveCallback ReceiveCallback;

// N.B.
// The function 
//      ns3::UniformVariable::GetInteger(s, l)
// is implemented via call to 
//      GetValue (s, l + 1)
// without check for overflow. This means we must
// not use  
//      std::numeric_limits< uint32_t >::max()
// as the value for TOCINO_TOTAL_TRAFFIC, or else
// the +1 will overflow and we will be picking from
// the range [0,0] and get 0 every time.
// -MAS

const uint32_t TOCINO_TOTAL_TRAFFIC =
    std::numeric_limits< uint32_t >::max() - 1;

class TocinoTrafficMatrixApplication : public Application
{
    public:

    static TypeId GetTypeId();

    TocinoTrafficMatrixApplication();

    void Initialize(
            const uint32_t,
            const NodeContainer*,
            const TocinoTrafficMatrix& );

    void SetPacketSize( uint32_t );

    void SetReceiveCallback( ReceiveCallback cb );

    uint32_t GetPacketsSent() const;
    uint32_t GetPacketsReceived() const;

    void ResetStatistics();

    private:

    static const uint32_t DO_NOT_SEND;

    uint32_t SelectRandomDestination();

    void ScheduleSend();
    
    void StartApplication();
    void StopApplication();
  
    bool AcceptPacket(
            Ptr<NetDevice>,
            Ptr<const Packet>,
            uint16_t,
            const Address& );

    EventId m_sendEvent;
    
    ExponentialRandomVariable
        m_sendIntervalRandomVariable;

    UniformVariable
        m_destinationRandomVariable;

    Time m_meanTimeBetweenSends;
    Time m_maxTimeBetweenSends;

    uint32_t m_packetSize;
  
    uint32_t m_nodeNumber;
    uint32_t m_totalNodes;

    uint32_t m_packetsSent;
    uint32_t m_packetsReceived;

    Ptr<NetDevice> m_netDevice;

    // ISSUE-REVIEW: it would be better if these
    // could be const references
    const NodeContainer* m_nodeContainer;
    const TocinoTrafficVector* m_trafficVector;
    
    ReceiveCallback m_receiveCallback;
};

}

#endif // __TOCINO_TRAFFIC_MATRIX_APPLICATION_H__
