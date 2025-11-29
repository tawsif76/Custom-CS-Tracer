import matplotlib.pyplot as plt
import pandas as pd


file_path = "custom-cs-trace.txt"

df = pd.read_csv(file_path, sep=r"\s+", header=0)

# ------------------------------
# Compute hit ratio
# ------------------------------
df["HitRatio"] = df["Hits"] / (df["Hits"] + df["Misses"])

# ------------------------------
# Average hit ratio per time
# ------------------------------
avg_df = df.groupby("Time")["HitRatio"].mean().reset_index()


plt.figure(figsize=(10, 6))

plt.plot(avg_df["Time"], avg_df["HitRatio"], marker="o")

plt.xlabel("Simulation Time (s)")
plt.ylabel("Average Cache Hit Ratio")
plt.title("NDN Average Cache Hit Ratio vs Simulation Time")
plt.grid(True, linestyle="--", alpha=0.6)
plt.tight_layout()

plt.savefig("ndn_avg_cache_hit_ratio.png", dpi=300)
plt.show()
