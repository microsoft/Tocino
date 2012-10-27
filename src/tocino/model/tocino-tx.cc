/* -*- Mode:C++; c-file-style:"microsoft"; indent-tabs-mode:nil; -*- */

#include <cstdio>

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/data-rate.h"

#include "tocino-tx.h"
#include "tocino-rx.h"
#include "callback-queue.h"
#include "tocino-net-device.h"
#include "tocino-channel.h"

NS_LOG_COMPONENT_DEFINE ("TocinoTx");

namespace ns3 {

TocinoTx::TocinoTx(uint32_t nPorts, uint32_t nVCs)
{
  m_portNumber = 0xffffffff;
  m_xstate = TocinoFlowControl::XON;
  m_state = IDLE;
  m_pending_xon = false;
  m_pending_xoff = false;
  m_queues.resize(nPorts*nVCs);
}

TocinoTx::~TocinoTx()
{}

void TocinoTx::SetXState(TocinoFlowControl::State s)
{
    m_xstate = s;
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
    m_pending_xoff = true;
}

void TocinoTx::SendXON()
{
    m_pending_xon = true;
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
    Time transmit_time;
    Ptr<Packet> p = 0;
//    Ptr<const Packet> p = 0;
    uint32_t winner, rx_port;
//    char str[64];
    
    //NS_LOG_FUNCTION(this->m_portNumber);

    if (m_state == BUSY) 
    {
//        NS_LOG_LOGIC("transmitter " << m_portNumber << " BUSY");
        return;
    }
    
    NS_ASSERT_MSG(!(m_pending_xoff && m_pending_xon), "race condition detected");
    
    // send an XOFF if one is pending
    if (m_pending_xoff)
    {
        m_pending_xoff = false;
        if (m_portNumber == m_tnd->injectionPortNumber())
        {
            // do nothing on an injection port stall
            // this case is handled by the transmitter
        }
        else
        {
            if (m_xstate == TocinoFlowControl::XON) // only send if we're currently enabled
            {
                //              sprintf(str,"sending XOFF on transmitter %d", m_portNumber);
                //NS_LOG_LOGIC(str);
                p = TocinoFlowControl::GetXOFFPacket();
            }
        }
        //sprintf(str, "pending XOFF processed for port %d", m_portNumber);
        //NS_LOG_LOGIC(str);
    }
    
    if (!p && m_pending_xon)
    {
        m_pending_xon = false;
        if (m_portNumber == m_tnd->injectionPortNumber())
        {
            // do nothing on an injection port resume
            // this case is handled by the transmitter
        }
        else
        {
            if (m_xstate == TocinoFlowControl::XOFF) // only send if we're currently disabled
            {
                //      sprintf(str,"sending XON on transmitter %d", m_portNumber);
                //NS_LOG_LOGIC(str);
                p = TocinoFlowControl::GetXONPacket();
            }
        }
        //sprintf(str, "pending XON processed for port %d", m_portNumber);
        //NS_LOG_LOGIC(str);
    }
    
    if (!p && (m_xstate == TocinoFlowControl::XON)) // legal to transmit
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
                rx_port = winner/m_tnd->m_nVCs;
                if (rx_port == m_tnd->injectionPortNumber()) // special handling
                {
                    bool was_blocked, is_blocked;
                    
                    // detect transition from blocked to unblocked
                    was_blocked = m_tnd->m_receivers[rx_port]->IsBlocked();
                    p = m_queues[winner]->Dequeue();
                    is_blocked = m_tnd->m_receivers[rx_port]->IsBlocked();

                    if (was_blocked && !is_blocked) // restart injection process if it had stalled
                    {
                        // kick the net device
                        //              NS_LOG_LOGIC ("kicking InjectFlits");
                        //m_tnd->InjectFlits();
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
    }
    else
    {
//        NS_LOG_LOGIC("transmitter in XOFF state");
    }

    if (p) // send the packet onward
    {
        if (m_portNumber == m_tnd->ejectionPortNumber()) // connected to ejection port
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
            //      sprintf(str, "packet 0x%08x to channel", (uint32_t)PeekPointer(p));
            //NS_LOG_LOGIC(str);
            m_state = BUSY;
            m_channel->TransmitStart(p);

            transmit_time= m_channel->GetTransmissionTime(p);
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
