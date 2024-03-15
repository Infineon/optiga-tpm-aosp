#!/bin/bash
# -x  Print commands and their arguments as they are executed.
# +e  Not to exit immediately if a command exits with a non-zero status.
set +e

# Check if arguments are provided
if [ $# -ne 3 ]; then
    echo "Usage: $0 <android emulator pid> <desired android version> <max wait time in seconds>"
    exit 1
fi

pid=$1
desired_version=$2
max_attempts=$3

counter=0

while [ $counter -lt $max_attempts ]; do
    ((counter++))

    # Get the Android version
    version=$(adb shell getprop ro.system.build.version.release)

    # Check if Android is ready for use
    bootanim=$(adb shell getprop init.svc.bootanim)

    # Check if the version matches the desired version
    # and Android is ready for use.
    if [[ "$version" == "$desired_version" && "$bootanim" == "stopped" ]]; then
        echo "Desired Android version $desired_version detected."
        exit 0
    fi

    remaining_attempts=$((max_attempts - counter))
    echo "Try again after 1 second... (remaining attempts: ${remaining_attempts})"

    sleep 1
done

echo "Timeout reached. Android version $desired_version not detected, or the package manager service has not started."
echo "Attempt to kill the Android emulator (pid: ${pid})."
kill -SIGTERM $pid

exit 1
