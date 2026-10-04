//#define ConstructMergedExecutions
//#define TLSComparison
// //BBT01|LB01|PSC01|SLM01
#if ConstructMergedExecutions

string[][] clientFirmware = [
    [.. File.ReadAllLines("../Data/Logs-EarlyClose-Ordered/Client.csv").Skip(1).Select(x => x.Split(",").Last())], 
    [.. File.ReadAllLines("../Data/Logs-EarlyClose-Randomized/Client.csv").Skip(1).Select(x => x.Split(",").Last())], 
    [.. File.ReadAllLines("../Data/Logs-FixedBudget-Ordered/Client.csv").Skip(1).Select(x => x.Split(",").Last())], 
    [.. File.ReadAllLines("../Data/Logs-FixedBudget-Randomized/Client.csv").Skip(1).Select(x => x.Split(",").Last())]
];

string[] orderedCloseEarlyExec = [.. File.ReadAllLines("../Data/Logs-EarlyClose-Ordered/Executions.csv").Skip(1).Select((x, idx) => $"{clientFirmware[0].ElementAt(idx)},{x}")];
string[] unorderedCloseEarlyExec = [.. File.ReadAllLines("../Data/Logs-EarlyClose-Randomized/Executions.csv").Skip(1).Select((x, idx) => $"{clientFirmware[1].ElementAt(idx)},{x}")];
string[] orderedFixedExec = [.. File.ReadAllLines("../Data/Logs-FixedBudget-Ordered/Executions.csv").Skip(1).Select((x, idx) => $"{clientFirmware[2].ElementAt(idx)},{x}")];
string[] unorderedFixedExec = [.. File.ReadAllLines("../Data/Logs-FixedBudget-Randomized/Executions.csv").Skip(1).Select((x, idx) => $"{clientFirmware[3].ElementAt(idx)},{x}")];

string outFile = "../Data/MergedExecutions.csv";

if(File.Exists(outFile)) { File.Delete(outFile); }
File.WriteAllText(outFile, "Mode,Ordering,Slots,TTFB Miliseconds,TTLB Miliseconds,Blocks,Id,ActualSlot,Slot0,Slot1,Slot2,Slot3\n");

if(orderedCloseEarlyExec.Length == unorderedCloseEarlyExec.Length && orderedFixedExec.Length == unorderedFixedExec.Length && orderedFixedExec.Length == unorderedCloseEarlyExec.Length)
{
    int counter = 0;
    for(int i = 0; i < orderedFixedExec.Length; i++)
    {
        string[] orderedCloseEarlyRow = orderedCloseEarlyExec[i].Split(",");
        int actualPositionOCE = orderedCloseEarlyRow.ElementAt(5).Replace("[", "").Replace("]", "").Split("|").IndexOf(orderedCloseEarlyRow.ElementAt(0));
        string[] unorderedCloseEarlyRow = unorderedCloseEarlyExec[i].Split(",");
        int actualPositionUCE = unorderedCloseEarlyRow.ElementAt(5).Replace("[", "").Replace("]", "").Split("|").IndexOf(unorderedCloseEarlyRow.ElementAt(0));
        string[] orderedFixedBudgetRow = orderedFixedExec[i].Split(",");
        int actualPositionOFB = orderedFixedBudgetRow.ElementAt(5).Replace("[", "").Replace("]", "").Split("|").IndexOf(orderedFixedBudgetRow.ElementAt(0));
        string[] unorderedFixedBudgetRow = unorderedFixedExec[i].Split(",");
        int actualPositionUFB = unorderedFixedBudgetRow.ElementAt(5).Replace("[", "").Replace("]", "").Split("|").IndexOf(unorderedFixedBudgetRow.ElementAt(0));

        string[] orderedEarlyCloseSlots = orderedCloseEarlyRow.ElementAt(5).Replace("[", "").Replace("]", "").Split("|");
        string[] unorderedEarlyCloseSlots = unorderedCloseEarlyRow.ElementAt(5).Replace("[", "").Replace("]", "").Split("|");
        string[] orderedFixedBudgetSlots = orderedFixedBudgetRow.ElementAt(5).Replace("[", "").Replace("]", "").Split("|");
        string[] unorderedFixedBudgetSlots = unorderedFixedBudgetRow.ElementAt(5).Replace("[", "").Replace("]", "").Split("|");

        string[] file = 
        [
            $"EarlyClose,Ordered,4,{TimeSpan.Parse(orderedCloseEarlyRow[2]).TotalMilliseconds},{TimeSpan.Parse(orderedCloseEarlyRow[7]).TotalMilliseconds},{orderedCloseEarlyRow[4]},{counter++},{actualPositionOCE},{orderedEarlyCloseSlots[0]},{orderedEarlyCloseSlots[1]},{orderedEarlyCloseSlots[2]},{orderedEarlyCloseSlots[3]}",
            $"EarlyClose,Unordered,4,{TimeSpan.Parse(unorderedCloseEarlyRow[2]).TotalMilliseconds},{TimeSpan.Parse(unorderedCloseEarlyRow[7]).TotalMilliseconds},{unorderedCloseEarlyRow[4]},{counter++},{actualPositionUCE},{unorderedEarlyCloseSlots[0]},{unorderedEarlyCloseSlots[1]},{unorderedEarlyCloseSlots[2]},{unorderedEarlyCloseSlots[3]}",
            $"FixedBudget,Ordered,4,{TimeSpan.Parse(orderedFixedBudgetRow[2]).TotalMilliseconds},{TimeSpan.Parse(orderedFixedBudgetRow[3]).TotalMilliseconds},{orderedFixedBudgetRow[4]},{counter++},{actualPositionOFB},{orderedFixedBudgetSlots[0]},{orderedFixedBudgetSlots[1]},{orderedFixedBudgetSlots[2]},{orderedFixedBudgetSlots[3]}",
            $"FixedBudget,Unordered,4,{TimeSpan.Parse(unorderedFixedBudgetRow[2]).TotalMilliseconds},{TimeSpan.Parse(unorderedFixedBudgetRow[3]).TotalMilliseconds},{unorderedFixedBudgetRow[4]},{counter++},{actualPositionUFB},{unorderedFixedBudgetSlots[0]},{unorderedFixedBudgetSlots[1]},{unorderedFixedBudgetSlots[2]},{unorderedFixedBudgetSlots[3]}"
        ];

        File.AppendAllLines(outFile, file);
    }

}
#endif

#if TLSComparison
if(File.Exists("../Data/TLSExecutions.csv")) { File.Delete("../Data/TLSExecutions.csv"); }
if(File.Exists("../Data/NonTLSExecutions.csv")) { File.Delete("../Data/NonTLSExecutions.csv"); }

string[] TLS = File.ReadAllLines("../Data/100TLSExecutions.csv").Skip(1).ToArray();
string[] NonTLS = File.ReadAllLines("../Data/100NonTLSExecutions.csv").Skip(1).ToArray();

foreach(string l in TLS)
{
    string[] lEntry = l.Split(",");
    lEntry[1] = TimeSpan.Parse(lEntry[1]).TotalMilliseconds.ToString();
    lEntry[2] = TimeSpan.Parse(lEntry[2]).TotalMilliseconds.ToString();
    File.AppendAllLines("../Data/TLSExecutions.csv", [string.Join(",", lEntry)]);

}

foreach(string l in NonTLS)
{
    string[] lEntry = l.Split(",");
    lEntry[1] = TimeSpan.Parse(lEntry[1]).TotalMilliseconds.ToString();
    lEntry[2] = TimeSpan.Parse(lEntry[2]).TotalMilliseconds.ToString();
    File.AppendAllLines("../Data/NonTLSExecutions.csv", [string.Join(",", lEntry)]);

}
#endif

#if SlotHeatmap
string[] Unordered = File.ReadAllLines("../Data/UnorderedExecutionsFixedBudgetBBT01.csv").Skip(1)
    .Union(File.ReadAllLines("../Data/UnorderedExecutionsFixedBudgetLB01.csv"))
    .Union(File.ReadAllLines("../Data/UnorderedExecutionsFixedBudgetPSC01.csv"))
    .Union(File.ReadAllLines("../Data/UnorderedExecutionsFixedBudgetSLM01.csv"))
    .ToArray();
var firmwareOrder = new[] { "BBT01", "LB01", "PSC01", "SLM01" };
var count = new Dictionary<(int Position, string Firmware), int>();

if(File.Exists("../Data/UnorderedSlotHeatmap.csv")) { File.Delete("../Data/UnorderedSlotHeatmap.csv"); }

foreach(var line in Unordered)
{
    string[] l = line.Split(",");
    string[] order = l[4].Replace("[", "").Replace("]", "").Split("|");

    for(int i = 0; i < order.Length; i++)
    {
        var kvp = (i, order[i]);
        if(!count.ContainsKey(kvp)) { count[kvp] = 0; }
        count[kvp]++;
    }
}

File.WriteAllText("../Data/UnorderedSlotHeatmap.csv", "Position,Firmware 1, Firmware 2, Firmware 3, Firmware 4\n");

for(int i = 0; i < 4; i++)
{
    double totalForPosition = count.Where(x => x.Key.Position == i).Sum(x => x.Value);
    List<double> normalized = [];
    foreach(var fw in firmwareOrder)
    {
        count.TryGetValue((i, fw), out int val);
        normalized.Add(totalForPosition == 0 ? 0 : (double)val / totalForPosition);
    }
    File.AppendAllText("../Data/UnorderedSlotHeatmap.csv", $"{i},{normalized[0]},{normalized[1]},{normalized[2]},{normalized[3]}\n");
}
#endif

// string[] lines = File.ReadAllLines("PlainExecutions.csv");
// string[] add = File.ReadAllLines("OTA.log");

// for(int i = 0; i < lines.Length; i++)
// {
//     lines[i] = $"{lines[i]}{add[i]}";
// }

// File.WriteAllLines("FixedPlainExecutions.csv", lines);