#include "tocino-misc.h"

#include "ns3/simulator.h"
#include "ns3/simulator-impl.h"
#include "ns3/log.h"

namespace
{

void TocinoTimePrinter( std::ostream &os )
{
    // Coerce logging to display nanoseconds
    os << ns3::Simulator::Now().GetNanoSeconds() << "ns";
}

}

namespace ns3
{

void TocinoCustomizeLogging()
{
    // HACK: Make the Simulator initialize itself
    // so we can override its preferred time printer
    Ptr<SimulatorImpl> hack = Simulator::GetImplementation();
    LogSetTimePrinter( &TocinoTimePrinter );
}

}
