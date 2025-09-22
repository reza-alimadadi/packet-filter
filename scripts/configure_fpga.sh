
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd -P)"
PCIMEM_DIR=$SCRIPT_DIR/pcimem
LOG_DIR=$SCRIPT_DIR/../logs

mkdir -p $LOG_DIR

exec > $LOG_DIR/configure_fpga.log 2>&1

DEVICE_BDF0=`lspci | grep Xilinx | grep 903f | cut -d ' ' -f 1`
DEVICE_BDF1=`lspci | grep Xilinx | grep 913f | cut -d ' ' -f 1`
DEVICE_PATH=$(find /sys/devices/ -type d -name "*$DEVICE_BDF0*")

PCIMEM_CMD="$PCIMEM_DIR/pcimem $DEVICE_PATH/resource2"

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
FUNC_0_VALUE=$(( $FUNC_0_BASE << 16 | $FUNC_0_NUM ))
FUNC_1_BASE=$FUNC_0_NUM
FUNC_1_NUM=1
FUNC_1_VALUE=$(( $FUNC_1_BASE << 16 | $FUNC_1_NUM ))

# Write to QDMA
sudo $PCIMEM 0x1000 w $FUNC_0_VALUE 
sudo $PCIMEM 0x2000 w $FUNC_1_VALUE 

# Write to Rx and Tx for CMAC0
sudo $PCIMEM 0x8014 w 0x1
sudo $PCIMEM 0x800c w 0x1

# Write to enable CMAC1
sudo $PCIMEM 0xc014 w 0x1;
sudo $PCIMEM 0xc00c w 0x1;
