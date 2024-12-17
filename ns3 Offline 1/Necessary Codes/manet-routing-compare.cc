#include "ns3/aodv-module.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/dsdv-module.h"
#include "ns3/dsr-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/olsr-module.h"
#include "ns3/yans-wifi-helper.h"

#include <fstream>
#include <iostream>

using namespace ns3;
using namespace dsr;

NS_LOG_COMPONENT_DEFINE("manet-routing-compare");

class RoutingExperiment
{
  public:
    RoutingExperiment();
    void Run();
    void CommandSetup(int argc, char** argv);
    Ptr<Socket> SetupPacketReceive(Ipv4Address addr, Ptr<Node> node);
    void ReceivePacket(Ptr<Socket> socket);
    void CheckThroughput();
    // //void CalculateMetrics(FlowMonitorHelper& flowmonHelper,
    //                       Ptr<FlowMonitor> flowMonitor,
    //                       double Totaltime);

    uint32_t port{9};
    uint32_t bytesTotal{0};
    uint32_t packetsReceived{0};
    std::string m_CSVfileName{"manet-routing.output.csv"};
    int m_nSinks{10};
    int m_numberOfNodes{50};
    int m_packetsPerSecond{4};
    int nodeSpeed{20};
    int nodePause{0};
    std::string m_protocolName{"AODV"};
    double m_txp{7.5};
    bool m_traceMobility{false};
    bool m_flowMonitor{true};
};

RoutingExperiment::RoutingExperiment()
{
}

static inline std::string
PrintReceivedPacket(Ptr<Socket> socket, Ptr<Packet> packet, Address senderAddress)
{
    std::ostringstream oss;

    oss << Simulator::Now().GetSeconds() << " " << socket->GetNode()->GetId();

    if (InetSocketAddress::IsMatchingType(senderAddress))
    {
        InetSocketAddress addr = InetSocketAddress::ConvertFrom(senderAddress);
        oss << " received one packet from " << addr.GetIpv4();
    }
    else
    {
        oss << " received one packet!";
    }
    return oss.str();
}

void
RoutingExperiment::ReceivePacket(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address senderAddress;
    while ((packet = socket->RecvFrom(senderAddress)))
    {
        bytesTotal += packet->GetSize();
        packetsReceived += 1;
        // NS_LOG_UNCOND(PrintReceivedPacket(socket, packet, senderAddress));
    }
}

void
RoutingExperiment::CheckThroughput()
{
    double kbs = (bytesTotal * 8.0) / 1000;
    bytesTotal = 0;

    // Write throughput data to CSV
    // std::ofstream out(m_CSVfileName, std::ios::app);
    // out << (Simulator::Now()).GetSeconds() << "," << kbs << "," << packetsReceived << ","
    //     << m_nSinks << "," << m_protocolName << "," << m_txp << std::endl;
    // out.close();
    packetsReceived = 0;
    Simulator::Schedule(Seconds(1.0), &RoutingExperiment::CheckThroughput, this);
}

Ptr<Socket>
RoutingExperiment::SetupPacketReceive(Ipv4Address addr, Ptr<Node> node)
{
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    Ptr<Socket> sink = Socket::CreateSocket(node, tid);
    InetSocketAddress local = InetSocketAddress(addr, port);
    sink->Bind(local);
    sink->SetRecvCallback(MakeCallback(&RoutingExperiment::ReceivePacket, this));
    return sink;
}


void
RoutingExperiment::CommandSetup(int argc, char** argv)
{
    CommandLine cmd(__FILE__);
    cmd.AddValue("numberOfNodes", "Number of nodes", m_numberOfNodes);
    cmd.AddValue("packetsPerSecond", "Number of packets generated per second", m_packetsPerSecond);
    cmd.AddValue("nodeSpeed", "Speed of nodes in m/s", nodeSpeed);
    cmd.Parse(argc, argv);
}

int
main(int argc, char* argv[])
{
    RoutingExperiment experiment;
    experiment.CommandSetup(argc, argv);
    experiment.Run();

    return 0;
}

void
RoutingExperiment::Run()
{
    Packet::EnablePrinting();
    if (!std::ifstream(m_CSVfileName).good())
    {
        std::ofstream out(m_CSVfileName);
        out << "NumOfNodes,PacketsPerSec,NodeSpeed,Throughput,EndToEndDelay,PacketDeliveryRatio,"
               "PacketDropRatio"
            << std::endl;
        out.close();
    }

    double TotalTime = 200.0;
    double startTime = 100.0;
    int rateVal = 64 * 8 * m_packetsPerSecond;
    std::string rate(std::to_string(rateVal) + "bps");
    std::string phyMode("DsssRate11Mbps");
    std::string tr_name("manet-routing-compare");

    Config::SetDefault("ns3::OnOffApplication::PacketSize", StringValue("64"));
    Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue(rate));

    Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue(phyMode));

    NodeContainer adhocNodes;
    adhocNodes.Create(m_numberOfNodes);

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);

    YansWifiPhyHelper wifiPhy;
    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel");
    wifiPhy.SetChannel(wifiChannel.Create());

    WifiMacHelper wifiMac;
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue(phyMode),
                                 "ControlMode",
                                 StringValue(phyMode));
    wifiPhy.Set("TxPowerStart", DoubleValue(m_txp));
    wifiPhy.Set("TxPowerEnd", DoubleValue(m_txp));

    wifiMac.SetType("ns3::AdhocWifiMac");
    NetDeviceContainer adhocDevices = wifi.Install(wifiPhy, wifiMac, adhocNodes);

    MobilityHelper mobilityAdhoc;
    int64_t streamIndex = 0;

    ObjectFactory pos;
    pos.SetTypeId("ns3::RandomRectanglePositionAllocator");
    pos.Set("X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=300.0]"));
    pos.Set("Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1500.0]"));

    Ptr<PositionAllocator> taPositionAlloc = pos.Create()->GetObject<PositionAllocator>();
    streamIndex += taPositionAlloc->AssignStreams(streamIndex);

    std::stringstream ssSpeed;
    ssSpeed << "ns3::UniformRandomVariable[Min=0.0|Max=" << nodeSpeed << "]";
    std::stringstream ssPause;
    ssPause << "ns3::ConstantRandomVariable[Constant=" << nodePause << "]";
    mobilityAdhoc.SetMobilityModel("ns3::RandomWaypointMobilityModel",
                                   "Speed",
                                   StringValue(ssSpeed.str()),
                                   "Pause",
                                   StringValue(ssPause.str()),
                                   "PositionAllocator",
                                   PointerValue(taPositionAlloc));
    mobilityAdhoc.SetPositionAllocator(taPositionAlloc);
    mobilityAdhoc.Install(adhocNodes);
    streamIndex += mobilityAdhoc.AssignStreams(adhocNodes, streamIndex);

    AodvHelper aodv;
    InternetStackHelper internet;
    Ipv4ListRoutingHelper list;
    list.Add(aodv, 100);
    internet.SetRoutingHelper(list);
    internet.Install(adhocNodes);

    Ipv4AddressHelper addressAdhoc;
    addressAdhoc.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer adhocInterfaces;
    adhocInterfaces = addressAdhoc.Assign(adhocDevices);

    OnOffHelper onoff1("ns3::UdpSocketFactory", Address());
    onoff1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1.0]"));
    onoff1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.0]"));

    m_nSinks = m_numberOfNodes / 2;

    for (int i = 0; i < m_nSinks; i++)
    {
        Ptr<Socket> sink = SetupPacketReceive(adhocInterfaces.GetAddress(i), adhocNodes.Get(i));

        AddressValue remoteAddress(InetSocketAddress(adhocInterfaces.GetAddress(i), port));
        onoff1.SetAttribute("Remote", remoteAddress);

        Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable>();
        ApplicationContainer temp = onoff1.Install(adhocNodes.Get(i + m_nSinks));
        temp.Start(Seconds(var->GetValue(100.0, 101.0)));
        temp.Stop(Seconds(TotalTime));
    }

    std::stringstream ss;
    ss << m_numberOfNodes;
    std::string nodes = ss.str();

    std::stringstream ss2;
    ss2 << nodeSpeed;
    std::string sNodeSpeed = ss2.str();

    std::stringstream ss3;
    ss3 << nodePause;
    std::string sNodePause = ss3.str();

    std::stringstream ss4;
    ss4 << rate;
    std::string sRate = ss4.str();

    // NS_LOG_INFO("Configure Tracing.");
    // tr_name = tr_name + "_" + m_protocolName +"_" + nodes + "nodes_" + sNodeSpeed + "speed_" +
    // sNodePause + "pause_" + sRate + "rate";

    // AsciiTraceHelper ascii;
    // Ptr<OutputStreamWrapper> osw = ascii.CreateFileStream(tr_name + ".tr");
    // wifiPhy.EnableAsciiAll(osw);
    AsciiTraceHelper ascii;
    MobilityHelper::EnableAsciiAll(ascii.CreateFileStream(tr_name + ".mob"));

    FlowMonitorHelper flowmonHelper;
    Ptr<FlowMonitor> flowmon;
    if (m_flowMonitor)
    {
        flowmon = flowmonHelper.InstallAll();
    }

    NS_LOG_INFO("Run Simulation.");

    //CheckThroughput();

    Simulator::Stop(Seconds(TotalTime));
    Simulator::Run();

    if (m_flowMonitor)
    {
        flowmon->CheckForLostPackets();
        FlowMonitor::FlowStatsContainer stats = flowmon->GetFlowStats();

        double throughput = 0.0;
        double delay = 0.0;
        double packetDeliveryRatio = 0.0;
        double packetDropRatio = 0.0;

        // Iterate over flow stats to extract the necessary metrics
        uint64_t totalBytes = 0;
        uint64_t totalPackets = 0;
        uint64_t totalDroppedPackets = 0;
        uint64_t totalReceivedPackets = 0;
        double totalDelay = 0;

        for (const auto &entry : stats)
        {
            FlowMonitor::FlowStats flowStats = entry.second;
            //throughput += (flowStats.rxBytes * 8.0)/(flowStats. timeLastRxPacket.GetSeconds() - flowStats.timeFirstTxPacket.GetSeconds()) / 1000; // kbps
            totalBytes += flowStats.rxBytes;
            totalPackets += flowStats.txPackets;
            totalDroppedPackets += flowStats.lostPackets;
            totalReceivedPackets += flowStats.rxPackets;
            totalDelay += flowStats.delaySum.GetSeconds();
        }

        double simulationTimeInSeconds = Simulator::Now().GetSeconds();
        throughput = (double)(totalBytes * 8.0) /(double)((TotalTime - startTime) *1024.0);// kbps
        packetDeliveryRatio = (double)((totalReceivedPackets * 1.0) * 100.0) / (double)totalPackets;
        packetDropRatio = (double)((totalDroppedPackets * 1.0) * 100.0 )/ (double)totalPackets;
        delay = totalDelay / (double)totalReceivedPackets;

        // Write the metrics to the CSV file
        std::ofstream out(m_CSVfileName, std::ios::app);
        out << m_numberOfNodes << "," << m_packetsPerSecond << "," << nodeSpeed << "," << throughput
            << "," << delay << "," << packetDeliveryRatio << "," << packetDropRatio << std::endl;
        out.close();
        flowmon->SerializeToXmlFile(tr_name + ".flowmon", false, false);
    }

    Simulator::Destroy();

    // FlowMonitorHelper flowmonHelper;
    // Ptr<FlowMonitor> flowMonitor = flowmonHelper.InstallAll();

    // Simulator::Stop(Seconds(TotalTime));
    // Simulator::Run();

    // std::cout << "Simulation completed" << std::endl;

    // CalculateMetrics(flowmonHelper, flowMonitor, TotalTime);

    // Simulator::Destroy();
}
