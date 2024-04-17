#!/bin/bash
app_name="iotc-python-demo.py"
config_name="config.json"

app_A_dir="/usr/iotc/bin/iotc-python-sdk/"
config_A_dir="/usr/iotc/local/"
app_B_dir="${app_A_dir::-1}.backup/"
config_B_dir="${config_A_dir::-1}.backup/"

run_app() {
    /usr/bin/python3 -u $1 $2
}

restore_backup_to_primary() {
    rm -rf $app_A_dir
    rm -rf $config_A_dir

    if [ -d "$app_A_dir" ]; then
        echo "Removing existing primary app at $app_A_dir"
        rm -r "$app_A_dir"
    fi

    if [ -d "$config_A_dir" ]; then
        echo "Removing existing primary config at $config_A_dir"
        rm -r "$config_A_dir"
    fi

    echo "Setting backup as primary"
    mkdir -p $app_A_dir && cp -a $app_B_dir. $app_A_dir 
    mkdir -p $config_A_dir && cp -a $config_B_dir. $config_A_dir 

}

recover_primary()
{
    echo "Primary app has failed, recovering"
    restore_backup_to_primary
    echo "Running backup"
    run_app $app_A_dir$app_name $config_A_dir$config_name
}

SECONDARY_EXISTS=false
SECONDARY_PATH=$app_B_dir$app_name
if [ -e "$SECONDARY_PATH" ]; then
    SECONDARY_EXISTS=true
fi

PRIMARY_EXISTS=false
PRIMARY_PATH=$app_A_dir$app_name
if [ -e "$PRIMARY_PATH" ]; then
    PRIMARY_EXISTS=true
fi

if [ "$PRIMARY_EXISTS" = true ] && [ "$SECONDARY_EXISTS" = true ]; then
    echo "Primary and Secondary exist, running"
    run_app $app_A_dir$app_name $config_A_dir$config_name || recover_primary


elif [ "$PRIMARY_EXISTS" = true ]; then
    echo "Only Primary exists, running"
    run_app $app_A_dir$app_name $config_A_dir$config_name

elif [ "$SECONDARY_EXISTS" = true ]; then
    echo "Only Secondary exists, running"
    run_app $app_B_dir$app_name $config_B_dir$config_name 

else
    echo "No valid application exists to run"
fi
