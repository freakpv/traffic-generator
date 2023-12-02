#! /bin/bash

#HUGEPGSZ=`cat /proc/meminfo | grep Hugepagesize | cut -d : -f 2 | tr -d ' '`
HUGEPGSZ=2048kB
PAGES=1000
PCI_PATHS=("0000:00:10.0")
DRV=igb

#
# Creates hugepage filesystem.
#
create_mnt_huge()
{
	echo "Creating /mnt/huge and mounting as hugetlbfs"
	sudo mkdir -p /mnt/huge

	grep -s '/mnt/huge' /proc/mounts > /dev/null
	if [ $? -ne 0 ] ; then
		sudo mount -t hugetlbfs nodev /mnt/huge
	fi
}

#
# Removes hugepage filesystem.
#
remove_mnt_huge()
{
	echo "Unmounting /mnt/huge and removing directory"
	grep -s '/mnt/huge' /proc/mounts > /dev/null
	if [ $? -eq 0 ] ; then
		sudo umount /mnt/huge
	fi

	if [ -d /mnt/huge ] ; then
		sudo rm -R /mnt/huge
	fi
}

#
# Removes all reserved hugepages.
#
clear_non_numa_huge_pages()
{
	echo > .echo_tmp
	echo "echo 0 > /sys/kernel/mm/hugepages/hugepages-${HUGEPGSZ}/nr_hugepages" > .echo_tmp

	echo "Removing currently reserved hugepages"
	sudo sh .echo_tmp
	rm -f .echo_tmp

	remove_mnt_huge
}

#
# Creates hugepages.
#
set_non_numa_huge_pages()
{
	clear_non_numa_huge_pages

	echo > .echo_tmp
	echo "echo $PAGES > /sys/kernel/mm/hugepages/hugepages-${HUGEPGSZ}/nr_hugepages" > .echo_tmp

	echo "Reserving hugepages"
	sudo sh .echo_tmp
	rm -f .echo_tmp

	create_mnt_huge
}

#
# Removes all reserved hugepages.
#
clear_numa_huge_pages()
{
	echo > .echo_tmp
	for d in /sys/devices/system/node/node? ; do
		echo "echo 0 > $d/hugepages/hugepages-${HUGEPGSZ}/nr_hugepages" >> .echo_tmp
	done
	echo "Removing currently reserved hugepages"
	sudo sh .echo_tmp
	rm -f .echo_tmp

	remove_mnt_huge
}

#
# Creates hugepages on specific NUMA nodes.
#
set_numa_huge_pages()
{
	clear_numa_huge_pages

	echo > .echo_tmp
	for d in /sys/devices/system/node/node? ; do
		node=$(basename $d)
		echo -n "Number of pages for $node: "
		echo "echo $PAGES > $d/hugepages/hugepages-${HUGEPGSZ}/nr_hugepages" >> .echo_tmp
	done
	echo "Reserving hugepages"
	sudo sh .echo_tmp
	rm -f .echo_tmp

	create_mnt_huge
}

#
# Print hugepage information.
#
grep_meminfo()
{
	grep -i huge /proc/meminfo
}

#
# Calls dpdk-devbind.py --status to show the devices and what they
# are all bound to, in terms of drivers.
#
show_devices()
{
	./dpdk-devbind.py --status
}

bind_devices_to_vfio_pci()
{
	# This setting/line taints the kernel and may not be needed 
	# when we run on physical machine and the IOMMU mode is enabled.
	echo "Y" > /sys/module/vfio/parameters/enable_unsafe_noiommu_mode
        for pci in "${PCI_PATHS[@]}";
            do sudo ./dpdk-devbind.py -b vfio-pci $pci && echo "Bound device $pci";
        done
	./dpdk-devbind.py --status
}

#
# Uses dpdk-devbind.py to move devices to work with kernel drivers again
#
unbind_devices()
{
    for pci in "${PCI_PATHS[@]}";
	    do sudo ./dpdk-devbind.py -b $DRV $pci && echo "Unbound device $pci"
    done
	./dpdk-devbind.py --status
}

################################################################################

if [ "$1" = "setup" ]; then
    set_non_numa_huge_pages
    bind_devices_to_vfio_pci
elif [ "$1" = "status" ]; then
    grep_meminfo
    show_devices
elif [ "$1" = "destroy" ]; then
    unbind_devices
    clear_non_numa_huge_pages
else
    echo "Unsupported command: $1"
fi
