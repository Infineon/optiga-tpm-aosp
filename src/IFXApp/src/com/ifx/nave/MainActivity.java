/**
 * MIT License
 *
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE
 */

package com.ifx.nave;

import java.io.BufferedReader;
import java.io.InputStreamReader;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.util.Log;
import android.widget.EditText;

import com.ifx.nave.JavaNative;

public class MainActivity extends Activity {

    private String DEBUG_TAG = "IFXAPP";
    private EditText editText = null;
    private Runnable runnable = null;
    private Process process = null;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        View view = getLayoutInflater().inflate(R.layout.main_activity, null);
        setContentView(view);

        editText = (EditText) findViewById(R.id.edit_text);
        Log.d(DEBUG_TAG, "MainActivity.onCreate done...");
        editText.append("\nMainActivity.onCreate done...");

        runnable = new Runnable() {
            @Override
            public void run() {
                try {
                    /* Launch the TPM simulator. */
                    launchMssim();

                    /* Delayed execution to ensure the TPM simulator is ready. */
                    Thread.sleep(2000);

                    jniTest();
                } catch (Exception e) {
                    editText.append("\nMainActivity exception: " + e.getClass().getCanonicalName());
                    Log.e(DEBUG_TAG, "MainActivity exception: " + e.getClass().getCanonicalName());
                    e.printStackTrace();
                }
            }
        };

        new Thread(runnable).start();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        if (process != null) {
            process.destroy();
        }
    }

    private void launchMssim() throws Exception {
        int exitCode;
        int seconds = 0;
        String[] cmd = {
            "/system/bin/sh",
            "-c",
            "netstat | grep -E \"localhost:2321|localhost:2322\" | grep TIME_WAIT"
        };

        while (true) {
            /* When the tpm2-simulator terminates after its last use, the associated sockets enter the TIME_WAIT state.
               Wait for these sockets to become available again. */
            Process proc = Runtime.getRuntime().exec(cmd);
            exitCode = proc.waitFor();
            if (exitCode != 0) {
                Log.d(DEBUG_TAG, "Starting tpm2-simulator...");
                break;
            }

            /* Read the output of the command */
            BufferedReader reader = new BufferedReader(new InputStreamReader(proc.getInputStream()));
            StringBuilder output = new StringBuilder();
            String line;

            while ((line = reader.readLine()) != null) {
                output.append(line).append("\n");
            }

            Log.d(DEBUG_TAG, "exitCode = " + exitCode);
            Log.d(DEBUG_TAG, "Command output:\n" + output.toString());
            Log.d(DEBUG_TAG, "Waiting for sockets (2321, 2322) to exit the TIME_WAIT state...(waited " + seconds + " seconds)");

            if (seconds++ == 0) {
                editText.append("\nMainActivity.launchMssim waiting for sockets to exit the TIME_WAIT state...");
            }

            Thread.sleep(1000);
        }

        Thread.sleep(1000);
        process = Runtime.getRuntime().exec("/system/bin/tpm2-simulator");
    }

    private void jniTest() throws Exception {
        Log.d(DEBUG_TAG, "MainActivity.jniTest enter...");
        editText.append("\nMainActivity.jniTest enter...");

        JavaNative jn = new JavaNative();
        jn.test();

        Log.d(DEBUG_TAG, "MainActivity.jniTest exit...");
        editText.append("\nMainActivity.jniTest exit...");
    }
}

