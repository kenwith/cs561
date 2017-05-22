#!/bin/bash
################################################################################
# CSEP561 - Network Systems
# Spring 2017
# Ken Withee
# Assignment 1
################################################################################


################################################################################
# Note: Mininet must be run as root so we need to check to make
# sure it is run as root.
################################################################################
if [ “$(id -u)” != “0” ]; then
   printf "\n\nExperiment must be run as root. Try the following:" 2>&1
   printf "\n\n/usr/bin/sudo ./run.sh\n\n\n" 2>&1
   exit 1
fi

# Clear the screen.
/usr/bin/clear

################################################################################
# bwnet: Bandwidth of bottleneck (network) link (Mb/s)
# bwhost: Bandwidth of host links (Mb/s)
# cong: Congestion control algorithm to use, default="reno"
# delay: Link propagation delay (ms)
# time: Duration (sec) to run the experiment
# maxq: Max buffer size of network interface in packets
# dir: Directory to store outputs
#
# Linux uses CUBIC-TCP by default that doesn't have the usual sawtooth
# behaviour.  For those who are curious, invoke this script with
# --cong cubic and see what happens...
################################################################################

# CONFIGURATION
bwnet="1.5"
bwhost="1000"
cong="cubic"
delay="10"
time="30"

iperf_port="5001"


# Run test for 20 and 100 size buffer. Save output to appropriate directories.
for qsize in 20 100; do
   dir=bb-q$qsize
   # Create the output directory if it doesn't already exist.
   /bin/mkdir -p $dir

   ./bufferbloat.py  -b $bwnet \
                  -B $bwhost \
                  --cong $cong \
                  --delay $delay \
                  -t $time \
                  --maxq $qsize \
                  -d $dir

   #############################################################################
   # Create graphs.
   #############################################################################
   python plot_tcpprobe.py -f $dir/cwnd.txt -o $dir/cwnd-iperf.png \
         -p $iperf_port > $dir/perf_test.log 2>&1
   python plot_queue.py -f $dir/q.txt -o $dir/q.png > \
         $dir/perf_test.log 2>&1
   python plot_ping.py -f $dir/ping.txt -o $dir/rtt.png > \
         $dir/perf_test.log 2>&1

done
