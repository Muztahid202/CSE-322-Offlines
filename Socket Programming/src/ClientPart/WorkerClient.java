package ClientPart;

import java.io.*;
import java.net.Socket;

public class WorkerClient extends Thread {
    private String fileName;
    static final int CHUNK_SIZE = 1024; // Chunk size for file transfer
    static final String SERVER_ADDRESS = "localhost"; // Server address
    static final int PORT = 5067; // Server port

    public WorkerClient(String fileName) {
        this.fileName = fileName;
    }

    @Override
    public void run() {
        try {
            Socket socket = new Socket(SERVER_ADDRESS, PORT);
           // System.out.println("asi befor fileInputStream");
            FileInputStream fileInputStream = new FileInputStream(fileName);
           // System.out.println("asi after fileInputStream");
            BufferedOutputStream outStream = new BufferedOutputStream(socket.getOutputStream());
            BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()));

            File file = new File(fileName);
            if (!file.exists()) {
                System.err.println("File not found: " + fileName);
                return;
            }

            // Send file upload request
            PrintWriter writer = new PrintWriter(socket.getOutputStream(), true);
            writer.println("UPLOAD " + fileName);

            // Read server response
            String response = in.readLine();
            //System.out.println("Response: " + response);
            if (response != null && response.startsWith("ERROR")) {
                System.out.println("Server error: " + response);
                return;
            }

            // Upload file in chunks
            byte[] buffer = new byte[CHUNK_SIZE];
            int bytesRead;
            while ((bytesRead = fileInputStream.read(buffer)) != -1) {
                outStream.write(buffer, 0, bytesRead);
            }
           // System.out.println("before flush");
            outStream.flush();

           // System.out.println("File upload completed: " + fileName);

        } catch (IOException e) {
            System.err.println("Error uploading file: " + fileName);
            e.printStackTrace();
        }
    }
}
