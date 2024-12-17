import pandas as pd
import matplotlib.pyplot as plt
import os

# File and output configurations
file_aodv = "manet-routing.output.csv"  # AODV data
file_raodv = "result.csv"  # RAODV data
output_dir = "graphs/task3"  # Directory to save the graphs
os.makedirs(output_dir, exist_ok=True)

# Load the data
df_aodv = pd.read_csv(file_aodv)
df_raodv = pd.read_csv(file_raodv)

# Metrics to plot
metrics = ["Throughput", "EndToEndDelay", "PacketDeliveryRatio", "PacketDropRatio"]

# Define row groups for filtering
rows_for_nodenumber_aodv = df_aodv.iloc[:4]  # First 4 rows for AODV
rows_for_packetspersecond_aodv = df_aodv.iloc[4:8]  # Next 4 rows for AODV
rows_for_nodespeed_aodv = df_aodv.iloc[8:12]  # Last 4 rows for AODV

rows_for_nodenumber_raodv = df_raodv.iloc[:4]  # First 4 rows for RAODV
rows_for_packetspersecond_raodv = df_raodv.iloc[4:8]  # Next 4 rows for RAODV
rows_for_nodespeed_raodv = df_raodv.iloc[8:12]  # Last 4 rows for RAODV

# Function to plot and save a single graph
def plot_graph(x_values_aodv, y_values_aodv, x_values_raodv, y_values_raodv, x_label, y_label, title, output_filename):
    plt.figure()
    plt.plot(x_values_aodv, y_values_aodv, marker='o', label='AODV', color='blue')
    plt.plot(x_values_raodv, y_values_raodv, marker='x', label='RAODV', color='red')
    plt.title(title)
    plt.xlabel(x_label)
    plt.ylabel(y_label)
    plt.grid(True)
    plt.legend()
    plt.savefig(output_filename)
    plt.close()

# Generate graphs for NumOfNodes
for metric in metrics:
    x_values_aodv = rows_for_nodenumber_aodv["NumOfNodes"]
    y_values_aodv = rows_for_nodenumber_aodv[metric]
    x_values_raodv = rows_for_nodenumber_raodv["NumOfNodes"]
    y_values_raodv = rows_for_nodenumber_raodv[metric]
    title = f"NumOfNodes vs {metric}"
    filename = os.path.join(output_dir, f"NumOfNodes_vs_{metric}.png")
    plot_graph(x_values_aodv, y_values_aodv, x_values_raodv, y_values_raodv, "NumOfNodes", metric, title, filename)

# Generate graphs for PacketsPerSec
for metric in metrics:
    x_values_aodv = rows_for_packetspersecond_aodv["PacketsPerSec"]
    y_values_aodv = rows_for_packetspersecond_aodv[metric]
    x_values_raodv = rows_for_packetspersecond_raodv["PacketsPerSec"]
    y_values_raodv = rows_for_packetspersecond_raodv[metric]
    title = f"PacketsPerSec vs {metric}"
    filename = os.path.join(output_dir, f"PacketsPerSec_vs_{metric}.png")
    plot_graph(x_values_aodv, y_values_aodv, x_values_raodv, y_values_raodv, "PacketsPerSec", metric, title, filename)

# Generate graphs for NodeSpeed
for metric in metrics:
    x_values_aodv = rows_for_nodespeed_aodv["NodeSpeed"]
    y_values_aodv = rows_for_nodespeed_aodv[metric]
    x_values_raodv = rows_for_nodespeed_raodv["NodeSpeed"]
    y_values_raodv = rows_for_nodespeed_raodv[metric]
    title = f"NodeSpeed vs {metric}"
    filename = os.path.join(output_dir, f"NodeSpeed_vs_{metric}.png")
    plot_graph(x_values_aodv, y_values_aodv, x_values_raodv, y_values_raodv, "NodeSpeed", metric, title, filename)

print(f"12 graphs have been saved in the '{output_dir}' directory.")
