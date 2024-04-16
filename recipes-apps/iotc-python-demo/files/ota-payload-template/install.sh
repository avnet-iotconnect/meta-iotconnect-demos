#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

application_payload_dir="$SCRIPT_DIR/application/"
local_data_payload_dir="$SCRIPT_DIR/local_data/"

# Source directory to be backed up
application_installed_dir="/usr/iotc/bin/iotc-python-sdk"
local_data_installed_dir="/usr/iotc/local/"
application_backup_dir="${application_installed_dir::-1}.backup/"
local_data_backup_dir="${local_data_installed_dir::-1}.backup/"

tuples=("$application_installed_dir $application_backup_dir" "$local_data_installed_dir $local_data_backup_dir")
for tuple in "${tuples[@]}"; do
    eval "tuple=($tuple)"
    installed_dir="${tuple[0]}"
    backup_dir="${tuple[1]}"

    if [ -d "$backup_dir" ]; then
        echo "Removing existing backup... at $backup_dir"
        rm -r "$backup_dir"
    fi

    echo "Creating backup directories at $backup_dir"
    mkdir -p "$backup_dir"

    echo "Backing up $installed_dir to $backup_dir"
    cp -va "$installed_dir". "$backup_dir"

    # replace paths in backed up config etc to use new backup directory
    if [ -d "$backup_dir" ]; then
        echo "Replacing paths in $backup_dir"
        find "$backup_dir" -type f -print | while read -r file; do
            sed -i "s|$installed_dir|$backup_dir|g" "$file"
        done
    else
        echo "$backup_dir not found."
    fi
done

tuples=("$application_installed_dir $application_payload_dir" "$local_data_installed_dir $local_data_payload_dir")
echo "PAYLOADINSTALL START"
for tuple in "${tuples[@]}"; do
    eval "tuple=($tuple)"
    to_install_dir="${tuple[0]}"
    payload_dir="${tuple[1]}"

    cp -va $payload_dir. $to_install_dir
done
echo "PAYLOADINSTALL END"

# Check if the write was successful
if [ $? -eq 0 ]; then
    echo "install.sh completed successfully."
else
    >&2 echo "install.sh encountered errors."
    exit 1
fi