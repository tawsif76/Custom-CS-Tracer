#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/point-to-point-module.h"

#include <fstream>
#include "ns3/ndnSIM/NFD/daemon/fw/forwarder.hpp"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("cacheTest");

std::ofstream csTraceFile;

// --- Custom Tracer Function ---
// Reads internal NFD counters (Cumulative Hits/Misses) directly from the Forwarder
void CustomCsTrace(NodeContainer& nodes) {
    if (csTraceFile.tellp() == 0) {
        csTraceFile << "Time\tNode\tHits\tMisses" << std::endl;
    }

    for (NodeContainer::Iterator it = nodes.Begin(); it != nodes.End(); ++it) {
        Ptr<Node> node = *it;
 
        Ptr<ns3::ndn::L3Protocol> l3 = node->GetObject<ns3::ndn::L3Protocol>();
        
        if (l3 != nullptr) {
            // Access NFD Forwarder and read counters
            auto forwarder = l3->getForwarder();
            auto& counters = forwarder->getCounters();
            
            csTraceFile << Simulator::Now().GetSeconds() << "\t"
                        << node->GetId() << "\t"
                        << counters.nCsHits << "\t"
                        << counters.nCsMisses << std::endl;
        }
    }

    Simulator::Schedule(Seconds(1.0), &CustomCsTrace, nodes);
}

namespace ns3 {
    namespace ndn {
        int main (int argc, char* argv[]) {
            double zipfAlpha = 1.2; // Default value for Zipf skew parameter
            CommandLine cmd;
            cmd.AddValue("alpha", "Zipf Skew Parameter", zipfAlpha);
            cmd.Parse(argc, argv);

            // Ensure the directory 'scratch/cacheTest' exists, or change this to just "custom-cs-trace.txt"
            csTraceFile.open("scratch/cacheTest/custom-cs-trace.txt", std::ios::out);

            std::cout << "[LOG] Actual Alpha Value " << zipfAlpha << "\n";
            NodeContainer nodes;
            nodes.Create(4);
            
            PointToPointHelper p2p;
            p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
            p2p.SetChannelAttribute("Delay", StringValue("2ms"));
            p2p.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("20p"));

            // Connect 0 <-> 1 <-> 2 <-> 3
            p2p.Install(nodes.Get(0), nodes.Get(1));
            p2p.Install(nodes.Get(1), nodes.Get(2));
            p2p.Install(nodes.Get(2), nodes.Get(3));


            NodeContainer routers;
            routers.Add(nodes.Get(1));
            routers.Add(nodes.Get(2));

            StackHelper routerHelper;
            routerHelper.setPolicy("nfd::cs::lru");
            routerHelper.setCsSize(100); // Routers get memory
            routerHelper.Install(routers);


            NodeContainer endpoints;
            endpoints.Add(nodes.Get(0));
            endpoints.Add(nodes.Get(3));

            StackHelper endpointHelper;
            endpointHelper.setCsSize(0); // Consumers/Producers have NO cache
            endpointHelper.Install(endpoints);
            
   

            StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");

            AppHelper consumerHelper("ns3::ndn::ConsumerZipfMandelbrot");
            consumerHelper.SetPrefix("/prefix");
            consumerHelper.SetAttribute("Frequency", StringValue("30"));
            consumerHelper.SetAttribute("NumberOfContents", StringValue("100"));
            consumerHelper.SetAttribute("q", StringValue("0.7"));
            consumerHelper.SetAttribute("s", StringValue(std::to_string(zipfAlpha)));
            consumerHelper.Install(nodes.Get(0));

            AppHelper producerHelper("ns3::ndn::Producer");
            producerHelper.SetPrefix("/prefix");
            producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
            producerHelper.Install(nodes.Get(3));


            FibHelper::AddRoute(nodes.Get(0), "/prefix", nodes.Get(1), 1);
            FibHelper::AddRoute(nodes.Get(1), "/prefix", nodes.Get(2), 1);
            FibHelper::AddRoute(nodes.Get(2), "/prefix", nodes.Get(3), 1);

            Simulator::Stop(Seconds(50.0));

            Simulator::Schedule(Seconds(1.0), &CustomCsTrace, nodes);
            
            Simulator::Run();
            Simulator::Destroy();

            csTraceFile.close();

            return 0;
        }
    }
}

int main(int argc, char* argv[]) {
  return ns3::ndn::main(argc, argv);
}