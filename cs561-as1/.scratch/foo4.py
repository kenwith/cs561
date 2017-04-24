#!/usr/bin/python

"""a simple test script"""

from mininet.util import ensureRoot, dumpNodeConnections
from mininet.topo import MinimalTopo, Topo
from mininet.net import Mininet
from time import sleep

#from subprocess import Popen
import subprocess


class SingleSwitchTopo(Topo):
    "Single switch connected to n hosts."
    def build(self, n=2):
        switch = self.addSwitch('s1')
        # Python's range(N) generates 0..N-1
        for h in range(n):
            host = self.addHost('h%s' % (h + 1))
            self.addLink(host, switch)

def main():
   # Ensure this script is being run as root.
   ensureRoot()

   topo = SingleSwitchTopo(n=2)
   net = Mininet(topo)
   net.start()

   print "Starting test..."
   h1 = net.get('h1')
   h2 = net.get('h2')

   print("trying with subprocess.call()")
   output = subprocess.call("exit 1", shell=True)
   print(output)

   print("trying with subprocess.check_output()")
   output = subprocess.check_output("ls blahblah; exit 0", 
   stderr=subprocess.STDOUT,
   shell=True)
   print(output)

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
