import time


# 雪花ID生成器
class Snowflake:
    def __init__(self, epoch_time, data_center_id, worker_id, sequence_bit=12):
        self.epoch_time = epoch_time
        self.data_center_id = data_center_id
        self.worker_id = worker_id
        self.sequence_bit = sequence_bit
        self.sequence = 0
        self.last_timestamp = time.time()

    def get_timestamp(self):
        return int((time.time() - self.epoch_time) * 1000)

    def get_id(self):
        timestamp = self.get_timestamp()
        if timestamp < self.last_timestamp:
            raise ValueError("Current timestamp is less than last timestamp, possibly system clock error.")

        if timestamp == self.last_timestamp:
            self.sequence = (self.sequence + 1) & ((2 ** self.sequence_bit) - 1)
            if self.sequence == 0:
                timestamp = self.get_timestamp_until_unique()
        else:
            self.sequence = 0

        self.last_timestamp = timestamp

        id = (
                (timestamp - self.epoch_time) << (self.sequence_bit + 10)
                | (self.data_center_id << self.sequence_bit)
                | self.worker_id
                | self.sequence
        )

        return id

    def get_timestamp_until_unique(self):
        while True:
            timestamp = self.get_timestamp()
            if timestamp > self.last_timestamp:
                break

        return timestamp

def next_id():
    snowflake = Snowflake(epoch_time=1577836800, data_center_id=0, worker_id=0, sequence_bit=12)
    id = snowflake.get_id()
    return id
