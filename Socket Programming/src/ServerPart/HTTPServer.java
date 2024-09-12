package ServerPart;

import java.io.*;
import java.net.ServerSocket;
import java.net.Socket;

public class HTTPServer {
    static final int PORT = 5067;

    public static void main(String[] args) throws IOException {
        ServerSocket serverConnect = new ServerSocket(PORT);
        System.out.println("Server started.\nListening for connections on port : " + PORT + " ...\n");

        while (true) {
            try {
                // Accept the connection
                Socket socket = serverConnect.accept();
                System.out.println("Connection established with client");

                // Open worker thread
                Thread worker = new Worker(socket);
                System.out.println("ServerPart.Worker created for client");

                worker.start();
                System.out.println("ServerPart.Worker started for client");
            } catch (IOException e) {
                System.out.println("Error establishing connection: " + e.getMessage());
                e.printStackTrace();
            }
        }
    }
}
