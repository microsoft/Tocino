Tocino Model Description
----------------------------

.. include:: replace.txt
.. highlight:: cpp

.. heading hierarchy:
   ------------- Chapter
   ************* Section (#.#)
   ============= Subsection (#.#.#)
   ############# Paragraph (no number)

Tocino models direct network topologies -- such as rings, meshes, and tori -- over which Ethernet is encapsulated.  This approach was inspired by the SeaMicro/AMD "Freedom Fabric".

The source code for Tocino lives in the directory ``src/tocino``.

Design
======

Tocino models a lossless direct network.  

Ethernet frames are broken into a sequence of 64B flits by the "flitter".  Typically an Ethernet frame will have a head flit, one or more body flits, and a tail flit.  It is possible for very small Ethernet frames to map to a single "headtail" flit.  

The format of the flits is described by the TocinoFlitHeader class.  Body and tail flits have 2B of header, and up to 62B of payload.  Head flits have an additional 22B of header info, most importantly the source and destination addresses of the packet, and so head flits can carry at most 40B of payload.

Tocino includes a 4-bit virtual channel (VC) designation in each flit, which effectively allows interleaving of packets across a link.  Packets may change VC as they transit the fabric (in fact, this is required for deadlock avoidance).

Addresses in Tocino are 4B each, and contain X, Y, and Z coordinates -- plus some reserved bits.  Since each coordinate gets a byte, the maximum addressible fabric in Tocino is 256^3 or 16M addresses.  The addresses are convertible to/from Ethernet addresses.

Each TocinoNetDevice is both a source/sink and a router of messages in the fabric, and maps 1:1 with an |ns3| Node.  Each has a host port for injection/ejection, and up to 6 ports into the fabric.  This allows us to model a 3D torus, with X+, X-, Y+, Y-, Z+, and Z- ports.

Tocino models channels as unidirectional.  Each TocinoChannel has a transmitter (TocinoTx) from one TocinoNetDevice at one end, and a receiver (TocinoRx) from another TocinoNetDevice at the other end.

In Tocino we have both input and output queues, for different reasons.

Each TocinoTx has a TocinoOutputQueue for each input port and each output virtual-channel (VC).  This is our implementation of "virtual output queues" which allow us to avoid head-of-line blocking.  These queues are mostly for performance.

Each TocinoRx has a TocinoInputQueue for each input VC.  These are prprimarily used for correctness (to tolerate the transmission time of flow-control messages), and should be small.

Flow control is XON/XOFF based, and uses a special flit type for link-layer control (LLC).  TocinoChannel::FlitBuffersRequired function dynamically calculates the input buffering required in order to prevent deadlock.  We simply ensure that there is enough input queue space at the receiver end to "cover" the XOFF message's delay.

We run the flow-control protocol independently for each VC, and each LLC flit can potentially switch the XON/XOFF status of any or all VCs simultaneously.

Routing is done with the very simple dimension-order method, and uses the dateline algorithm to avoid deadlock in topologies that contain cycles.  We attempted to provide some flexibility to allow for fancier routing algorithms in the future.

The TocinoRx calls the router upon receipt of a head flit, to determine the proper route for the flow.  The route is stored in a routing table, to avoid calling the router again on each body flit.

The TocinoCrossbar, which moves filts from the input stage to the output stage, has a fowarding table which is a slightly different concept.  The fowarding table is simply used to prevent interleaving flits from different flows onto the same output port and output VC.  If a flow is already in progress on a given output port / VC combination, we must wait for it to finish before sending a new one.


Scope and Limitations
=====================

Tocino does not yet handle broadcast or multicast.

Head flits have reserved space for a sequence number and timestamp, both of which are currently unused.

The current implementation does not easily scale to a 4th dimension.

Because Tocino is lossless, our NetDevice's Send() function must always return true.  (Returning false seems to be interpreted by |ns3| as a packet drop.)  As a work-around, we have an m_outgoingFlits queue inside the TocinoNetDevice which can effectively grow unbounded.  If |ns3| afforded us a backpressure mechanism other than packet drops, this hack could be removed.
