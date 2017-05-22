#!/usr/bin/python
"""a simple test script"""

import sys

from mininet.node import Host, Switch, Controller, Node
from mininet.topo import Topo
from mininet.net import Mininet
from mininet.util import ensureRoot, waitListening, dumpNodeConnections
from mininet.log import setLogLevel, info, warn, output

# Ensure this script is being run as root.
ensureRoot()

#net = Mininet(controller=Controller)

#net.addController('c0')

#host1 = net.addHost('h1', ip='172.16.0.1')
myHost = Host('myHostName', cls=Host, ip='10.0.0.200')
myHost.addIntf(

print "ip is", myHost.IP()

#print(type(myHost))
#print "Host", myHost.name, "and IP", myHost.IP()
#, "has IP address", host1.IP(), "done"

#h1 = net.addHost('h1', cls=Host, ip='10.0.0.1', defaultRoute=None)
#myHost = Host("h1", cls=Host, ip="172.16.0.1", defaultRoute=None)
#myNode = Node("h1")
#myNode.setIP("172.16.0.1")

#print "Host", myHost.name, "has IP address", myHost.IP()

#, "and MAC address", myHost.MAC()
