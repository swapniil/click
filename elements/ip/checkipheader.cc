/*
 * checkipheader.{cc,hh} -- element checks IP header for correctness
 * (checksums, lengths, source addresses)
 * Robert Morris
 *
 * Copyright (c) 1999 Massachusetts Institute of Technology.
 *
 * This software is being provided by the copyright holders under the GNU
 * General Public License, either version 2 or, at your discretion, any later
 * version. For more information, see the `COPYRIGHT' file in the source
 * distribution.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "checkipheader.hh"
#include "click_ip.h"
#include "glue.hh"
#include "confparse.hh"
#include "error.hh"
#include "bitvector.hh"

CheckIPHeader::CheckIPHeader()
  : _drops(0)
{
  add_input();
  add_output();
  _bad_src = 0;
  _n_bad_src = 0;
}

CheckIPHeader::~CheckIPHeader()
{
  delete[] _bad_src;
}

CheckIPHeader *
CheckIPHeader::clone() const
{
  return new CheckIPHeader();
}

void
CheckIPHeader::notify_noutputs(int n)
{
  set_noutputs(n < 2 ? 1 : 2);
}

void
CheckIPHeader::processing_vector(Vector<int> &in_v, int in_offset,
				   Vector<int> &out_v, int out_offset) const
{
  in_v[in_offset+0] = out_v[out_offset+0] = AGNOSTIC;
  if (noutputs() == 2)
    out_v[out_offset+1] = PUSH;
}

int
CheckIPHeader::configure(const String &conf, ErrorHandler *errh)
{
  Vector<String> args;
  cp_argvec(conf, args);
  if (args.size() > 1)
    return errh->error("too many arguments to `CheckIPHeader([ADDRS])'");
  
  Vector<u_int> ips;
  ips.push_back(0);
  ips.push_back(0xffffffff);

  if (args.size()) {
    String s = args[0];
    while (s) {
      u_int a;
      if (!cp_ip_address(s, (unsigned char *)&a, &s))
	return errh->error("expects IPADDRESS");
      cp_eat_space(s);
      for (int j = 0; j < ips.size(); j++)
	if (ips[j] == a)
	  goto repeat;
      ips.push_back(a);
     repeat: ;
    }
  }
  
  _n_bad_src = ips.size();
  _bad_src = new u_int [_n_bad_src];
  memcpy(_bad_src, &ips[0], sizeof(u_int) * ips.size());

  return 0;
}

inline Packet *
CheckIPHeader::smaction(Packet *p)
{
  struct ip *ip = (struct ip *) p->data();
  unsigned int src;
  unsigned hlen;
  
  if(p->length() < sizeof(struct ip))
    goto bad;
  
  if(ip->ip_v != 4)
    goto bad;
  
  hlen = ip->ip_hl << 2;
  if(hlen < sizeof(struct ip))
    goto bad;
  
  if(hlen > p->length())
    goto bad;
  
  if(in_cksum((unsigned char *)ip, hlen) != 0)
    goto bad;
  
  if(ntohs(ip->ip_len) < hlen)
    goto bad;

  /*
   * RFC1812 5.3.7 and 4.2.2.11: discard illegal source addresses.
   * Configuration string should have listed all subnet
   * broadcast addresses known to this router.
   */
  src = ip->ip_src.s_addr;
  for(int i = 0; i < _n_bad_src; i++)
    if(src == _bad_src[i])
      goto bad;

  /*
   * RFC1812 4.2.3.1: discard illegal destinations.
   * We now do this in the IP routing table.
   */
  
  return(p);
  
 bad:
  if (_drops == 0)
    click_chatter("IP checksum failed");
  _drops++;
  
  if (noutputs() == 2)
    output(1).push(p);
  else
    p->kill();
  
  return 0;
}

void
CheckIPHeader::push(int, Packet *p)
{
  if((p = smaction(p)) != 0)
    output(0).push(p);
}

Packet *
CheckIPHeader::pull(int)
{
  Packet *p = input(0).pull();
  if(p)
    p = smaction(p);
  return(p);
}

static String
CheckIPHeader_read_drops(Element *xf, void *)
{
  CheckIPHeader *f = (CheckIPHeader *)xf;
  return String(f->drops()) + "\n";
}

void
CheckIPHeader::add_handlers(HandlerRegistry *fcr)
{
  Element::add_handlers(fcr);
  fcr->add_read("drops", CheckIPHeader_read_drops, 0);
}

EXPORT_ELEMENT(CheckIPHeader)
