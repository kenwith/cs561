#!/usr/bin/python
"""a simple test script"""

import sys

from mininet.node import Host, Switch
from mininet.topo import Topo
from mininet.net import Mininet
from mininet.util import ensureRoot, waitListening, dumpNodeConnections
from mininet.log import setLogLevel, info, warn, output

# Ensure this script is being run as root.
ensureRoot()

topo = Topo()

s1 = Switch("s1")
h1 = Host("h1")
h2 = Host("h2")

topo.addSwitch(s1.name)
topo.addHost(h1.name)
topo.addHost(h2.name)

topo.addLink(h1.name, s1.name)
topo.addLink(h2.name, s1.name)

net = Mininet(topo)
net.start()

print("Dumping host connections")
dumpNodeConnections(net.hosts)
print("Testing network connectivity")
net.pingAll()

net.stop()
