#!/usr/bin/python

"""a simple test script"""

from mininet.util import ensureRoot, dumpNodeConnections
from mininet.topo import MinimalTopo
from mininet.net import Mininet
from time import sleep

def main():
   # Ensure this script is being run as root.
   ensureRoot()

   topo = MinimalTopo()
   net = Mininet(topo)

   print net.hosts[0].IP()
   print("Host", net.hosts[0].name, "has IP address", \
   net.hosts[0].IP(), "and MAC address", net.hosts[0].MAC())

   net.start()

   print("Dumping host connections")
   dumpNodeConnections(net.hosts)
   print("Testing network connectivity")
   net.pingAll()

   #start a web server
   #print("starting web server...")
   #host1 = net.get('h1')
   #host1.popen("ping -c 5 localhost > myfuckingfile", shell=True)
   #proc = host1.popen("python -m SimpleHTTPServer")
   #print("done. waiting 20 seconds")

   #host2 = net.get('h2')
   #proc2 = host2.popen("curl -o ./fuck.txt -s -w %{time_total} 10.0.0.1")

   net.stop()

if __name__ == "__main__":
   main()
