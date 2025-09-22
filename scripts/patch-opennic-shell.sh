
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd -P)"
OPENNIC_SHELL_DIR=$ROOT_DIR/open-nic-shell
OPENNIC_SHELL_PATCH=$ROOT_DIR/patches/opennic_shell.patch

pushd $OPENNIC_SHELL_DIR || exit 1
git apply $OPENNIC_SHELL_PATCH
popd || exit 1
