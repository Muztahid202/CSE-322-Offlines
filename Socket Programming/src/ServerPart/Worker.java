package ServerPart;

import java.io.*;
import java.net.SocketException;
import java.net.Socket;
import java.nio.file.Files;
import java.text.SimpleDateFormat;
import java.util.Date;

public class Worker extends Thread {
    Socket socket;
    static final int CHUNK_SIZE = 1024;  // Define chunk size for file transfer
    static final String ROOT_DIR = "src/ServerPart/root";  // Root directory for the web server
    static final String UPLOADED_DIR = "src/ServerPart/uploaded";  // Directory for uploaded files
    static final String LOG_FILE = "src/ServerPart/logFile.txt"; // Log file for request and response logs
    static final String[] ALLOWED_EXTENSIONS = {".txt", ".jpg", ".png", ".mp4"}; // Allowed file extensions

    public Worker(Socket socket) {
        this.socket = socket;
    }

    public void run() {
        try {
            BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
            PrintWriter pr = new PrintWriter(socket.getOutputStream());
            OutputStream outStream = socket.getOutputStream();


            // Read HTTP request
            String requestLine = in.readLine();
          //  System.out.println("Request: " + requestLine);
            if (requestLine == null) {
                sendError(pr, 400, "Bad Request");
                return;
            }
            if (requestLine.startsWith("UPLOAD")) {
                // Process file upload request
                String[] tokens = requestLine.split(" ");
                if (tokens.length != 2) {
                    sendErrorToClient(pr, "Invalid upload command");
                    return;
                }

                String fileName = tokens[1];
                if (!isValidFile(fileName)) {
                    sendErrorToClient(pr, "Invalid file format");
                    return;
                }

                File destinationFile = new File(UPLOADED_DIR + File.separator + fileName);
              //  System.out.println("function call er age");
                handleFileUpload(pr, outStream, destinationFile);
            } else if (requestLine.startsWith("GET")) {
                // Handle HTTP GET request
                handleGetRequest(in, pr, outStream, requestLine);
            } else {
                // Invalid request
                sendError(pr, 400, "Bad Request");
            }
           
            // if(requestLine.startsWith("GET")){
            // // Close resources
            // in.close();
            // pr.close();
            // socket.close();
            // }
        } catch (SocketException e) {
            // Log the disconnection and handle cleanup
            System.err.println("Client disconnected: " + e.getMessage());
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            try {
                socket.close();  // Ensure socket is closed
            } catch (IOException e) {
                System.err.println("Error closing socket: " + e.getMessage());
            }
        }
    }

    private void handleGetRequest(BufferedReader in, PrintWriter pr, OutputStream outStream, String requestLine) throws IOException {
         // Log request
         logRequest(requestLine);

         // Extract the requested path
         String[] tokens = requestLine.split(" ");
         String filePath = tokens[1];
        // System.out.println("Request for: " + filePath);
         if (filePath.equals("/")) {
            String response = sendDirectoryListing(pr, new File(ROOT_DIR), "");
            logResponse(response); // Log the response
            return;
         }
         String reqFilePath = ROOT_DIR + filePath;
         File requestedFile = new File(reqFilePath);

       //  System.out.println("requested file path " +  ROOT_DIR + filePath);

         // Handle directory listing
         if (requestedFile.isDirectory()) {
             String response = sendDirectoryListing(pr, requestedFile, filePath);
             logResponse(response); // Log the response
         } else if (requestedFile.exists() && requestedFile.isFile()) {
             // Handle file request
             String mimeType = getMimeType(requestedFile);
             if (mimeType.startsWith("text/") || mimeType.startsWith("image/")) {
                 String response = sendFile(pr, outStream, requestedFile, mimeType);
                 logResponse(response); // Log the response
             } else {
                 // Force download for non-text/image files
                 String response = sendFileDownload(pr, outStream, requestedFile);
                 logResponse(response); // Log the response
             }
         } else {
             // File not found
             System.out.println("404 : path not found");
             String response = sendError(pr, 404, "Not Found");
             logResponse(response); // Log the response
         }
    }

    private void handleFileUpload(PrintWriter pr, OutputStream outStream, File destinationFile) throws IOException {
        // Ensure the parent directory exists
        File uploadedDir = new File(UPLOADED_DIR);
        if (!uploadedDir.exists()) {
            uploadedDir.mkdirs(); // Create the directory if it doesn't exist
        }
        //System.out.println("File upload request received: " + destinationFile.getName());

        String filePath = UPLOADED_DIR + "/" + destinationFile.getName();
        // System.out.println("File path: " + filePath);
        File file = new File(filePath);

        pr.write("OK"+ "\n");
        pr.flush();

        try (FileOutputStream fileOutputStream = new FileOutputStream(file);
             BufferedInputStream fileInputStream = new BufferedInputStream(socket.getInputStream())) {

            byte[] buffer = new byte[CHUNK_SIZE];
            int bytesRead;
            while ((bytesRead = fileInputStream.read(buffer)) != -1) {
                fileOutputStream.write(buffer, 0, bytesRead);
            }
            fileOutputStream.flush();
        }

        String response = "File upload completed: " + destinationFile.getName();
        pr.println(response);
       // logResponse(response);
    }

    private boolean isValidFile(String fileName) {
        for (String ext : ALLOWED_EXTENSIONS) {
            if (fileName.endsWith(ext)) {
                return true;
            }
        }
        return false;
    }

    private void sendErrorToClient(PrintWriter pr, String errorMessage) {
        String response = "ERROR: " + errorMessage;
        pr.write(response + "\n");
        pr.flush();
       // logResponse(response);
       // return response;
    }

    private String sendDirectoryListing(PrintWriter pr, File directory, String path) throws IOException {
        File[] files = directory.listFiles();
        if (files == null) {
            System.out.println("404 : Directory not found");
            return sendError(pr, 404, "Directory Not Found");
        }

        StringBuilder content = new StringBuilder("<html><body><h1>Directory Listing</h1><ul>");
        for (File file : files) {
            if (file.isDirectory()) {
                content.append("<li><b><i><a href=\"")
                        .append(path + "/" + file.getName())
                        .append("/\">")
                        .append(file.getName())
                        .append("</a></i></b></li>");
            } else {
                content.append("<li><a href=\"")
                        .append(path + "/" + file.getName())
                        .append("\">")
                        .append(file.getName())
                        .append("</a></li>");
            }
        }
        content.append("</ul></body></html>");

        String response = buildHttpResponse(200, "text/html", content.toString().length());
        pr.write(response);
        pr.write(content.toString());
        pr.flush();
        
        return response + content.toString(); // Return the full response for logging
    }

    private String sendFile(PrintWriter pr, OutputStream outStream, File file, String mimeType) throws IOException {
        String response = buildHttpResponse(200, mimeType, file.length());
        pr.write(response);
        pr.flush();

        sendFileInChunks(outStream, file);
        return response; // Return the response for logging
    }

    private String sendFileDownload(PrintWriter pr, OutputStream outStream, File file) throws IOException {
        String mimeType = "application/octet-stream";
        String response = buildHttpResponse(200, mimeType, file.length());
        pr.write(response);
        pr.flush();

        sendFileInChunks(outStream, file);
        return response; // Return the response for logging
    }

    private void sendFileInChunks(OutputStream outStream, File file) throws IOException {
        FileInputStream fileInputStream = new FileInputStream(file);
        byte[] buffer = new byte[CHUNK_SIZE];
        int bytesRead;
        while ((bytesRead = fileInputStream.read(buffer)) != -1) {
            outStream.write(buffer, 0, bytesRead);
        }
        outStream.flush();
        fileInputStream.close();
    }

    private String sendError(PrintWriter pr, int statusCode, String statusMessage) {
        String content = "<html><body><h1>Error " + statusCode + ": " + statusMessage + "</h1></body></html>";
        String response = buildHttpResponse(statusCode, "text/html", content.length());
        pr.write(response);
        pr.write(content);
        pr.flush();
        
        return response + content; // Return the full response for logging
    }

    private String buildHttpResponse(int statusCode, String contentType, long contentLength) {
        String reasonPhrase = getReasonPhrase(statusCode);  // Get the correct reason phrase for the status code
        return "HTTP/1.0 " + statusCode + " " + reasonPhrase + "\r\n" +
                "Date: " + new Date() + "\r\n" +
                "Server: Java HTTP Server 1.0\r\n" +
                "Content-Type: " + contentType + "\r\n" +
                "Content-Length: " + contentLength + "\r\n" +
                "Connection: close\r\n\r\n";
    }

    private String getReasonPhrase(int statusCode) {
        switch (statusCode) {
            case 200: return "OK";
            case 400: return "Bad Request";
            case 403: return "Forbidden";
            case 404: return "Not Found";
            case 500: return "Internal Server Error";
            default: return "Unknown Status";
        }
    }

    private String getMimeType(File file) throws IOException {
        return Files.probeContentType(file.toPath());
    }

    private void logRequest(String request) {
        try (FileWriter logWriter = new FileWriter(LOG_FILE, true);
             BufferedWriter bufferedLogWriter = new BufferedWriter(logWriter);
             PrintWriter logPrinter = new PrintWriter(bufferedLogWriter)) {

            logPrinter.println("Request:");
            logPrinter.println("Timestamp: " + new SimpleDateFormat("yyyy-MM-dd HH:mm:ss").format(new Date()));
            logPrinter.println(request);
            logPrinter.println("----------------------------------------------------");
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void logResponse(String response) {
        try (
             FileWriter logWriter = new FileWriter(LOG_FILE, true);
             BufferedWriter bufferedLogWriter = new BufferedWriter(logWriter);
             PrintWriter logPrinter = new PrintWriter(bufferedLogWriter)) {

            logPrinter.println("Response:");
            logPrinter.println("Timestamp: " + new SimpleDateFormat("yyyy-MM-dd HH:mm:ss").format(new Date()));
            logPrinter.println(response);
            logPrinter.println("----------------------------------------------------");
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}