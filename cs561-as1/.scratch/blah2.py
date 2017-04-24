#!/usr/bin/python

"""a simple test script"""

from mininet.util import ensureRoot, dumpNodeConnections
from mininet.topo import MinimalTopo
from mininet.net import Mininet

def main():
   # Ensure this script is being run as root.
   ensureRoot()

   topo = MinimalTopo()
   net = Mininet(topo)

   print net.hosts[0].IP()
   print("Host", net.hosts[0].name, "has IP address", net.hosts[0].IP(), "and MAC address", net.hosts[0].MAC())

   net.start()

   print("Dumping host connections")
   dumpNodeConnections(net.hosts)
   print("Testing network connectivity")
   net.pingAll()

   net.stop()

if __name__ == "__main__":
   main()
