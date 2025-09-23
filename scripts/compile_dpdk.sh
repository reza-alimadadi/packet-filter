ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd -P)"
SCRIPTS_DIR=$ROOT_DIR/scripts
QDMA_DPDK_DIR=$ROOT_DIR/dma_ip_drivers/QDMA/DPDK
BUILD_DIR=$ROOT_DIR/dpdk_build

mkdir -p $BUILD_DIR
pushd $BUILD_DIR

wget https://fast.dpdk.org/rel/dpdk-22.11.8.tar.xz
tar -xJf dpdk-22.11.8.tar.xz

# Apply QDMA DPDK patch
bash $SCRIPTS_DIR/patch-dpdk.sh

cd dpdk-22.11.8
cp -R $QDMA_DPDK_DIR/QDMA/DPDK/drivers/net/qdma ./drivers/net/
cp -R $QDMA_DPDK_DIR/QDMA/DPDK/examples/examples/qdma_testapp ./examples

# Add QDMA to meson build file on the drivers list. We add it between 'pfe' and 'qede' 
# to keep alphabetical order.
sed -i '/pfe/a\ \ \ \ \ \ \ \ "qdma",' ./drivers/net/meson.build

# Build DPDK with QDMA support
meson setup build
ninja -C build -j($nproc)
sudo ninja -C build install
sudo ldconfig

popd
