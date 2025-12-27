import pandas as pd


df = pd.read_csv("test_metrics.csv")
df = df.sort_values("ts").reset_index(drop=True)

# ===============================
#  Average Latency (steady-state)
# ===============================
steady_df = df.iloc[5:]  # تجاهل warm-up
avg_latency = steady_df["latency_ms"].mean()

# ===============================
#  Throughput
# ===============================
successful_requests = df[df["success"] == True].shape[0]
total_time = df["ts"].iloc[-1] - df["ts"].iloc[0]
throughput = successful_requests / total_time


df["replica_change"] = df["replica"] != df["replica"].shift(1)
crash_index = df[df["replica_change"]].index[0]

crash_time = df.loc[crash_index, "ts"]
recovery_time = df.loc[crash_index + 1, "ts"]
recovery_duration = recovery_time - crash_time


summary = pd.DataFrame({
    "Metric": [
        "Average Latency (ms)",
        "Throughput (requests/sec)",
        "Recovery Time (sec)"
    ],
    "Value": [
        round(avg_latency, 2),
        round(throughput, 2),
        round(recovery_duration, 2)
    ]
})


summary.to_csv("performance_summary.csv", index=False)


with open("performance_summary.txt", "w") as f:
    f.write("Performance Analysis Summary\n")
    f.write("----------------------------\n")
    f.write(f"Average Latency (steady-state): {avg_latency:.2f} ms\n")
    f.write(f"Throughput: {throughput:.2f} requests/sec\n")
    f.write(f"Recovery Time: {recovery_duration:.2f} seconds\n")

print("✅ Results saved to performance_summary.csv and performance_summary.txt")
