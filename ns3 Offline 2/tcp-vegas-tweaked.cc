/*
 * Copyright (c) 2016 ResiliNets, ITTC, University of Kansas
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Truc Anh N. Nguyen <annguyen@ittc.ku.edu>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  https://resilinets.org/
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 */

#include "tcp-vegas-tweaked.h"

#include "tcp-socket-state.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TcpVegasTweaked");
NS_OBJECT_ENSURE_REGISTERED(TcpVegasTweaked);

TypeId
TcpVegasTweaked::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TcpVegasTweaked")
                            .SetParent<TcpNewReno>()
                            .AddConstructor<TcpVegasTweaked>()
                            .SetGroupName("Internet")
                            .AddAttribute("Alpha",
                                          "Lower bound of packets in network",
                                          UintegerValue(2),
                                          MakeUintegerAccessor(&TcpVegasTweaked::m_alpha),
                                          MakeUintegerChecker<uint32_t>())
                            .AddAttribute("Beta",
                                          "Upper bound of packets in network",
                                          UintegerValue(4),
                                          MakeUintegerAccessor(&TcpVegasTweaked::m_beta),
                                          MakeUintegerChecker<uint32_t>())
                            .AddAttribute("Gamma",
                                          "Limit on increase",
                                          UintegerValue(1),
                                          MakeUintegerAccessor(&TcpVegasTweaked::m_gamma),
                                          MakeUintegerChecker<uint32_t>());
    return tid;
}

TcpVegasTweaked::TcpVegasTweaked()
    : TcpNewReno(),
      m_alpha(2),
      m_beta(4),
      m_gamma(1),
      m_baseRtt(Time::Max()),
      m_minRtt(Time::Max()),
      m_cntRtt(0),
      m_prevThroughput(0),
      m_currentThroughput(0),
      m_doingVegasNow(true),
      m_begSndNxt(0)
{
    NS_LOG_FUNCTION(this);
}

TcpVegasTweaked::TcpVegasTweaked(const TcpVegasTweaked& sock)
    : TcpNewReno(sock),
      m_alpha(sock.m_alpha),
      m_beta(sock.m_beta),
      m_gamma(sock.m_gamma),
      m_baseRtt(sock.m_baseRtt),
      m_minRtt(sock.m_minRtt),
      m_cntRtt(sock.m_cntRtt),
      m_doingVegasNow(true),
      m_begSndNxt(0)
{
    NS_LOG_FUNCTION(this);
}

TcpVegasTweaked::~TcpVegasTweaked()
{
    NS_LOG_FUNCTION(this);
}

Ptr<TcpCongestionOps>
TcpVegasTweaked::Fork()
{
    return CopyObject<TcpVegasTweaked>(this);
}

void
TcpVegasTweaked::PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt)
{
    NS_LOG_FUNCTION(this << tcb << segmentsAcked << rtt);

    if (rtt.IsZero())
    {
        return;
    }

    m_minRtt = std::min(m_minRtt, rtt);
    NS_LOG_DEBUG("Updated m_minRtt = " << m_minRtt);

    m_baseRtt = std::min(m_baseRtt, rtt);
    NS_LOG_DEBUG("Updated m_baseRtt = " << m_baseRtt);

    // Update RTT counter
    m_cntRtt++;
    NS_LOG_DEBUG("Updated m_cntRtt = " << m_cntRtt);
}

void
TcpVegasTweaked::EnableVegas(Ptr<TcpSocketState> tcb)
{
    NS_LOG_FUNCTION(this << tcb);

    m_doingVegasNow = true;
    m_begSndNxt = tcb->m_nextTxSequence;
    m_cntRtt = 0;
    m_minRtt = Time::Max();
    m_prevThroughput = 0;
    m_currentThroughput = 0;
}

void
TcpVegasTweaked::DisableVegas()
{
    NS_LOG_FUNCTION(this);

    m_doingVegasNow = false;
}

void
TcpVegasTweaked::CongestionStateSet(Ptr<TcpSocketState> tcb,
                                    const TcpSocketState::TcpCongState_t newState)
{
    NS_LOG_FUNCTION(this << tcb << newState);
    if (newState == TcpSocketState::CA_OPEN)
    {
        EnableVegas(tcb);
    }
    else
    {
        DisableVegas();
    }
}

void
TcpVegasTweaked::IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
    NS_LOG_FUNCTION(this << tcb << segmentsAcked);

    if (!m_doingVegasNow)
    {
        // If Vegas is not on, we follow NewReno algorithm
        NS_LOG_LOGIC("Vegas is not turned on, we follow NewReno algorithm.");
        TcpNewReno::IncreaseWindow(tcb, segmentsAcked);
        return;
    }

    if (tcb->m_lastAckedSeq >= m_begSndNxt)
    { // A Vegas cycle has finished, we do Vegas cwnd adjustment every RTT.

        NS_LOG_LOGIC("A Vegas cycle has finished, we adjust cwnd once per RTT.");

        // Save the current right edge for next Vegas cycle
        m_begSndNxt = tcb->m_nextTxSequence;

        /*
         * We perform Vegas calculations only if we got enough RTT samples to
         * insure that at least 1 of those samples wasn't from a delayed ACK.
         */
        if (m_cntRtt <= 2)
        { // We do not have enough RTT samples, so we should behave like Reno
            NS_LOG_LOGIC(
                "We do not have enough RTT samples to do Vegas, so we behave like NewReno.");
            TcpNewReno::IncreaseWindow(tcb, segmentsAcked);
        }
        else
        {
            NS_LOG_LOGIC("We have enough RTT samples to perform Vegas calculations");
            /*
             * We have enough RTT samples to perform Vegas algorithm.
             * Now we need to determine if cwnd should be increased or decreased
             * based on the calculated difference between the expected rate and actual sending
             * rate and the predefined thresholds (alpha, beta, and gamma).
             */
            uint32_t diff;
            uint32_t targetCwnd;
            uint32_t segCwnd = tcb->GetCwndInSegments();
            m_currentThroughput = segCwnd / m_minRtt.GetSeconds();
            /*
             * Calculate the cwnd we should have. baseRtt is the minimum RTT
             * per-connection, minRtt is the minimum RTT in this window
             *
             * little trick:
             * desidered throughput is currentCwnd * baseRtt
             * target cwnd is throughput / minRtt
             */
            double tmp = m_baseRtt.GetSeconds() / m_minRtt.GetSeconds();
            targetCwnd = static_cast<uint32_t>(segCwnd * tmp);
            NS_LOG_DEBUG("Calculated targetCwnd = " << targetCwnd);
            NS_ASSERT(segCwnd >= targetCwnd); // implies baseRtt <= minRtt

            /*
             * Calculate the difference between the expected cWnd and
             * the actual cWnd
             */
            diff = segCwnd - targetCwnd;
            NS_LOG_DEBUG("Calculated diff = " << diff);

            if (diff > m_gamma && (tcb->m_cWnd < tcb->m_ssThresh))
            {
                /*
                 * We are going too fast. We need to slow down and change from
                 * slow-start to linear increase/decrease mode by setting cwnd
                 * to target cwnd. We add 1 because of the integer truncation.
                 */
                NS_LOG_LOGIC("We are going too fast. We need to slow down and "
                             "change to linear increase/decrease mode.");
                segCwnd = std::min(segCwnd, targetCwnd + 1);
                tcb->m_cWnd = segCwnd * tcb->m_segmentSize;
                tcb->m_ssThresh = GetSsThresh(tcb, 0);
                NS_LOG_DEBUG("Updated cwnd = " << tcb->m_cWnd << " ssthresh=" << tcb->m_ssThresh);
            }
            else if (tcb->m_cWnd < tcb->m_ssThresh)
            { // Slow start mode
                NS_LOG_LOGIC("We are in slow start and diff < m_gamma, so we "
                             "follow NewReno slow start");
                TcpNewReno::SlowStart(tcb, segmentsAcked);
            }
            else
            { // Linear increase/decrease mode

                if (diff > m_alpha && diff <= m_beta)
                {
                    if (m_currentThroughput > m_prevThroughput)
                    {
                        segCwnd++;
                        m_alpha++;
                        m_beta++;
                    }
                }
                else if (diff <= m_alpha)
                {
                    if (m_alpha > 1)
                    {
                        if (m_currentThroughput > m_prevThroughput)
                        {
                            segCwnd++;
                        }
                        else if (m_currentThroughput < m_prevThroughput)
                        {
                            segCwnd--;
                            m_alpha--;
                            m_beta--;
                        }
                    }
                    else
                    {
                        segCwnd++;
                    }
                }
                else if (diff > m_beta)
                {
                    if (m_alpha > 1)
                    {
                        m_alpha--;
                        m_beta--;
                    }
                    segCwnd--;
                    tcb->m_ssThresh = GetSsThresh(tcb, 0);
                }

            }

            // Implement the new conditions based on α, β, Th(t), and Th(t-rtt)

            // Update cwnd and save state
            tcb->m_cWnd = segCwnd * tcb->m_segmentSize;
            //m_prevThroughput = currThroughput;
            NS_LOG_DEBUG("Updated cwnd = " << tcb->m_cWnd << " ssthresh = " << tcb->m_ssThresh);

            tcb->m_ssThresh = std::max(tcb->m_ssThresh, 3 * tcb->m_cWnd / 4);
            NS_LOG_DEBUG("Updated ssThresh = " << tcb->m_ssThresh);
        }

        // Reset cntRtt & minRtt every RTT
        m_cntRtt = 0;
        m_minRtt = Time::Max();
        m_prevThroughput = m_currentThroughput;
    }
    else if (tcb->m_cWnd < tcb->m_ssThresh)
    {
        TcpNewReno::SlowStart(tcb, segmentsAcked);
    }
}

// void
// TcpVegasTweaked::IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
// {
//     NS_LOG_FUNCTION(this << tcb << segmentsAcked);

//     if (!m_doingVegasNow)
//     {
//         // If Vegas is not on, we follow NewReno algorithm
//         NS_LOG_LOGIC("Vegas is not turned on, we follow NewReno algorithm.");
//         TcpNewReno::IncreaseWindow(tcb, segmentsAcked);
//         return;
//     }

//     if (tcb->m_lastAckedSeq >= m_begSndNxt)
//     { // A Vegas cycle has finished, we do Vegas cwnd adjustment every RTT.

//         NS_LOG_LOGIC("A Vegas cycle has finished, we adjust cwnd once per RTT.");

//         // Save the current right edge for next Vegas cycle
//         m_begSndNxt = tcb->m_nextTxSequence;

//         /*
//          * We perform Vegas calculations only if we got enough RTT samples to
//          * insure that at least 1 of those samples wasn't from a delayed ACK.
//          */
//         if (m_cntRtt <= 2)
//         { // We do not have enough RTT samples, so we should behave like Reno
//             NS_LOG_LOGIC(
//                 "We do not have enough RTT samples to do Vegas, so we behave like NewReno.");
//             TcpNewReno::IncreaseWindow(tcb, segmentsAcked);
//         }
//         else
//         {
//             NS_LOG_LOGIC("We have enough RTT samples to perform Vegas calculations");
//             /*
//              * We have enough RTT samples to perform Vegas algorithm.
//              * Now we need to determine if cwnd should be increased or decreased
//              * based on the calculated difference between the expected rate and actual sending
//              * rate and the predefined thresholds (alpha, beta, and gamma).
//              */
//             uint32_t diff;
//             uint32_t targetCwnd;
//             uint32_t segCwnd = tcb->GetCwndInSegments();
//             m_currentThroughput = segCwnd / m_minRtt.GetSeconds();

//             /*
//              * Calculate the cwnd we should have. baseRtt is the minimum RTT
//              * per-connection, minRtt is the minimum RTT in this window
//              *
//              * little trick:
//              * desidered throughput is currentCwnd * baseRtt
//              * target cwnd is throughput / minRtt
//              */
//             double tmp = m_baseRtt.GetSeconds() / m_minRtt.GetSeconds();
//             targetCwnd = static_cast<uint32_t>(segCwnd * tmp);
//             NS_LOG_DEBUG("Calculated targetCwnd = " << targetCwnd);
//             NS_ASSERT(segCwnd >= targetCwnd); // implies baseRtt <= minRtt

//             /*
//              * Calculate the difference between the expected cWnd and
//              * the actual cWnd
//              */
//             diff = segCwnd - targetCwnd;
//             NS_LOG_DEBUG("Calculated diff = " << diff);

//             if (diff > m_gamma && (tcb->m_cWnd < tcb->m_ssThresh))
//             {
//                 /*
//                  * We are going too fast. We need to slow down and change from
//                  * slow-start to linear increase/decrease mode by setting cwnd
//                  * to target cwnd. We add 1 because of the integer truncation.
//                  */
//                 NS_LOG_LOGIC("We are going too fast. We need to slow down and "
//                              "change to linear increase/decrease mode.");
//                 segCwnd = std::min(segCwnd, targetCwnd + 1);
//                 tcb->m_cWnd = segCwnd * tcb->m_segmentSize;
//                 tcb->m_ssThresh = GetSsThresh(tcb, 0);
//                 NS_LOG_DEBUG("Updated cwnd = " << tcb->m_cWnd << " ssthresh=" <<
//                 tcb->m_ssThresh);
//             }
//             else if (tcb->m_cWnd < tcb->m_ssThresh)
//             { // Slow start mode
//                 NS_LOG_LOGIC("We are in slow start and diff < m_gamma, so we "
//                              "follow NewReno slow start");
//                 TcpNewReno::SlowStart(tcb, segmentsAcked);
//             }
//             else
//             { // Linear increase/decrease mode
//                 NS_LOG_LOGIC("We are in linear increase/decrease mode");

//                 // Calculate current throughput
//                // CalculateThroughput(tcb);

//                 if (diff > m_alpha && diff < m_beta)
//                 {
//                     if (m_currentThroughput > m_prevThroughput)
//                     {
//                         // Increase cwnd and thresholds
//                         segCwnd++;
//                         m_alpha++;
//                         m_beta++;
//                         //tcb->m_cWnd = segCwnd * tcb->m_segmentSize;
//                         NS_LOG_LOGIC("Increasing cwnd, alpha, and beta");
//                         std::cout<<"Increasing cwnd, alpha, and beta"<<std::endl;
//                     }
//                     // else: no updates needed
//                 }
//                 else if (diff < m_alpha)
//                 {
//                     if (m_alpha > 1)
//                     {
//                         if (m_currentThroughput > m_prevThroughput)
//                         {
//                             segCwnd++;
//                            // tcb->m_cWnd = segCwnd * tcb->m_segmentSize;
//                             NS_LOG_LOGIC("Alpha > 1, throughput increasing, incrementing cwnd");
//                             std::cout<<"Alpha > 1, throughput increasing, incrementing
//                             cwnd"<<std::endl;
//                         }
//                         else if (m_currentThroughput < m_prevThroughput)
//                         {
//                             segCwnd--;
//                             m_alpha--;
//                             m_beta--;
//                             //tcb->m_cWnd = segCwnd * tcb->m_segmentSize;
//                             NS_LOG_LOGIC(
//                                 "Alpha > 1, throughput decreasing, decrementing
//                                 cwnd/alpha/beta");
//                             std::cout<<"Alpha > 1, throughput decreasing, decrementing
//                             cwnd/alpha/beta"<<std::endl;
//                         }
//                     }
//                     else // alpha == 1
//                     {
//                         segCwnd++;
//                        // tcb->m_cWnd = segCwnd * tcb->m_segmentSize;
//                         NS_LOG_LOGIC("Alpha = 1, incrementing cwnd");
//                         std::cout<<"Alpha = 1, incrementing cwnd"<<std::endl;
//                     }
//                 }
//                 else if (diff > m_beta)
//                 {
//                     if (m_alpha > 1)
//                     {
//                         m_alpha--;
//                         m_beta--;
//                         NS_LOG_LOGIC("diff > beta, alpha > 1, adjusting parameters");
//                         std::cout<<"diff > beta, alpha > 1, adjusting parameters"<<std::endl;
//                     }
//                     segCwnd--;
//                   //  tcb->m_cWnd = segCwnd * tcb->m_segmentSize;
//                     tcb->m_ssThresh = GetSsThresh(tcb, 0);
//                     // else: no updates needed
//                     std::cout<<"no updates needed"<<std::endl;
//                 }
//             }
//             tcb->m_cWnd = segCwnd * tcb->m_segmentSize;
//             tcb->m_ssThresh = std::max(tcb->m_ssThresh, 3 * tcb->m_cWnd / 4);
//             NS_LOG_DEBUG("Updated ssThresh = " << tcb->m_ssThresh);
//         }

//         // Reset cntRtt & minRtt every RTT
//         m_cntRtt = 0;
//         m_minRtt = Time::Max();
//         m_prevThroughput = m_currentThroughput;
//         m_currentThroughput = 0;
//     }
//     else if (tcb->m_cWnd < tcb->m_ssThresh)
//     {
//         TcpNewReno::SlowStart(tcb, segmentsAcked);
//     }
// }

std::string
TcpVegasTweaked::GetName() const
{
    return "TcpVegas";
}

uint32_t
TcpVegasTweaked::GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight)
{
    NS_LOG_FUNCTION(this << tcb << bytesInFlight);
    return std::max(std::min(tcb->m_ssThresh.Get(), tcb->m_cWnd.Get() - tcb->m_segmentSize),
                    2 * tcb->m_segmentSize);
}

} // namespace ns3
