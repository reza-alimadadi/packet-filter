
SCRIPT_DIR="$(dirname $0)"
PCIMEM_DIR=$SCRIPT_DIR/pcimem

# LOG_DIR=$SCRIPT_DIR/../logs
# mkdir -p $LOG_DIR
# exec > $LOG_DIR/configure_fpga.log 2>&1

DEVICE_BDF0=`lspci | grep Xilinx | grep 903f | cut -d ' ' -f 1`
DEVICE_BDF1=`lspci | grep Xilinx | grep 913f | cut -d ' ' -f 1`
DEVICE_PATH=$(find /sys/devices/ -type d -name "*$DEVICE_BDF0*")

if [ "$DEVICE_BDF0" == "" ] || [ "$DEVICE_BDF1" == "" ]; then
    echo "PCIe device not found! Please check if FPGA is programmed correctly."
    exit 1
fi

# Check if pcimem is compiled
if [ ! -f "$PCIMEM_DIR/pcimem" ]; then
    echo "pcimem not found! Please compile it first."
    exit 1
fi

PCIMEM="$PCIMEM_DIR/pcimem $DEVICE_PATH/resource2"

# Unbind devices from vfio-pci driver if already bound since pcimem cannot access them
# while they are bound to vfio-pci. We will rebind them later after configuration.
sudo $SCRIPT_DIR/dpdk-devbind.py --unbind $DEVICE_BDF0 $DEVICE_BDF1

# By default, we configure only one queue
NUM_QUEUES=1
if [ "$1" != "" ]; then
    NUM_QUEUES=$1
fi

# Enable PCIe device for writing
sudo setpci -s $DEVICE_BDF0 COMMAND=0x02;
sudo setpci -s $DEVICE_BDF1 COMMAND=0x02;

FUNC_0_BASE=0
FUNC_0_NUM=$QUEUE_NUM
FUNC_0_VALUE=$(( FUNC_0_BASE << 16 | FUNC_0_NUM ))
FUNC_1_BASE=$FUNC_0_NUM
FUNC_1_NUM=1
FUNC_1_VALUE=$(( FUNC_1_BASE << 16 | FUNC_1_NUM ))

# Write to QDMA
sudo $PCIMEM 0x1000 w $FUNC_0_VALUE
sudo $PCIMEM 0x2000 w $FUNC_1_VALUE

# Write to Rx and Tx for CMAC0
sudo $PCIMEM 0x8014 w 0x1
sudo $PCIMEM 0x800c w 0x1

# Write to enable CMAC1
sudo $PCIMEM 0xc014 w 0x1;
sudo $PCIMEM 0xc00c w 0x1;

# Bind devices back to vfio-pci driver for DPDK usage
sudo $SCRIPT_DIR/dpdk-devbind.py -b vfio-pci $DEVICE_BDF0 $DEVICE_BDF1

