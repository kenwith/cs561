#!/usr/bin/python

"""a simple test script"""

from mininet.util import ensureRoot, dumpNodeConnections
from mininet.topo import MinimalTopo
from mininet.net import Mininet
from time import sleep

from subprocess import Popen

def main():
   # Ensure this script is being run as root.
   ensureRoot()

   topo = MinimalTopo()
   net = Mininet(topo)
   net.start()

   print "Starting test..."
   h1 = net.get('h1')
   h2 = net.get('h2')

   #sleep(10)
   #Popen(['ping', '-c 5 localhost'], shell=True)
   #Popen(["ifconfig"], shell=True).wait()
   #Popen(["ping", "-c", "5", "localhost"]).wait()
   #h1.popen(["ping", "-c", "5", h2.IP()]).wait()
   #h1.popen(["ifconfig", ">", "FUCKME"]).wait()
   #tmp = h1.popen(["ifconfig"]).wait()
   #print "Monitoring output for", seconds, "seconds"
   #endTime = time() + seconds

   #hosts = net.hosts

   net.stop()

if __name__ == "__main__":
   main()
