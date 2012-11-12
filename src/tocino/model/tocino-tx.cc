/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include <cstdio>

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/data-rate.h"

#include "tocino-tx.h"
#include "tocino-rx.h"
#include "callback-queue.h"
#include "tocino-net-device.h"
#include "tocino-channel.h"
#include "tocino-misc.h"

NS_LOG_COMPONENT_DEFINE ("TocinoTx");

#ifdef NS_LOG_APPEND_CONTEXT
#pragma push_macro("NS_LOG_APPEND_CONTEXT")
#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT \
    { std::clog << "(" \
                << (int) m_tnd->m_address.GetX() << "," \
                << (int) m_tnd->m_address.GetY() << "," \
                << (int) m_tnd->m_address.GetZ() << ") " \
                << m_portNumber << " "; }
#endif

namespace ns3 {

TocinoTx::TocinoTx( Ptr<TocinoNetDevice> tnd )
    : m_portNumber( TOCINO_INVALID_PORT )
    , m_xstate( TocinoFlowControl::XON )
    , m_packet( NULL )
    , m_state( IDLE )
    , m_pending_xon( false )
    , m_pending_xoff( false )
    , m_tnd( tnd )
    , m_queues( tnd->GetNPorts() * tnd->GetNVCs() )
    , m_channel( NULL )
{}

TocinoTx::~TocinoTx()
{}

void TocinoTx::SetXState(TocinoFlowControl::State s)
{
    m_xstate = s;
    if (m_xstate == TocinoFlowControl::XON)
    {
        NS_LOG_LOGIC("transmitter xstate now XON");
    }
    if (m_xstate == TocinoFlowControl::XOFF)
    {
        NS_LOG_LOGIC("transmitter xstate now XOFF");
    }
}

TocinoFlowControl::State TocinoTx::GetXState()
{
    return m_xstate;
}

void TocinoTx::SetChannel(Ptr<TocinoChannel> channel)
{
    m_channel = channel;
}

void TocinoTx::SendXOFF()
{
    // attempts to set both pending flags simultaneously can happen:
    // a buffer fills - generates XOFF - and then is popped - generates XON - 
    // before transmitter becomes !BUSY and can schedule LLC
    if (m_pending_xon) 
    {
        m_pending_xon = false;
        NS_LOG_LOGIC("clearing pending XON");
    }
    else
    {
        m_pending_xoff = true;
        NS_LOG_LOGIC("pending XOFF");
    }
}

void TocinoTx::SendXON()
{
    if (m_pending_xoff)
    {
        m_pending_xoff = false;
        NS_LOG_LOGIC("clearing pending XOFF");
    }
    else
    {
        m_pending_xon = true;
        NS_LOG_LOGIC("pending XON");
    }
}

Ptr<NetDevice> TocinoTx::GetNetDevice()
{
    return m_tnd;
}

void
TocinoTx::TransmitEnd()
{
  m_state = IDLE;
  Transmit();
}

void
TocinoTx::Transmit()
{
    NS_LOG_FUNCTION_NOARGS();

    Time transmit_time;
    Ptr<Packet> p = 0;
    uint32_t winner, rx_port;
    
    if (m_state == BUSY) 
    {
        //NS_LOG_LOGIC("transmitter BUSY");
        return;
    }
    NS_ASSERT_MSG(!(m_pending_xoff && m_pending_xon), "race condition detected");

    // send an XOFF if one is pending
    if (m_pending_xoff)
    {
        m_pending_xoff = false;
        if (m_portNumber == m_tnd->GetHostPort())
        {
            // do nothing on an injection port stall
            // this case is handled by the transmitter
        }
        else
        {
            p = TocinoFlowControl::GetXOFFPacket();

            // reflect xstate of transmitter on far end of channel in local rx
            // assumption: the rx[i] and tx[i] in this NetDevice connect to some
            // tx[j] and rx[j] on the NetDevice at far end of channel
            // THIS IS VERY IMPORTANT and probably should be checked at construction time
            // an alternative would be to infer the appropriate receiver to signal
            m_tnd->m_receivers[m_portNumber]->SetXState(TocinoFlowControl::XOFF);
            NS_LOG_LOGIC( "sending XOFF(" << p << ")" );
        }
    }
    
    if (!p && m_pending_xon)
    {
        m_pending_xon = false;
        if (m_portNumber == m_tnd->GetHostPort())
        {
            // do nothing on an injection port resume
            // this case is handled by the transmitter
        }
        else
        {
            p = TocinoFlowControl::GetXONPacket();

            // same state reflection as described above
            m_tnd->m_receivers[m_portNumber]->SetXState(TocinoFlowControl::XON);
            NS_LOG_LOGIC( "sending XON(" << p << ")" );
        }
    }
    
    if (!p)
    {
        if (m_xstate == TocinoFlowControl::XON) // legal to transmit
        {
            winner = Arbitrate();
            if (winner < m_queues.size())
            {
                m_state = BUSY; // this acts as a mutex on Transmit

                // if we've unblocked the winner receive port we need to cause an XON
                // to be scheduled on its corresponding transmit port (hide the crud in
                // TocinoNetDeviceReceiver::CheckForUnblock())
                //
                // check for full must occur before CheckForUnblock but Dequeue must occur
                // whether the queue was full or not
                if (m_queues[winner]->IsFull())
                {
                    rx_port = m_tnd->QueueToPort( winner );
                    
                    if (rx_port == m_tnd->GetHostPort()) // special handling
                    {
                        bool was_blocked, is_blocked;
                    
                        // detect transition from blocked to unblocked
                        was_blocked = m_tnd->m_receivers[rx_port]->IsBlocked();
                        p = m_queues[winner]->Dequeue();
                        is_blocked = m_tnd->m_receivers[rx_port]->IsBlocked();

                        if (was_blocked && !is_blocked) // if injection process had stalled
                        {
                            m_tnd->SendFlits();
                        }
                    }
                    else
                    {
                        p = m_queues[winner]->Dequeue();
                        //NS_LOG_LOGIC("request CheckForUnblock");
                        m_tnd->m_receivers[rx_port]->CheckForUnblock();
                    }
                }
                else
                {
                    p = m_queues[winner]->Dequeue();
                }
                NS_ASSERT_MSG (p, "queue underrun " << winner);
            }
            else
            {
                // nothing to send - idle link
                NS_LOG_LOGIC("nothing to send");
            }
        }
        else 
        {
            //NS_LOG_LOGIC("transmitter is XOFF");
        }
    }

    if (p) // send the packet onward
    {
        if (m_portNumber == m_tnd->GetHostPort()) // connected to ejection port
        {
            // ejection port modeled as having infinite bandwidth and buffering
            // need to keep m_state == BUSY to this point to prevent reentrancy
            m_state = IDLE;
            m_tnd->EjectFlit(p); // eject the packet
        }
        else
        {
            NS_ASSERT_MSG (m_channel, "undefined channel, port " << m_portNumber);

            // send packet to channel
            m_state = BUSY;
            m_channel->TransmitStart(p);

            transmit_time= m_channel->GetTransmissionTime(p);
            NS_LOG_LOGIC("transmitting " << p << " for " << transmit_time);
            Simulator::Schedule(transmit_time, &TocinoTx::TransmitEnd, this);
        }
    }
}

uint32_t
TocinoTx::Arbitrate()
{
  uint32_t i;

    // trivial arbitration - obvious starvation concern
    for (i = 0; i < m_queues.size(); i++)
    {
        if (m_queues[i]->IsEmpty() == false) return i;
    }
    return m_queues.size(); // nothing pending
}

} // namespace ns3

#pragma pop_macro("NS_LOG_APPEND_CONTEXT")
