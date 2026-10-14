namespace ObliviousOTA;
public sealed class RequestMetrics
{
    public long RequestId { get; set; }
    public string Operation { get; set; } = "";
    public string Status { get; set; } = "Completed";

    public int Concurrent { get; set; }

    public long StartTimestamp { get; set; }
    public TimeSpan CpuBefore { get; set; }

    public double DurationMs { get; set; }
    public double OpaqueMs { get; set; }
    public double OprfMs { get; set; }
    public double EncryptionMs { get; set; }
    public double BundleMs { get; set; }

    public long GeneratedBytes { get; set; }
    public long SentBytes { get; set; }

    public double CPUPercent { get; set; }
    public long MemoryBefore { get; set; }
    public long MemoryAfter { get; set; }
    public long PeakWorkingSet { get; set; }
}