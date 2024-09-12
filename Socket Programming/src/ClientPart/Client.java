package ClientPart;

import java.util.Scanner;

public class Client {
    static final int PORT = 5067; // Server port
    static final String SERVER_ADDRESS = "localhost"; // Server address

    public static void main(String[] args) {
            while (true) {
                Scanner scanner = new Scanner(System.in);
                
                System.out.print("Enter file name to upload (or type 'exit' to quit): ");
                String fileName = scanner.nextLine();

                if (fileName.equalsIgnoreCase("exit")) {
                    break;
                }

                // Add prompt before file upload starts to ensure timely prompt
                System.out.println("Starting upload for file: " + fileName);
                
                // Assuming files are located in the src/ClientPart/ directory
                fileName = "src/ClientPart/" + fileName;

                // Start the file upload in a separate thread (non-blocking)
                WorkerClient workerClient = new WorkerClient(fileName);
                workerClient.start(); // This starts the file upload in parallel

                // Prompt to continue accepting user input while the file uploads in the background
               // System.out.println("You can now enter the next file name.");
            }
        }
    }
