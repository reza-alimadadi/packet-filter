#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd -P)"

if [ "$1" == "" ]; then
    echo "Usage: $0 <bitstream_file>"
    exit 1
fi

HOSTNAME=$(hostname)
BITSTREAM_FILE=$1
if [ ! -f "$BITSTREAM_FILE" ]; then
    echo "Bitstream file $BITSTREAM_FILE not found!"
    exit 1
fi

check_vivado=$(which vivado)
if [ "$check_vivado" == "" ]; then
    echo "Vivado not found in PATH. Please source Vivado settings first."
    exit 1
fi

# Check if onic module is loaded
if lsmod | grep -q onic; then
    echo "onic module is loaded. Will remove it first."
    sudo rmmod onic
fi

DEVICE_BDF0=`lspci | grep Xilinx | awk 'NR==1{print $1}'`
DEVICE_BDF1=`lspci | grep Xilinx | awk 'NR==2{print $1}'`
device_bdf0="0000:$DEVICE_BDF0"
device_bdf1="0000:$DEVICE_BDF1"


device0_path=$(find /sys/bus/pci/devices/ -type l -name "*$DEVICE_BDF0*")
device1_path=$(find /sys/bus/pci/devices/ -type l -name "*$DEVICE_BDF1*")

# Unbind devices from vfio-pci driver to allow FPGA programming
if [[ $DEVICE_BDF0 != "" && $DEVICE_BDF1 != "" ]]; then
    echo "Unbinding FPGA devices from vfio-pci driver..."
    sudo $SCRIPT_DIR/dpdk-devbind.py --unbind $DEVICE_BDF0 $DEVICE_BDF1
fi

# Disabling error report to PCIe controller.
if [ -z "$device0_path" ]; then
    bridge_bdf=$(basename $(dirname $(readlink "$device0_path")))

    # COMMAND register: clear SERR# enable
    sudo setpci -s $bridge_bdf COMMAND=0000:0100

    # DevCtl register of CAP_EXP: clear ERR_FATAL (Fatal Error Reporting Enable)
    sudo setpci -s $bridge_bdf CAP_EXP+8.w=0000:0004
fi

echo "Programming FPGA with bitstream..."
vivado -mode batch -source $SCRIPT_DIR/download_bitstream.tcl -tclargs $HOSTNAME $BITSTREAM_FILE

echo "PCIe link rescan..."
if [ -e "$device0_path" ]; then
    echo 1 | sudo tee "/sys/bus/pci/devices/${bridge_bdf}/${device_bdf0}/remove" >/dev/null
    if [ -e "$device1_path" ]; then
        echo 1 | sudo tee "/sys/bus/pci/devices/${bridge_bdf}/${device_bdf1}/remove" >/dev/null
    fi
    echo 1 | sudo tee "/sys/bus/pci/devices/${bridge_bdf}/rescan" > /dev/null
else
    echo 1 | sudo tee "/sys/bus/pci/rescan" > /dev/null
fi

sleep 1

DEVICE_BDF0=`lspci | grep Xilinx | grep 903f | cut -d ' ' -f 1`
DEVICE_BDF1=`lspci | grep Xilinx | grep 913f | cut -d ' ' -f 1`
if [[ $DEVICE_BDF0 == "" || $DEVICE_BDF1 == "" ]]; then
    echo "You need to reboot the system for PCIe link to come up."
    exit 1
fi
