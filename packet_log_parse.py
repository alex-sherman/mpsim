import sys
import json


def parse_packets(fname):
    with open(sys.argv[1], 'r') as f:
        getKVP = lambda line: [p.split("=") for p in line.split()[-1].split(",")] + [("time", line.split()[1])]
        parseRow = lambda line: {k: float(v) if k == "time" else int(v) for k, v in getKVP(line) if k != "type"}
        packets = map(parseRow, f.readlines())
    for packet in packets:
        packet["delivery_time"] = max([p["time"] for p in packets if p["seq"] <= packet["seq"]])
        packet["queueing_time"] = packet["delivery_time"] - packet["time"]
    return packets


def reorder_buffer_needed(buffer_time, packets):
    return sum([packet["size"] for packet in packets if packet["queueing_time"] > buffer_time])
def missed_deadlines(buffer_time, packets):
    return sum([1.0 / len(packets) for packet in packets if packet["queueing_time"] > buffer_time])

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: packet_log_parse.py <packet_log>")
        exit(0)
    packets = parse_packets(sys.argv[1])
    #print(json.dumps(packets, indent=2))
    times = [t / 10.0 for t in range(0, 11)]
    print("Reorder buffer needed: ")
    print("\n".join(map(str, [(latency, reorder_buffer_needed(latency, packets)) for latency in times])))
    print("Missed deadlines: ")
    print("\n".join(map(str, [(latency, missed_deadlines(latency, packets)) for latency in times])))