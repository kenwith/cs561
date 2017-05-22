#!/usr/bin/python
"""a simple test script"""

import sys

from mininet.node import Host, Switch, Controller
from mininet.topo import Topo
from mininet.net import Mininet
from mininet.util import ensureRoot, waitListening, dumpNodeConnections
from mininet.log import setLogLevel, info, warn, output

def main():
   # Ensure this script is being run as root.
   ensureRoot()

   topo = Topo()
   mySwitch = Switch("s1")
   myHost1 = Host("h1")

   topo.addSwitch(mySwitch.name)
   topo.addHost(myHost1.name)
   topo.addLink(myHost1.name, mySwitch.name)

   net = Mininet(topo, controller=Controller)
   net.start()

   print("Dumping host connections")
   dumpNodeConnections(net.hosts)
   print("Testing network connectivity")
   net.pingAll()

   net.stop()

if __name__ == "__main__":
    main()
