#!/usr/bin/python

"""Goal is to open web server on h1, then get from h2"""

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

   print("creating web server on h1...")
   # send all output to a file
   h1.cmd("python http/webserver.py > output_foo8.txt 2>&1 &")
   sleep(2)
   # Get the default index.html page for server h1.
   print("fetching index.html from h1...")
   h2.sendCmd("curl " + h1.IP())
   output = h2.waitOutput()
   sleep(1)

   print("checking for webserver process...")
   h1.sendCmd("pgrep -f webserver.py")
   output4 = h1.waitOutput()
   print("result is: " + output4)

   # kill the web server.
   print("cleaning up, killing web server...")
   h1.sendCmd("pgrep -f webserver.py | xargs kill -9")
   output2 = h1.waitOutput()
   print("result is: " + output2)
   print("done.")

   #print("got the following from web server: " + output)
   #h2.sendCmd("curl " + h1.IP())
   #print(output)
   #print("done.")

   #h1.sendCmd("pgrep -f webserver.py")
   #output2 = h1.waitOutput()
   #print("found PID: " + output2)
   #print("output2 is: " + output2)
   #print("output2 is type: " + str(type(output2)) )
   #killit = int(str(output2))
   #print("killit is type: " + str(type(killit)) )

   #killit = int(output2)
   #print("killing pid: " + str(killit))
   print("let's make sure we killed it...")
   h1.sendCmd("pgrep -f webserver.py")
   output3 = h1.waitOutput()
   print("result is: " + output3)

   net.stop()

if __name__ == "__main__":
   main()
