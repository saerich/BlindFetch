import csv
import statistics
import matplotlib.pyplot as plt


def get_stats(filename, column, divisor=1):
    values = []

    with open(filename, newline="") as csvfile:
        reader = csv.DictReader(csvfile)

        for row in reader:
            try:
                value = float(row[column]) / divisor
                values.append(value)
            except (ValueError, KeyError):
                pass

    mean = statistics.mean(values)
    stddev = statistics.stdev(values)
    variance = statistics.variance(values)

    return mean, stddev, variance


# esp_https_ota
plain_mean, plain_std, plain_var = get_stats(
    "Data/PlainTiming.csv",
    "Delta",
    divisor=1_000_000
)

# Blind Fetch
blind_mean, blind_std, blind_var = get_stats(
    "Data/Logs-FixedBudget-Randomized/Client.csv",
    "Duration(s)"
)


print(f"esp_https_ota: {plain_mean:.2f} ± {plain_std:.2f} s "
    f"(variance={plain_var:.2f} s²)")

print(f"Blind Fetch:   {blind_mean:.2f} ± {blind_std:.2f} s "
      f"(variance={blind_var:.2f} s²)")

labels = ["esp_https_ota", "Blind Fetch"]
means = [plain_mean, blind_mean]
stddevs = [plain_std, blind_std]

plt.bar(labels, means, yerr=stddevs, capsize=5)
plt.ylabel("Execution Time (seconds)")
plt.title("Execution Time of esp_https_ota vs Blind Fetch")
plt.tight_layout()

plt.show()