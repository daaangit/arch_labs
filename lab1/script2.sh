#!/bin/bash

SCR_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WPATH="$SCR_DIR/log"
BACKUP="$SCR_DIR/backup"
DISK_IMAGE="$SCR_DIR/test_disk.img"
MAIN_SCRIPT="$SCR_DIR/script1.sh"
mkdir -p "$BACKUP"

unmount_func() 
{
    if mountpoint -q "$WPATH" 2>/dev/null; then
        sudo umount "$WPATH" 2>/dev/null || true
    fi
    rm -f "$DISK_IMAGE"
    rm -rf "$WPATH"
    rm -f "$BACKUP"/*.tar.*
}

create_test_disk() 
{
    echo "Creating test disk (1GB)..."
    dd if=/dev/zero of="$DISK_IMAGE" bs=1M count=1000 2>/dev/null
    mkfs.ext4 -F "$DISK_IMAGE" >/dev/null 2>&1
    mkdir -p "$WPATH"
    sudo mount "$DISK_IMAGE" "$WPATH"
    sudo chown -R $USER:$USER "$WPATH"
}

create_test_files() 
{
    local count=$1
    local size=$2
    
    echo "Creating ${count} files, ${size}MB each..."
    for i in $(seq 1 $count); do
        dd if=/dev/zero of="$WPATH/file_$i.dat" bs=1M count=$size 2>/dev/null
        touch -d "-$i days" "$WPATH/file_$i.dat"
    done
    sync
}

run_test() 
{
    local name="$1"
    local count=$2
    local size=$3
    local threshold=$4
    
    echo "    Test: $name    "
    echo "Files: ${count}x${size}MB, Threshold: ${threshold}%"
    unmount_func
    create_test_disk
    create_test_files $count $size
    echo "Before: $(df -h "$WPATH" | tail -1 | awk '{print "Used " $3 " (" $5 ")"}')"
    "$MAIN_SCRIPT" "$WPATH" "$BACKUP" $threshold
    echo "After: $(df -h "$WPATH" | tail -1 | awk '{print "Used " $3 " (" $5 ")"}')"
    echo "Archives created: $(ls "$BACKUP"/*.tar.* 2>/dev/null | wc -l)"
    echo ""
}

test_lzma() 
{
    echo "    Test: LZMA Compression    "
    
    unmount_func
    create_test_disk
    create_test_files 8 100
    export LAB1_MAX_COMPRESSION=1
    "$MAIN_SCRIPT" "$WPATH" "$BACKUP" 50
    unset LAB1_MAX_COMPRESSION
    
    lzma_count=$(ls "$BACKUP"/*.tar.lzma 2>/dev/null | wc -l)
    echo "LZMA archives: $lzma_count"
    echo ""
}

if [ ! -f "$MAIN_SCRIPT" ]; then
    echo "Error: script1.sh not found"
    exit 1
fi

chmod +x "$MAIN_SCRIPT"

echo "Starting tests..." 

run_test "Low disk usage" 10 50 90

run_test "Medium disk usage" 7 80 65

run_test "High disk usage" 10 70 40

run_test "MAX disk usage" 15 60 30

test_lzma

unmount_func
echo "All Tests completed!"