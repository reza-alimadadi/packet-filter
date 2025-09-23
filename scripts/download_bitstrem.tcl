
set device "xcu280_u55c_0"
set bitstream_file [lindex $argv 1]

open_hw_manager
connect_hw_server -url [lindex $argv 0]:3121
current_hw_target [get_hw_targets */xilinx_tcf/Xilinx/*]
open_hw_target
current_hw_device [lindex [get_hw_devices $device] 0]
refresh_hw_device -update_hw_probes false [lindex [get_hw_devices ${device}] 0]

set_property PROBES.FILE {} [get_hw_devices ${device}]
set_property FULL_PROBES.FILE {} [get_hw_devices ${device}]
set_property PROGRAM.FILE ${bitstream} [get_hw_devices ${device}]
program_hw_devices [get_hw_devices ${device}]
refresh_hw_device [lindex [get_hw_devices] 0]
close_hw_manager
quit
