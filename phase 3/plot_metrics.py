import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("test_metrics.csv")

df["ts"] = pd.to_datetime(df["ts"], unit="s")

success_df = df[df["success"] == True]

plt.figure()
plt.plot(success_df["ts"], success_df["latency_ms"], marker='o')
plt.xlabel("Time")
plt.ylabel("Latency (ms)")
plt.title("Latency vs Time")
plt.grid(True)

plt.savefig("latency_vs_time.png", dpi=300, bbox_inches="tight")
plt.close()


throughput = success_df.groupby(success_df["ts"].dt.floor("s")).size()

plt.figure()
plt.plot(throughput.index, throughput.values, marker='o')
plt.xlabel("Time")
plt.ylabel("Requests per Second")
plt.title("Throughput vs Time")
plt.grid(True)

plt.savefig("throughput_vs_time.png", dpi=300, bbox_inches="tight")
plt.close()

failed_df = df[df["success"] == False]

if not failed_df.empty:
    plt.figure()
    plt.scatter(failed_df["ts"], [1]*len(failed_df))
    plt.title("Failure Events Over Time")
    plt.xlabel("Time")
    plt.ylabel("Failure")
    plt.savefig("failures_vs_time.png", dpi=300, bbox_inches="tight")
    plt.close()
