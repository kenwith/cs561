#!/usr/bin/python

"""a simple test script"""

from mininet.net import Mininet
from mininet.node import Controller
from mininet.cli import CLI
from mininet.log import setLogLevel, info
from mininet.util import ensureRoot

def main(n=2):
   # Ensure this script is being run as root.
   ensureRoot()

   for a in range(n):
      print("a=" + str(a))

if __name__ == "__main__":
   main()
