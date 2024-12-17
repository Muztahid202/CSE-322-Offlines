import pandas as pd
import matplotlib.pyplot as plt
import os

# File and output configurations
file_path = "manet-routing.output.csv"  # Replace with your CSV file name
output_dir = "graphs"  # Directory to save the graphs
os.makedirs(output_dir, exist_ok=True)

# Load the data
df = pd.read_csv(file_path)

# Metrics to plot
metrics = ["Throughput", "EndToEndDelay", "PacketDeliveryRatio", "PacketDropRatio"]

# Define row groups for filtering
rows_for_nodenumber = df.iloc[:4]  # First 4 rows
rows_for_packetspersecond = df.iloc[4:8]  # Next 4 rows
rows_for_nodespeed = df.iloc[8:12]  # Last 4 rows

# Function to plot and save a single graph
def plot_graph(x_values, y_values, x_label, y_label, title, output_filename):
    plt.figure()
    plt.plot(x_values, y_values, marker='o')
    plt.title(title)
    plt.xlabel(x_label)
    plt.ylabel(y_label)
    plt.grid(True)
    plt.savefig(output_filename)
    plt.close()

# Generate graphs for NumOfNodes
for metric in metrics:
    x_values = rows_for_nodenumber["NumOfNodes"]
    y_values = rows_for_nodenumber[metric]
    title = f"NumOfNodes vs {metric}"
    filename = os.path.join(output_dir, f"NumOfNodes_vs_{metric}.png")
    plot_graph(x_values, y_values, "NumOfNodes", metric, title, filename)

# Generate graphs for PacketsPerSec
for metric in metrics:
    x_values = rows_for_packetspersecond["PacketsPerSec"]
    y_values = rows_for_packetspersecond[metric]
    title = f"PacketsPerSec vs {metric}"
    filename = os.path.join(output_dir, f"PacketsPerSec_vs_{metric}.png")
    plot_graph(x_values, y_values, "PacketsPerSec", metric, title, filename)

# Generate graphs for NodeSpeed
for metric in metrics:
    x_values = rows_for_nodespeed["NodeSpeed"]
    y_values = rows_for_nodespeed[metric]
    title = f"NodeSpeed vs {metric}"
    filename = os.path.join(output_dir, f"NodeSpeed_vs_{metric}.png")
    plot_graph(x_values, y_values, "NodeSpeed", metric, title, filename)

print(f"12 graphs have been saved in the '{output_dir}' directory.")
