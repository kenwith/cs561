#!/usr/bin/python
##########################################################################
#Ken Withee - CSEP561 - Assignment 1 - Bufferbloat
##########################################################################

##########################################################################
# Imports
##########################################################################
from mininet.topo import Topo, MinimalTopo
from mininet.node import CPULimitedHost
from mininet.link import TCLink
from mininet.net import Mininet
from mininet.log import lg, info
from mininet.util import dumpNodeConnections
from mininet.cli import CLI

from subprocess import Popen
from time import sleep, time
from multiprocessing import Process
from argparse import ArgumentParser

from monitor import monitor_qlen
import termcolor as T

import sys
import os
import numpy


##########################################################################
# Arguments
##########################################################################
parser = ArgumentParser(description="Bufferbloat tests")
parser.add_argument('--bw-host', '-B',
                    type=float,
                    help="Bandwidth of host links (Mb/s)",
                    default=1000)

parser.add_argument('--bw-net', '-b',
                    type=float,
                    help="Bandwidth of bottleneck (network) link (Mb/s)",
                    required=True)

parser.add_argument('--delay',
                    type=float,
                    help="Link propagation delay (ms)",
                    required=True)

parser.add_argument('--dir', '-d',
                    help="Directory to store outputs",
                    required=True)

parser.add_argument('--time', '-t',
                    help="Duration (sec) to run the experiment",
                    type=int,
                    default=10)

parser.add_argument('--maxq',
                    type=int,
                    help="Max buffer size of network interface in packets",
                    default=100)

parser.add_argument('--cong',
                    help="Congestion control algorithm to use",
                    default="reno")

# Parse the parameters and store as attributes of namespace.
args = parser.parse_args()


##########################################################################
# Host to home router at 1 Gb/s.
##########################################################################
h1_linkopts = dict(
   bw = float(args.bw_host),
   delay = float(args.delay),
   loss= 0,
   max_queue_size = int(args.maxq),
   use_htb = True
)
##########################################################################
# Home router to Internet at 1.5 Mb/s.
##########################################################################
h2_linkopts = dict(
   bw = float(args.bw_net),
   delay = float(args.delay),
   loss= 0,
   max_queue_size = int(args.maxq),
   use_htb = True
)


##########################################################################
# Extended Topo class for buffer bloat project.
##########################################################################
class BBTopo(Topo):

   # Default instantiator.
   def build(self):
      # Add switch that will also stand in as router.
      switch = self.addSwitch("s0")

      # Link from h1 to router. 
      h1 = self.addHost('h1')
      self.addLink(h1, switch, **h1_linkopts)

      # Link from h2 to router. 
      h2 = self.addHost('h2')
      self.addLink(h2, switch, **h2_linkopts)

      return


##########################################################################
# This sends a system command using standard C system(0 function. It 
# removes the module tcp_probe and then adds the module tcp_probe with the 
# full=1. TCP probe is a module that records the state of a TCP connection 
# in response to incoming packets. It works by inserting a hook into the 
# tcp_recv processing path using kprobe so that the congestion window and 
# sequence number can be captured.If the module is not already loaded then
# an ERROR might be shown however this is OK because the module will be 
# loaded, see https://wiki.linuxfoundation.org/networking/tcpprobe.
##########################################################################
def start_tcpprobe(outfile="cwnd.txt"):
   sys.stdout.write("START TCP PROBE \t\t\t[.")
   sys.stdout.flush()
   Popen("rmmod tcp_probe; \
         modprobe tcp_probe full=1; \
         cat /proc/net/tcpprobe > %s/%s 2>&1" \
         % (args.dir, outfile), shell=True)

   # Wait a bit for the file to get created and the cat to start.
   for num in xrange(20):
      sys.stdout.write('.')
      sys.stdout.flush()
      sleep(.1)
   sys.stdout.write(".]\n\n")
   sys.stdout.flush()

   return


##########################################################################
# Set congestion control algorithm.
##########################################################################
def set_cong():
   sys.stdout.write("START CONGESTION CONTROL ALGORITHM \t[.")
   sys.stdout.flush()
   Popen("sysctl -w net.ipv4.tcp_congestion_control=%s > \
              %s/perf_test.log 2>&1" % (args.cong, args.dir), shell=True).wait()

   # Wait a bit for the OS to set the algo.
   for num in xrange(20):
      sys.stdout.write('.')
      sys.stdout.flush()
      sleep(.1)
   sys.stdout.write(".]\n\n")
   sys.stdout.flush()

   return


##########################################################################
# Kills all the cat processes. Waits until they are dead.
##########################################################################
def stop_tcpprobe():
   Popen("killall -q -9 cat >> %s/perf_test.log 2>&1" % args.dir, \
         shell=True).wait()


##########################################################################
# Creates a Process object with name monitor. The Process
# object calls the monitor_qlen method from monitor.py which
# is part of the mininet-util github.
##########################################################################
def start_qmon(iface, interval_sec=0.1, outfile="q.txt"):
    monitor = Process(target=monitor_qlen,
                      args=(iface, interval_sec, outfile))
    # Start the process.
    monitor.start()
    return monitor

##########################################################################
# Start iperf.
##########################################################################
def start_iperf(net):
   # Get references for hosts.
   h1 = net.get('h1')
   h2 = net.get('h2')

   # The -w 16m parameter ensures that the TCP flow is not receiver 
   # window limited.  If it is, there is a chance that the router 
   # buffer may not get filled up.
   sys.stdout.write("START IPERF \t\t\t\t[.")
   sys.stdout.flush()
   server = h2.popen("iperf -s -w 16m")
   h1.popen("iperf -c %s -t %d > %s/%s" % (h2.IP(), args.time + 15, args.dir, 
      "iperf_results"), shell=True)

   # Wait a bit for iperft to get rolling.
   for num in xrange(20):
      sys.stdout.write('.')
      sys.stdout.flush()
      sleep(.1)
   sys.stdout.write(".]\n\n")
   sys.stdout.flush()

   return


##########################################################################
# Start web server.
##########################################################################
def start_webserver(net):
   # Get reference for host.
   h1 = net.get('h1')

   # start web server on host h1
   sys.stdout.write("START WEB SERVER \t\t\t[.")
   sys.stdout.flush()
   proc = h1.popen("python http/webserver.py", shell=True)

   # Wait a bit for webserver to start completely.
   for num in xrange(35):
      sys.stdout.write('.')
      sys.stdout.flush()
      sleep(.1)
   sys.stdout.write(".]\n\n")
   sys.stdout.flush()

   return


##########################################################################
# Print basic net info and ping tests.
##########################################################################
def print_net_info(net):
   print("NODE CONNECTIONS")
   # This dumps the topology and how nodes are interconnected through links.
   dumpNodeConnections(net.hosts)

   print("\nPING TESTS")
   # This performs a basic all pairs ping test.
   net.pingAll()
   print("\n")

   return


##########################################################################
# fetch web page
##########################################################################
def fetch_webpage(net):
   h1 = net.get('h1')
   h2 = net.get('h2')

   sys.stdout.write("Fetching HTML...")
   sys.stdout.flush()
   tmp_proc = h2.popen("curl -o /dev/null -s -w \
         %%{time_total} %s/http/index.html" % h1.IP())
   tmp_proc.wait()
   fetch_time = float(tmp_proc.communicate()[0])
   sys.stdout.write("done. Elapsed Time: %.1fs" % fetch_time)
   sys.stdout.flush()
   #print("time taken: %s" % fetch_time)

   return fetch_time


##########################################################################
# start ping
##########################################################################
def start_ping(net):
    # TODO: Start a ping train from h1 to h2 (or h2 to h1, does it
    # matter?)  Measure RTTs every 0.1 second.  Read the ping man page

   h1 = net.get('h1')
   h2 = net.get('h2')

   # start ping train
   sys.stdout.write("START PING TRAIN \t\t\t[.")
   sys.stdout.flush()
   proc = h1.popen("ping -i .1 %s > %s/%s" % (h2.IP(), args.dir, 
         "ping.txt"), shell=True)

   # Wait a bit for webserver to start completely.
   for num in xrange(20):
      sys.stdout.write('.')
      sys.stdout.flush()
      sleep(.1)
   sys.stdout.write(".]\n\n")
   sys.stdout.flush()

   return


##########################################################################
# main bufferbloat method which does all the work
##########################################################################
def bufferbloat():
   print("\n\n--- STARTING PERFORMANCE TEST FOR QUEUE %s ---\n\n" % args.maxq)
   # Take a measurement of the start time and add 8 for servers starts.
   start_time = time() + 8

   # Make sure the directory exists to store output.
   if not os.path.exists(args.dir):
      os.makedirs(args.dir)

   # We can just use SingleSwitchTopo which is already setup how we want.
   topo = BBTopo()
   net = Mininet(topo=topo, host=CPULimitedHost, link=TCLink)
   net.start()

   # Print basic net info and ping tests.
   print_net_info(net)

   # Set congestion control algorithm.
   set_cong()

   # Start all the monitoring processes. this is the tcp_probe
   # which outputs to the file cwnd.txt.
   start_tcpprobe("cwnd.txt")

   # s0-eth1 is interface of local host to the router, s0-eth2 is the
   # interface of the router to the Internet which is also the bottleneck.
   qmon = start_qmon(iface='s0-eth2', outfile='%s/q.txt' % (args.dir))

   # Start iperf, ping train, and web server.
   start_iperf(net)
   start_ping(net)
   start_webserver(net)

   # list to hold webpage fetc times
   fetch_times = []

   # Fetch web page while we still have time.
   while True:
      # fetch web page and add time to list
      fetch_times.append(fetch_webpage(net))

      # get current time, calculate delta since start time
      now = time()
      delta = now - start_time

      # if delta is greater than time input as argument, then stop
      if delta > args.time:
         break

      # print the time left in the test
      print ", Time Remaining in Test:  %.1fs" % (args.time - delta)

   # Print average and standard deviation.
   print("\n\nFetch Times Average : \t\t\t%.1fs" % numpy.mean(fetch_times))
   print("Fetch Times Standard Deviation: \t%.1fs\n" % numpy.std(fetch_times))
   print("Buffer, RTT, and CWND graphs in %s directory. \n\n" % args.dir)

   # Uncomment to debug with command line.
   # CLI(net)

   # Clean up, stop probe, qmon, and net.
   # Ensure that all processes you create within Mininet are killed.
   # Sometimes they require manual killing.
   stop_tcpprobe()
   qmon.terminate()
   net.stop()
   Popen("pgrep -f webserver.py | xargs kill -9 > %s/perf_test.log 2>&1" % \
            args.dir, shell=True).wait()

   print("--- END PERFORMANCE TEST FOR QUEUE %s ---\n\n" % args.maxq)

##########################################################################
# main method
##########################################################################
if __name__ == "__main__":
    bufferbloat()
