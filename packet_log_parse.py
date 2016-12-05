#!/usr/bin/python
import sys
import json


def parse_packets(fname):
    with open(sys.argv[1], 'r') as f:
        getKVP = lambda line: [p.split("=") for p in line.split()[-1].split(",")] + [("time", line.split()[1])]
        parseRow = lambda line: {k: float(v) if k == "time" else float(v) for k, v in getKVP(line) if k != "type"}
        packets = map(parseRow, f.readlines())
    print(len(packets))
    for packet in packets:
        packet["delivery_time"] = max([p["time"] for p in packets if p["seq"] <= packet["seq"]])
        packet["queueing_time"] = packet["delivery_time"] - packet["time"]
    return packets


def end_to_end_latency(time, packets):
    return sum([1.0 / len(packets) for packet in packets if packet["time"] - packet["arrival_time"] > time])
def reorder_buffer_needed(buffer_time, packets):
    return sum([packet["size"] for packet in packets if packet["queueing_time"] > buffer_time])
def induced_jitter(buffer_time, packets):
    return sum([1.0 / len(packets) for packet in packets if packet["queueing_time"] > buffer_time])
def data_rate(packets):
    return sum([packet["size"] for packet in packets]) / (max([packet["time"] for packet in packets]) - min([packet["time"] for packet in packets]))

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: packet_log_parse.py <packet_log>")
        exit(0)
    packets = parse_packets(sys.argv[1])
    #print(json.dumps([p for p in packets if p["queueing_time"] > 0.08], indent=2))
    times = [t / 100.0 for t in range(0, 101)]
    text = ""
    text += str("Reorder buffer needed, jitter induced, deadlines missed \n")
    text += str("\n".join([str(
        (latency, reorder_buffer_needed(latency, packets), induced_jitter(latency, packets), end_to_end_latency(latency, packets)))[1:-1]
         for latency in times]))
    text += str("\nData rate:\n")
    text += str(data_rate(packets) * 8.0 / (1024 ** 2))
    with open(sys.argv[1] + ".csv", 'w') as f:
        f.write(text)
    print(text)
