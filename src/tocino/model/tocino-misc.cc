#include "tocino-misc.h"

#include <ostream>

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

std::string
Tocino3dTorusPortNumberToString( const uint32_t port )
{
    if( port == 6 )
    {
        return "host";
    }

    std::ostringstream oss;

    switch( port / 2 )
    {
        case 0:
            oss << "x";
            break;

        case 1:
            oss << "y";
            break;

        case 2:
            oss << "z";
            break;

        default:
            NS_ASSERT( false );
            oss << "UNKNOWN PORT!";
            break;
    }

    if( port % 2 == 0 )
    {
        oss << "+";
    }
    else
    {
        oss << "-";
    }

    return oss.str();
}

std::string
Tocino3dTorusPortNumberToString( const TocinoInputPort inputPort )
{
    return Tocino3dTorusPortNumberToString( inputPort.AsUInt32() ); 
}

std::string
Tocino3dTorusPortNumberToString( const TocinoOutputPort outputPort )
{
    return Tocino3dTorusPortNumberToString( outputPort.AsUInt32() ); 
}

}
