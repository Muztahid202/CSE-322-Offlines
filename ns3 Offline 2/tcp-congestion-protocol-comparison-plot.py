import matplotlib.pyplot as plt
import os


def plot_combined_data(data_files, output_dir, output_file, ylabel, title):
    """
    Plots data from multiple .data files on the same graph for comparison.

    Args:
        data_files (list): List of tuples containing file paths and labels.
        output_dir (str): Directory to save the combined plot.
        output_file (str): Name of the output file (e.g., 'cwnd_comparison.png').
        ylabel (str): Label for the y-axis.
        title (str): Title of the plot.
    """
    try:
        plt.figure(figsize=(12, 8))

        # Loop through each file and plot its data
        for file_path, label in data_files:
            with open(file_path, 'r') as f:
                lines = f.readlines()

            times = []
            values = []
            for line in lines:
                parts = line.strip().split()
                if len(parts) == 2:  # Ensure valid data format
                    times.append(float(parts[0]))
                    values.append(float(parts[1]))

            # Plot data with a label
            print(f"Plotting: {file_path}, Label: {label}")
            plt.plot(times, values, label=label)

        # Add labels, title, and legend
        plt.xlabel('Time (s)')
        plt.ylabel(ylabel)
        plt.title(title)
        plt.legend()
        plt.grid(True)

        # Save the combined plot
        os.makedirs(output_dir, exist_ok=True)
        output_path = os.path.join(output_dir, output_file)
        plt.savefig(output_path)
        plt.close()

        print(f"Combined plot saved as: {output_path}")

    except Exception as e:
        print(f"Error creating combined plot: {e}")


def main():
    """
    Main function to combine plots for different metrics and algorithms.
    """
    # Define the prefixes for each congestion control algorithm
    prefixes = ["TcpVegas", "TcpBbr", "TcpVegasTweaked"]

    # Define the metrics to plot
    metrics = {
        "cwnd": "Congestion Window (cwnd)",
        "inflight": "Inflight Data",
        "next-rx": "Next RX",
        "next-tx": "Next TX",
        "rto": "Retransmission Timeout (RTO)",
        "rtt": "Round Trip Time (RTT)",
        "ssth": "Slow Start Threshold (ssth)"
    }

    # Directory containing .data files
    data_dir = '.'  # Change this if files are in a different directory
    output_dir = 'scratch/graphs/comparison'  # Directory to save the combined plots

    for metric, ylabel in metrics.items():
        # Separate data files by flow
        flow_data = {"flow0": [], "flow1": []}

        for prefix in prefixes:
            matching_files = [
                f for f in os.listdir(data_dir)
                if f.startswith(prefix + "-") and f.endswith('.data') and metric in f.lower()
            ]

            for file_name in matching_files:
                file_path = os.path.join(data_dir, file_name)
                if "flow0" in file_name:
                    flow_data["flow0"].append((file_path, prefix))
                elif "flow1" in file_name:
                    flow_data["flow1"].append((file_path, prefix))

        # Deduplicate data files
        for flow in flow_data:
            flow_data[flow] = list(set(flow_data[flow]))

        # Plot data for each flow
        for flow, data_files in flow_data.items():
            if len(data_files) < 2:
                print(f"Not enough data files to create a comparison plot for '{metric}' ({flow}).")
                continue

            # Plot combined data for the current metric and flow
            plot_combined_data(
                data_files,
                output_dir,
                f"{metric}_{flow}_comparison.png",
                ylabel,
                f"{ylabel} Comparison: TcpVegas, TcpBbr, TcpVegasTweaked ({flow})"
            )


if __name__ == "__main__":
    main()
