/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 IITP RAS
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: António Fonseca <afonseca@tagus.inesc-id.pt>, written after OlsrHelper by Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "ngpsr-helper.h"
#include "ns3/ngpsr.h"
#include "ns3/node-list.h"
#include "ns3/names.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/node-container.h"
#include "ns3/callback.h"
#include "ns3/udp-l4-protocol.h"


namespace ns3 {

NGpsrHelper::NGpsrHelper ()
  : Ipv4RoutingHelper ()
{
  m_agentFactory.SetTypeId ("ns3::ngpsr::RoutingProtocol");
}

NGpsrHelper*
NGpsrHelper::Copy (void) const
{
  return new NGpsrHelper (*this);
}

Ptr<Ipv4RoutingProtocol>
NGpsrHelper::Create (Ptr<Node> node) const
{
  //Ptr<Ipv4L4Protocol> ipv4l4 = node->GetObject<Ipv4L4Protocol> ();
  Ptr<ngpsr::RoutingProtocol> ngpsr = m_agentFactory.Create<ngpsr::RoutingProtocol> ();
  //ngpsr->SetDownTarget (ipv4l4->GetDownTarget ());
  //ipv4l4->SetDownTarget (MakeCallback (&ngpsr::RoutingProtocol::AddHeaders, ngpsr));
  node->AggregateObject (ngpsr);
  return ngpsr;
}

void
NGpsrHelper::Set (std::string name, const AttributeValue &value)
{
  m_agentFactory.Set (name, value);
}


void
NGpsrHelper::Install (void) const
{
  NodeContainer c = NodeContainer::GetGlobal ();
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = (*i);
      Ptr<UdpL4Protocol> udp = node->GetObject<UdpL4Protocol> ();
      Ptr<ngpsr::RoutingProtocol> ngpsr = node->GetObject<ngpsr::RoutingProtocol> ();
      ngpsr->SetDownTarget (udp->GetDownTarget ());
      udp->SetDownTarget (MakeCallback(&ngpsr::RoutingProtocol::AddHeaders, ngpsr));
    }


}


}
