#!/bin/bash

if [ $# -lt 2 ]; then
    echo "Usage: $0 <LPATH> <backup_path> [threshold]"
    exit 1
fi

LPATH="$1"
BACKUP_PATH="$2"
THRESHOLD="${3:-70}"
if [ ! -d "$LPATH" ]; then
    echo "ERROR: Log directory not found: $LPATH"
    exit 1
fi
if [ ! -d "$BACKUP_PATH" ]; then
    echo "ERROR: Backup directory not found: $BACKUP_PATH"
    exit 1
fi
if ! [[ "$THRESHOLD" =~ ^[0-9]+$ ]] || [ "$THRESHOLD" -lt 1 ] || [ "$THRESHOLD" -gt 100 ]; then
    echo "ERROR: Threshold must be 1-100"
    exit 1
fi

DISK_INFO=$(df "$LPATH" | tail -1)
USED_PERCENT=$(echo "$DISK_INFO" | awk '{print $5}' | sed 's/%//')

echo "Disk usage: ${USED_PERCENT}%"

if [ "$USED_PERCENT" -gt "$THRESHOLD" ]; then
    echo "Threshold exceeded (${USED_PERCENT}% > ${THRESHOLD}%). Starting archiving..."
    TEMP_FILE=$(mktemp)
    find "$LPATH" -maxdepth 1 -type f -printf "%T@ %p\n" | sort -n | cut -d' ' -f2- > "$TEMP_FILE"    
    TOTAL_FILES=$(wc -l < "$TEMP_FILE")
    if [ "$TOTAL_FILES" -eq 0 ]; then
        echo "No files to archive"
        rm -f "$TEMP_FILE"
        exit 0
    fi
    echo "Found $TOTAL_FILES files"
    if [ "${LAB1_MAX_COMPRESSION}" = "1" ]; then
        COMPRESSION="--lzma"
        EXT="tar.lzma"
        echo "Using LZMA compression"
    else
        COMPRESSION="-z"
        EXT="tar.gz"
        echo "Using GZIP compression"
    fi
    files_arch=0
    DATETIME=$(date +%Y%m%d_%H%M%S)
    
    while IFS= read -r filepath && [ "$(df "$LPATH" | tail -1 | awk '{print $5}' | sed 's/%//')" -gt "$THRESHOLD" ]; do
        if [ -f "$filepath" ]; then
            filename=$(basename "$filepath")
            archive_name="backup_${DATETIME}_$(printf "%03d" $((files_arch + 1))).${EXT}"
            
            if [ "$COMPRESSION" = "--lzma" ]; then
                tar --lzma -cf "$BACKUP_PATH/$archive_name" -C "$LPATH" "$filename"
            else
                tar -czf "$BACKUP_PATH/$archive_name" -C "$LPATH" "$filename"
            fi
            
            if [ $? -eq 0 ]; then
                rm -f "$filepath"
                files_arch=$((files_arch + 1))
                echo "files_arch: $filename"
            else
                echo "Failed to archive: $filename"
                break
            fi
        fi
    done < "$TEMP_FILE"
    rm -f "$TEMP_FILE"
    F_PCNT=$(df "$LPATH" | tail -1 | awk '{print $5}' | sed 's/%//')
    echo "Archiving completed. Files files_arch: $files_arch"
    echo "FINAL USAGE: ${F_PCNT}%"
else
    echo "Usage threshold is not exceeded. No actions done."
fi