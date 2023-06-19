#!/usr/bin/python 

import socket, binascii , struct, string

def SMAC(packet):
   return binascii.hexlify(packet[6:12]).decode()

def DMAC(packet):
   return binascii.hexlify(packet[0:6]).decode()

protocols={socket.IPPROTO_TCP:'tcp',
           socket.IPPROTO_UDP:'udp',
           socket.IPPROTO_ICMP:'icmp'}



def decode_ip_packet(s):
    d={}
    d['version']=(ord(s[0]) & 0xf0) >> 4
    d['header_len']=ord(s[0]) & 0x0f
    d['tos']=ord(s[1])
    d['total_len']=socket.ntohs(struct.unpack('H',s[2:4])[0])
    d['id']=socket.ntohs(struct.unpack('H',s[4:6])[0])
    d['flags']=(ord(s[6]) & 0xe0) >> 5
    d['fragment_offset']=socket.ntohs(struct.unpack('H',s[6:8])[0] & 0x1f)
    d['ttl']=ord(s[8])
    d['protocol']=ord(s[9])
    d['checksum']=socket.ntohs(struct.unpack('H',s[10:12])[0])
    d['source_address']=socket.inet_ntoa(s[12:16])
    d['destination_address']=socket.inet_ntoa(s[16:20])
    if d['header_len']>5:
      d['options']=s[20:4*(d['header_len']-5)]
    else:
      d['options']=None
    d['data']=s[4*d['header_len']:]
    return d
def dumphex(s):
    bytes = map(lambda x: '%.2x' % x, map(ord, s))
    i = 0
    for i in xrange(0,len(bytes)/16):
      print '    %s' % string.join(bytes[i*16:(i+1)*16],' ')
    print '    %s' % string.join(bytes[(i+1)*16:],' ')
      
def print_packet(pktlen, data):
    if not data:
      return
    if data[12:14]=='\x08\x00':
      decoded=decode_ip_packet(data[14:])
      print '\n %s > %s' % (decoded['source_address'], decoded['destination_address'])
      for key in ['version', 'header_len', 'tos', 'total_len', 'id',
                  'flags', 'fragment_offset', 'ttl', 'protocol']:
        print '  %s: %d' % (key, decoded[key])
#      print '  protocol: %s' % protocols[decoded['protocol']]
      print '  header checksum: %d' % decoded['checksum']
      print '  data:'
      dumphex(decoded['data'])

ETH_P_ALL = 3		# To receive all Ethernet protocols
ETH_P_IP = 0x0800	# To receive all Ethernet protocols

HOST = socket.gethostbyname(socket.gethostname())
print(HOST)
s = socket.socket(socket.PF_PACKET, socket.SOCK_RAW, socket.htons(ETH_P_IP))
#s.bind(("eth0", 0))

while True:
   pkt = s.recvfrom(2000)
   
   print( '\nMAC Src addr = ' + SMAC(pkt[0]))
   print( 'MAC Dst addr = ' + DMAC(pkt[0]))

   print_packet( pkt[1][2],pkt[0])
