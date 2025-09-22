
# This script will check if intel IOMMU is enabled on the system and check
# the hugepage settings.

# Check if intel_iommu is enabled
CMDLINE=$(cat /proc/cmdline)
if echo "$CMDLINE" | grep -q "intel_iommu=on"; then
    echo "intel_iommu is enabled."
else
    echo "intel_iommu is not enabled. Please enable it by adding 'intel_iommu=on' to the kernel command line."
    exit 1
fi

if echo "$CMDLINE" | grep -q "iommu=pt"; then
    echo "iommu=pt is enabled."
else
    echo "iommu=pt is not enabled. Please enable it by adding 'iommu=pt' to the kernel command line."
    exit 1
fi

# Check hugepage settings
HUGEPAGES=$(grep HugePages_Total /proc/meminfo | awk '{print $2}')
HUGEFREE=$(grep HugePages_Free /proc/meminfo | awk '{print $2}')
HUGESIZE=$(grep Hugepagesize /proc/meminfo | awk '{print $2}')
NPROC=$(nproc) # Number of CPU cores
if [ "$HUGEFREE" -lt "$NPROC" ]; then
    echo "Not enough free hugepages. Found $HUGEFREE free hugepages, but need at least $NPROC."
    exit 1
else
    echo "Hugepage settings are sufficient. Found $HUGEFREE free hugepages of size $HUGESIZE KB."
fi


