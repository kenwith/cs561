#!/usr/bin/python

"""a simple test script"""

from mininet.util import ensureRoot, dumpNodeConnections
from mininet.topo import MinimalTopo, Topo
from mininet.net import Mininet
from time import sleep

#from subprocess import Popen
import subprocess
from time import sleep


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

   h1 = net.get('h1')
   h2 = net.get('h2')

   print "Starting test..."
   h1.cmd('while true; do date; sleep 1; done > /tmp/date.out &')

   #pid = int( h1.cmd('echo $!') )
   pid = h1.cmd('echo $!')
   name_of_shell = h1.cmd('echo $0')
   print("pid is " + pid)
   print("name_of_shell is " + name_of_shell)

   sleep(20)

   print "Stopping test"
   h1.cmd('kill %while')
   print "Reading output"
   f = open('/tmp/date.out')
   lineno = 1
   for line in f.readlines():
      print "%d: %s" % ( lineno, line.strip() )
      lineno += 1
   f.close()

   net.stop()

if __name__ == "__main__":
   main()
