/*
 * Copyright (C) 2010 The Android Open Source Project
 * Copyright (c) 2011, NVIDIA CORPORATION. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.webkit.dumprendertree;


import android.util.Log;
import java.util.HashSet;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.net.ServerSocket;
import java.lang.RuntimeException;
import java.lang.Thread;

/** Forwards a TCP port from device to host computer.
 *
 * A class that forwards one TCP port from device to host computer.
 * The forwarding is done using adb.
 *
 * Forwards the a port on device to the same port on the host running
 * the adb connection.
 *
 * The forwarding is stopped when either of the sides closes the connection.
 * The thread that exits when the forwarding stops.
 */
public class AdbPortForwarder extends Thread {
    private static final String LOG_TAG = "AdbPortForwarder";

    private static final String ADB_OK = "OKAY";
    private static final int ADB_PORT = 5037;
    private static final String ADB_HOST = "127.0.0.1";
    private static final int ADB_RESPONSE_SIZE = 4;

    private static final String REMOTE_ADDRESS = "127.0.0.1";

    private ServerSocket mServerSocket;
    private Socket mAdbSocket;
    private int mPort;
    private HashSet<PairedSockets> mRunningPairedSockets;


    public AdbPortForwarder(int port) {
        mPort = port;
    }

    public void requestShutdown() {
        if (!mServerSocket.isClosed()) {
            try {
                mServerSocket.close();
            } catch (IOException e) {}
        }
    }

    @Override
    public void run() {
        try {
            mServerSocket = new ServerSocket(mPort);
            mServerSocket.setReceiveBufferSize(4096);
        } catch (IOException e) {
            Log.e(LOG_TAG, "Unable to start port forwarding to port " + mPort);
            return;
        }

        while (true) {
            Socket localQuerySocket = null;
            Socket adbSocket = null;
            try {
                localQuerySocket = mServerSocket.accept();
                localQuerySocket.setSendBufferSize(4096);
                adbSocket = createAdbSocket();

                pairSocketsAndStart(localQuerySocket, adbSocket);

            } catch (Exception e) {
                try {
                    if (localQuerySocket != null)
                        localQuerySocket.close();
                } catch (IOException ioe) {}

                try {
                    if (adbSocket != null)
                        adbSocket.close();
                } catch (IOException ioe) {}
                break;
            }
        }

        HashSet<PairedSockets> runningPairs;
        synchronized(this) {
            runningPairs = mRunningPairedSockets;
            mRunningPairedSockets = null;
        }
        if (runningPairs != null) {
            // Close the running sockets. This notifies the threads to stop too.
            for (PairedSockets runningPair: runningPairs)
                runningPair.stopForwarding();
        }
    }

    private Socket createAdbSocket() throws IOException {
        Socket adbSocket = new Socket(ADB_HOST, ADB_PORT);
        adbSocket.setReceiveBufferSize(4096);
        adbSocket.setSendBufferSize(4096);
        String cmd = "tcp:" + mPort + ":" + REMOTE_ADDRESS;
        cmd = String.format("%04X", cmd.length()) + cmd;

        byte[] buf = new byte[ADB_RESPONSE_SIZE];
        adbSocket.getOutputStream().write(cmd.getBytes());
        adbSocket.getOutputStream().flush();
        int read = adbSocket.getInputStream().read(buf);
        if (read != ADB_RESPONSE_SIZE || !ADB_OK.equals(new String(buf))) {
            adbSocket.close();
            throw new RuntimeException("Unable to start port forwarding to port " + mPort);
        }
        return adbSocket;
    }

    synchronized private void pairSocketsAndStart(Socket a, Socket b) {
        if (mRunningPairedSockets == null)
            mRunningPairedSockets = new HashSet<PairedSockets>();

        PairedSockets pair = new PairedSockets(a, b, this);
        mRunningPairedSockets.add(pair);
        pair.startForwarding();
    }

    synchronized private void pairedSocketsForwardingStopped(PairedSockets pair) {
        if (mRunningPairedSockets != null)
            mRunningPairedSockets.remove(pair);
    }

    private class PairedSockets {
        Socket mA;
        Socket mB;
        AdbPortForwarder mOwner;

        public PairedSockets(Socket a, Socket b, AdbPortForwarder owner) {
            mA = a;
            mB = b;
            mOwner = owner;
        }

        /** Starts forwarding for the sockets.
         *
         * Closing the socket will notify the thread to stop.  Closing either socket will cause both
         * threads to stop.
         */
        public void startForwarding() {
            new Thread() {
                @Override
                public void run() {
                    forwardUntilEnd(mA, mB);
                }
            }.start();

            new Thread() {
                @Override
                public void run() {
                    forwardUntilEnd(mB, mA);
                }
            }.start();
        }

        /** Forwards data from input to output-
         *
         * Stops immediately when either of the streams fail to deliver data.
         */
        private void forwardUntilEnd(Socket input, Socket output) {
            try {
                InputStream i = input.getInputStream();
                OutputStream o = output.getOutputStream();

                byte[] buffer = new byte[4096];
                int length;
                while (true) {
                    if ((length = i.read(buffer)) < 0)
                        break;
                    o.write(buffer, 0, length);
                    o.flush();
                }
            } catch (IOException e) { }

            /* Close both of the connections immediately when either of them closes.
             * This is important when a test times out and server closes HTTP keep-alive
             * connection. If we don't close the local socket, the client will
             * send the next query to this socket and it will be lost.
             */

            if (stopForwarding()) {
                // Inform the owner that these sockets do not need closing.
                mOwner.pairedSocketsForwardingStopped(this);
            }
        }

        synchronized public boolean stopForwarding() {
            if (mA != null) {
                try {
                    if (!mA.isClosed())
                        mA.close();
                } catch (IOException e) {}

                try {
                    if (!mB.isClosed())
                        mB.close();
                } catch (IOException e) {}

                mA = null;
                mB = null;
                return true;
            }
            return false;
        }
    }
}


