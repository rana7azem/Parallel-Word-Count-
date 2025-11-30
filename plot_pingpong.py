import matplotlib.pyplot as plt

sizes = []
latencies = []
bandwidths = []

with open("pingpong_results.csv", "r") as f:
    for line in f:
        parts = line.strip().split(",")
        if len(parts) != 3:
            continue
        size = int(parts[0])
        lat = float(parts[1])
        bw = float(parts[2])
        sizes.append(size)
        latencies.append(lat)
        bandwidths.append(bw)

# Latency plot
plt.figure()
plt.loglog(sizes, latencies, marker="o")
plt.xlabel("Message size (bytes)")
plt.ylabel("Latency (sec)")
plt.title("MPI Ping-Pong Latency")
plt.grid(True, which="both")
plt.savefig("latency_plot.png", dpi=200)

# Bandwidth plot
plt.figure()
plt.loglog(sizes, bandwidths, marker="o")
plt.xlabel("Message size (bytes)")
plt.ylabel("Bandwidth (bytes/sec)")
plt.title("MPI Ping-Pong Bandwidth")
plt.grid(True, which="both")
plt.savefig("bandwidth_plot.png", dpi=200)
