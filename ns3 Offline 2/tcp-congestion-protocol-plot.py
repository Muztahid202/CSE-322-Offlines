import matplotlib.pyplot as plt
import os

def plot_data(file_path, output_dir, prefix):
    """
    Plots the data from the given file and saves it to a subdirectory within the output directory.

    Args:
        file_path (str): Path to the .data file.
        output_dir (str): Directory to save the plot.
        prefix (str): The prefix to create a subdirectory under output_dir.
    """
    try:
        # Read the file
        with open(file_path, 'r') as f:
            lines = f.readlines()

        # Parse data into two lists: time and metric value
        times = []
        values = []
        for line in lines:
            parts = line.strip().split()
            if len(parts) == 2:  # Ensure valid data format
                times.append(float(parts[0]))
                values.append(float(parts[1]))

        # Extract metric and label from filename
        filename = os.path.basename(file_path)
        metric = filename.split('-')[-1].replace('.data', '')  # Extract the metric (e.g., cwnd, rtt)
        label = filename.replace('.data', '')  # Use the full filename without extension as label

        # Plot the data
        plt.figure(figsize=(10, 6))
        plt.plot(times, values, label=label)
        plt.xlabel('Time (s)')
        plt.ylabel(f"{metric.upper()} Value")
        plt.title(f"{metric.upper()} Plot for {filename}")
        plt.legend()
        plt.grid(True)

        # Save the plot in a subdirectory within output_dir
        sub_output_dir = os.path.join(output_dir, prefix)
        os.makedirs(sub_output_dir, exist_ok=True)
        plot_file = os.path.join(sub_output_dir, filename.replace('.data', '.png'))
        plt.savefig(plot_file)
        plt.close()

        print(f"Plot saved as: {plot_file}")

    except Exception as e:
        print(f"Error processing file {file_path}: {e}")

def main(prefix):
    """
    Main function to plot all .data files in the current directory matching the given prefix
    and save the plots to scratch/graphs/prefix.

    Args:
        prefix (str): The prefix for selecting .data files (e.g., TcpVegas, TcpBbr).
    """
    # Directory containing .data files
    data_dir = '.'  # Change this if files are in a different directory
    output_dir = 'scratch/graphs'  # Directory to save the plots

    # List all .data files matching the prefix in the directory
    data_files = [
        f for f in os.listdir(data_dir)
        if f.endswith('.data') and f.split('-')[0] == prefix
    ]

    if not data_files:
        print(f"No .data files found with prefix '{prefix}'.")
        return

    # Plot each .data file
    for data_file in data_files:
        plot_data(os.path.join(data_dir, data_file), output_dir, prefix)

if __name__ == "__main__":
    prefix = input("Enter the prefix (e.g., TcpVegas or TcpBbr): ")
    main(prefix)
