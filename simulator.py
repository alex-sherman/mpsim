import random

random.seed(0)
PER = 0.00

class Event(object):
    def __init__(self, time):
        self.time = time
        self.save = False
    def __repr__(self):
        return str(type(self).__name__) + ": " + str(self.time)
    def __call__(self, sim):
        pass

class AckEvent(Event):
    def __init__(self, time, packet):
        Event.__init__(self, time)
        self.packet = packet
    def __call__(self, sim):
        sim.scheduler.ack(self.packet)

class ReceptionEvent(Event):
    def __init__(self, time, packet):
        Event.__init__(self, time)
        self.save = True
        self.packet = packet
    def __repr__(self):
        return Event.__repr__(self) + "|" + str(self.packet)

class DataEvent(Event):
    SEQ = 0
    def __init__(self, packet_size = 1, bw = 280.0, time = 0.0):
        Event.__init__(self, time)
        self.packet_size = packet_size
        self.bw = bw
    def __call__(self, sim):
        sim.scheduler(Packet(DataEvent.SEQ, self.packet_size))
        DataEvent.SEQ += 1
        sim.events.append(DataEvent(self.packet_size, self.bw, self.time + self.packet_size / self.bw))


class Packet(object):
    def __init__(self, seq, size = 1):
        self.seq = seq
        self.size = size
    def __repr__(self):
        return "<{0}, {1}>".format(self.seq, self.size)

class Path(object):
    def __init__(self, delay, bw):
        self.delay = delay
        self.bw = bw
        self.ready_time = 0.0

class Simulator(object):
    def __init__(self, paths):
        self.time = 0.0
        self.paths = [Path(*p) for p in paths]
        self.scheduler = None
        self.events = []
        self.past_events = []
        self.recvd_bytes = 0

    def step(self):
        if not len(self.events):
            return False
        self.events = sorted(self.events, key=lambda e: e.time)
        event = self.events.pop(0)
        if event.save:
            self.past_events.append(event)
        self.time = event.time
        event(self)
        bw_window = [e for e in self.past_events if e.time > self.time - 10 and type(e) is ReceptionEvent]
        #print(bw_window)
        print(self.time)
        if(len(bw_window) > 1):
            bw_window = sorted(bw_window, key=lambda e: e.time)
            bw_delta = bw_window[-1].time - bw_window[0].time
            bw = sum([e.packet.size for e in bw_window]) / bw_delta
            print(bw)
        return True

    def run(self):
        while(self.step()):
            pass

    def send_packet(self, packet, path_id):
        if(random.random() < PER):
            return
        path = self.paths[path_id]
        if(path.ready_time - self.time > 2 * path.delay):
            print("dopped", packet.path_seq, path.ready_time - self.time, path.delay)
            return
        transmission_time = max(self.time, path.ready_time)
        tx_completion = transmission_time + packet.size / path.bw
        path.ready_time = tx_completion
        self.events.append(AckEvent(tx_completion + path.delay, packet))
        self.events.append(ReceptionEvent(tx_completion + path.delay / 2, packet))

class Scheduler(object):
    def __call__(self, packet):
        pass
    def ack(self, packet):
        pass

class MPTCPlowRTT(Scheduler):
    def __init__(self, sim):
        self.sim = sim
        self.ws = [2 for _ in sim.paths]
        self.seq = [0 for _ in sim.paths]
        self.outstanding = [[] for _ in sim.paths]

    def __call__(self, packet):
        packet.path = sorted([(self.ws[i] - len(self.outstanding[i]), i) for i in range(len(self.ws))])[-1][1]
        if len(self.outstanding[packet.path]) >= self.ws[packet.path]:
            return
        packet.path_seq = self.seq[packet.path]
        self.seq[packet.path] += 1
        self.outstanding[packet.path].append(packet)
        self.sim.send_packet(packet, packet.path)

    def ack(self, packet):
        print(self.ws)
        #print("ACK", packet, packet.path_seq)
        outstanding = self.outstanding[packet.path]

        #print(packet.path_seq, outstanding)
        if packet in outstanding:
            self.ws[packet.path] += 1.0 / self.ws[packet.path]
            outstanding.remove(packet)
        if outstanding and outstanding[0].path_seq < packet.path_seq - 2:
            outstanding.pop(0)
            self.ws[packet.path] = max(2, self.ws[packet.path] / 2)
        
        #print(self.ws)


if __name__ == "__main__":
    sim = Simulator([(0.05, 200.0), (0.01, 80.0)])
    sim.scheduler = MPTCPlowRTT(sim)
    sim.events.append(DataEvent())
    sim.run()