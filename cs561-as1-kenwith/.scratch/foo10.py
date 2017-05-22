#!/usr/bin/python

"""Goal is to open web server on h1, then get from h2"""

from mininet.util import ensureRoot, dumpNodeConnections
from mininet.topo import MinimalTopo, Topo
from mininet.net import Mininet
from mininet.link import TCLink
from mininet.node import CPULimitedHost

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
            self.addLink(host, switch, bw=10, delay='5ms', loss=2,
                        max_queue_size=1000, use_htb=True)

def main():
   # Ensure this script is being run as root.
   ensureRoot()

   topo = SingleSwitchTopo(n=2)
   net = Mininet(topo, host=CPULimitedHost, link=TCLink)
   net.start()

   h1 = net.get('h1')
   h2 = net.get('h2')

   print("Starting test...")

   print("performing iperf test...")

   net.iperf( (h1, h2) )

   print("done.")

   #print("creating web server on h1...")
   # send all output to a file
   #h1.cmd("python http/webserver.py > output_foo9.txt 2>&1 &")

   # wait a second for web server to start
   #sleep(1)

   # Get the default index.html page for server h1.
   # TODO now start timing this.
   #h2.sendCmd("curl -o /dev/null -s " + h1.IP() + "/http/index.html")
   #html_output = h2.waitOutput()

   # kill the web server.
   #print("cleaning up, killing web server...")
   #h1.cmd("pgrep -f webserver.py | xargs kill -9")
   #print("done.")

   print("...end test.")

   net.stop()

if __name__ == "__main__":
   main()
