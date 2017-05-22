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

s1 = topo.addSwitch("s1")
h1 = topo.addHost("h1")
h2 = topo.addHost("h2")

topo.addLink(h1, s1)
topo.addLink(h2, s1)

net = Mininet(topo)
net.start()

print("Dumping host connections")
dumpNodeConnections(net.hosts)
print("Testing network connectivity")
net.pingAll()

net.stop()
