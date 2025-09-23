# Packet Filter on OpenNIC
This is a packet filter implementation where the user can specify desired filters, which are then applied to the hardware.
On the hardware side, I implemented the packet filter as a part of the user plugin in UserBox 250MHz and modified the box to connect to the IP through AXI-Stream as well as AXI-Lite for its configuration.
The packet filter is implemented using Vitis HLS, which generates the HDL code used in the project.
The packet filter utilizes Toeplitz hashing to map the desired addresses to a hash table on on-chip memory with one port, instead of using an expensive CAM (Content Addressable Memory).
This approach allows the packet filter to include many filters based on the size of the hash table. By default, it uses 32 entries, but it can be easily configured to other sizes.

The software is implemented over DPDK utilizing the AMD DMA driver for QDMA, which allows configuration of the packet filter IP through MMIO and provides a high-performance receive/send interface.

## Repo Structure

The repository is organized as follows.

    |-- packet-filter --
        |-- dma_ip_drivers --
        |-- hardware --
            |-- Makefile
            |-- src --
                |-- hdl --
                |-- hls --
        |-- open-nic-shell --
        |-- patches --
            |-- dpdk.patch
            |-- opennic_shell.patch
        |-- script --
            |-- configure_fpga.sh
            |-- program_fpga.sh
            |-- ...
        |-- software --
            |-- src --
            |-- CMakeLists.txt
            |-- Makefile
        |-- README.md

## 1. Cloning and Install dependancies
To clone the repository along with all of its submodules, run the following:
```bash
git clone git@github.com:reza-alimadadi/packet-filter.git --recurse-submodules
```

Our software depends on the following packages:
```bash
sudo apt install -y gcc g++ build-essential cmake pkg-config ninja-build
```

## 2. Applying DPDK and OpenNIC Patches
Next, apply the DPDK and OpenNIC shell patches:
```bash
bash scripts/patch-dpdk.sh
bash scripts/patch-opennic-shell.sh
```

## 3. Compile DPDK
Currently, we are using DPDK version 22.11.
```bash
wget https://fast.dpdk.org/rel/dpdk-22.11.8.tar.xz
tar -xJf dpdk-22.11.8.tar.xz
```

Before compiling, we need to add QDMA to DPDK:
```bash
cd dpdk-22.11.8
cp -R ../dma_ip_drivers/QDMA/DPDK/drivers/net/qdma ./drivers/net/
cp -R ../dma_ip_drivers/QDMA/DPDK/examples/examples/qdma_testapp ./examples
```

Add QDMA to the meson build file for the driver list located at `drivers/net/meson.build` by adding `qdma` somewhere after the `pfe`.

Now we can compile DPDK:
```bash
meson setup build
ninja -C build -j($nproc)
sudo ninja -C build install
sudo ldconfig
```

## 4. Build FPGA Design
OpenNIC provides a script to build, synthesize, place and route, and generate a bitstream. We use the same script:
```bash
cd open-nic-shell/scripts
vivado -mode tcl -source build.tcl -tclargs -board au250 -num_cmac_port 2 -num_phys_func 2 -tag packet-filter -impl 1
```

This command generates the bitstream at the following path, which is used later to program the FPGA:
```
open-nic-shell/build/au280_packet-filter/open_nic_shell/open_nic_shell.runs/impl_1/open_nic_shell.bit
```

## 5. Downloading Bitstream
After the bitstream is generated, use the provided scripts to program the FPGA.
First, run hw_server on the FPGA machine, located at `<path/to/xilinx>/Vivado/<version>/bin/hw_server`.
Next, use our script to download the bitstream to the FPGA:
```bash
bash scripts/program_fpga.sh <path/to/bitsream> 
```

One benefit of using this script is that it often avoids the need to reboot the machine. 
Instead, it instructs the root complex to scan PCIe devices, and if it cannot find the FPGA with the specific design, it informs the user that rebooting is the last resort.

## 6. Configure IOMMU and HugePages
Before running the software on the FPGA machine, perform a system check to verify that IOMMU is enabled and HugePages are allocated:
```bash
bash scripts/check_system.sh
```
If these are not configured, edit `/etc/default/grub` to include:
```bash
GRUB_CMDLINE_LINUX=" default_hugepagesz=1G hugepagesz=1G hugepages=4 intel_iommu=on iommu=pt"
```
Then update grub and reboot the machine for the changes to take effect:
```bash
sudo update-grub
```

## 7. Running the Server
First, compile the code:
```bash
cd software/
make build
```

Then configure the FPGA. This step enables the CMAC, configures OpenNIC queues, and loads the VFIO driver for the PCIe device:
```bash
bash scripts/configure_fpga.sh
```

After configuration, run the server code. The server requires the following arguments:
* DPDK configuration (-c): Required by DPDK EAL to configure the library. Includes the PCIe BDF for the FPGA device with device-specific configuration.
* Address Filter (-f): Our design accepts up to 32 filters by default. Each address is in <ip>:<port> format and separated by commas.
* Duration (-d): How long the server runs.

Below is an example of how to run the server:
```bash
sudo ./build/bin/main -c "./main -a 17:00.0,desc_prefetch=1" -f "192.168.2.1:8500,192.168.2.95:8501" -d 30
```
