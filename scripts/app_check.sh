#!/bin/bash
# -x  Print commands and their arguments as they are executed.
# +e  Not to exit immediately if a command exits with a non-zero status.
set +e

# Check if arguments are provided
if [ $# -ne 1 ]; then
    echo "Usage: $0 <max wait time in seconds>"
    exit 1
fi

max_attempts=$1

counter=0

while [ $counter -lt $max_attempts ]; do
    ((counter++))

    # Read system log
    log=$(adb shell logcat -d)

    # Scan for Java layer error
    app_err=$(echo $log | grep "E IFXAPP")
    if [[ -n "$app_err" ]]; then
        echo "Error found: $app_err"
        exit 1
    fi

    # Scan for JNI layer error
    jni_err=$(echo $log | grep "E com.ifx.nave")
    if [[ -n "$jni_err" ]]; then
        echo "Error found: $jni_err"
        exit 1
    fi

    # Scan for app exit
    exit_log=$(echo $log | grep "MainActivity.jniTest exit")
    if [[ -n "$exit_log" ]]; then
        echo "The application has exited normally without reporting any errors."
        exit 0
    fi

    remaining_attempts=$((max_attempts - counter))
    echo "Try again after 1 second... (remaining attempts: ${remaining_attempts})"

    sleep 1
done

echo "Timeout reached. No application activity detected."

exit 1