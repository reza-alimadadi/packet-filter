ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd -P)"
DPDK_DIR=$ROOT_DIR/dma_ip_drivers
DPDK_PATCH=$ROOT_DIR/patches/dpdk.patch

pushd $DPDK_DIR || exit 1
git apply $DPDK_PATCH
popd || exit 1
