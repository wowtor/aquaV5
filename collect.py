#!/usr/bin/env python3

import argparse
import csv
import dataclasses
from dataclasses import dataclass
import datetime
from functools import partial
import json
import logging
import os
import sys
import time

import paho.mqtt.client as mqtt


LOG = logging.getLogger(__name__)


def parse_temperature_from_bytes(v: list[int]) -> float:
    if len(v) != 2:
        raise ValueError(f'expected two bytes; found: {v}')

    return int.from_bytes(bytearray(v), byteorder='big',signed=True)/100


@dataclass(frozen=True)
class EncodedMessage:
    raw: str
    timestamp: str
    message_type: str
    length: int
    payload: str

    @property
    def payload_bytes(self):
        return [int(self.payload[i*2:i*2+2], 16) for i in range(len(self.payload)//2)]


@dataclass(frozen=True)
class GenericMessage:
    encoded: EncodedMessage

    def __str__(self) -> str:
        fields = [f'{field.name}={getattr(self, field.name)}' for field in dataclasses.fields(self) if field.name != 'encoded']
        return f'{self.__class__.__name__}({"; ".join(fields)})'



@dataclass(frozen=True)
class TemperatureMessage(GenericMessage):
    water_temperature: float
    compressor_outlet_temperature: float
    air_inlet_temperature: float
    evaporator1_temperature: float
    evaporator2_temperature: float
    evaporator3_temperature: float

    def update(self, entities: dict):
        entity_names = [field.name for field in dataclasses.fields(self) if field.type == float]
        for field in entity_names:
            entities[field].value = getattr(self, field)

    @staticmethod
    def parse(message: EncodedMessage) -> 'TemperatureMessage':
        payload = [parse_temperature_from_bytes(message.payload_bytes[i*2:i*2+2]) for i in range(len(message.payload_bytes)//2)]
        return TemperatureMessage(message, *payload)


@dataclass(frozen=True)
class InputMessage(GenericMessage):
    """
    Observed behavior:
    - this message is sent periodically
    - the first value corresponds to input I2
    - the second value corresponds to input I1
    - the third value corresponds to heating status
    """
    input_i2: bool
    input_i1: bool
    aux_heating: bool

    def update(self, entities: dict):
        entities['input_i2'].value = self.input_i2
        entities['input_i1'].value = self.input_i1
        entities['aux_heating'].value = self.aux_heating

    @staticmethod
    def parse(message: EncodedMessage) -> 'InputMessage':
        for v in message.payload_bytes:
            if v not in [0, 1]:
                raise ValueError('illegal input')
        payload = [bool(v) for v in message.payload_bytes]
        return InputMessage(message, *payload)


@dataclass(frozen=True)
class TemperatureExtremesMessage(GenericMessage):
    """
    Observed behavior:
    - byte 0 is alway 00
    - byte 1-2 is the smallest value
    - byte 3-4 is the greatest value
    """
    field: str
    min: float
    max: float

    def update(self, entities: dict):
        entities[f'{self.field}_min'].value = self.min
        entities[f'{self.field}_max'].value = self.max

    @staticmethod
    def parse(field: str, message: EncodedMessage) -> 'TemperatureExtremesMessage':
        if message.length != 5 or message.payload_bytes[0] != 0:
            raise ValueError('illegal input')

        temp_min, temp_max = [parse_temperature_from_bytes(message.payload_bytes[i:i+2]) for i in [1, 3]]
        return TemperatureExtremesMessage(message, field, temp_min, temp_max)


@dataclass(frozen=True)
class DummyMessage(GenericMessage):
    def update(self, entities: dict):
        pass


@dataclass(frozen=True)
class HeatingMessage(DummyMessage):
    """
    Observed behavior:
    - this message is sent repeatedly (every second or so)
    - when active, payload value is 00
    - when inactive, payload value is 64 
    """
    state: bool

    @staticmethod
    def parse(message: EncodedMessage) -> 'HeatingMessage':
        if message.length != 1:
            raise ValueError('illegal input')
        if message.payload not in ['00', '64']:
            raise ValueError('illegal input')
        return HeatingMessage(message, message.payload == '00')


@dataclass(frozen=True)
class CounterMessage(DummyMessage):
    """
    Observed behavior:
    This message is a counter. It counts the number of seconds passed since a state change, and the number of cycles.

    The counter messages have three 32bit numbers, and it can be in either of two states, `state0` or `state1`.

    - If in `state0`, the first number is the number of seconds in this state, and the second number is 0.
    - If in `state1`, the second number is the number of seconds in this state, and the first number is 0.
    - On transition from `state1` to `state0`, the third number is incremented by 1.
    """
    name: str
    state: int
    value: int
    cycle_count: int

    @staticmethod
    def parse(name: str, message: EncodedMessage) -> 'HeatingMessage':
        if message.length != 12:
            raise ValueError(f'expected message length 12; found: {message.length}')
        values = [int(message.payload[i*8:i*8+8], 16) for i in range(3)]
        if values[0] != 0 and values[1] == 0:
            state = 0
        elif values[1] != 0 and values[0] == 0:
            state = 1
        else:
            raise ValueError('illegal input')
        return CounterMessage(message, name, state, values[state], values[2])
        


MESSAGE_TYPES = {
    #'016421B601':
    #'0164FDED01':
    '0164FEB006': TemperatureMessage.parse,
    '0164FEBA03': partial(TemperatureExtremesMessage.parse, 'water_temperature'),
    '0164FEBD03': partial(TemperatureExtremesMessage.parse, 'compressor_outlet_temperature'),
    '0164FEC003': partial(TemperatureExtremesMessage.parse, 'air_inlet_temperature'),
    '0164FEC303': partial(TemperatureExtremesMessage.parse, 'evaporator1_temperature'),
    '0164FEC603': partial(TemperatureExtremesMessage.parse, 'evaporator2_temperature'),
    '0164FEC903': partial(TemperatureExtremesMessage.parse, 'evaporator3_temperature'),
    '0164FEE203': partial(CounterMessage.parse, 'counter1'),
    '0164FEE503': partial(CounterMessage.parse, 'counter2'),
    '0164FEE803': partial(CounterMessage.parse, 'counter3'),
    '0164FEEB03': partial(CounterMessage.parse, 'counter4'),
    '0164FEEE03': partial(CounterMessage.parse, 'counter5'),
    '0164FEF103': partial(CounterMessage.parse, 'counter6'),
    '0164FF1403': InputMessage.parse,

    #'0165FEF701':
    '0165FEF901': HeatingMessage.parse,
    #'0165FEFB01':
    #'0165FEFD01':
    #'0165FEFF01':
    #'0165FF0101':
    #'0165FF0301':
    #'016516B301':
}


def parse_message(timestamp: str, data: str) -> tuple[EncodedMessage, GenericMessage | None]:
    message_type = data[:10]
    if len(data) > 14:
        length = int(data[10:12], 16)
        payload = data[12:-4]
    else:
        length = 0
        payload = ""

    m = EncodedMessage(data, timestamp, message_type, length, payload)

    if len(m.payload_bytes) > 0 and m.message_type in MESSAGE_TYPES:
        try:
            return m, MESSAGE_TYPES[m.message_type](m)
        except Exception as e:
            LOG.warning(f'parsing message failed: {m}; the error was: {e}')
            raise
            return m, None
    else:
        return m, None


@dataclass
class Entity:
    id: str
    name: str
    category: str | None = 'diagnostic'

    def definition(self, device_id: str):
        d = {
            'name': self.name,
            'value_template': '{{value_json.%s}}' % self.id,
            'unique_id': f'{device_id}_{self.id}',
        }
        if self.category:
            d |= {'entity_category': self.category}
        return d


@dataclass
class BinarySensor(Entity):
    value: bool | None = None

    def definition(self, device_id: str):
        return super().definition(device_id) | {
            'p': 'binary_sensor',
        }

    @property
    def state(self) -> str | None:
        return ['OFF', 'ON'][self.value] if self.value is not None else None


@dataclass
class Sensor(Entity):
    value: float | None = None

    def definition(self, device_id: str):
        return super().definition(device_id) | {
            'p': 'sensor',
            'device_class': 'temperature',
            'unit_of_measurement': '°C',
        }

    @property
    def state(self) -> str | None:
        return str(self.value) if self.value is not None else None


class MovingAverageSensor(Entity):
    def __init__(self, n, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.n = n
        self.values = []

    @property
    def value(self) -> float:
        return sum(self.values) / len(self.values) if self.values else None

    @value.setter
    def value(self, value: float):
        self.values = [value] + self.values[:self.n-1]

    def definition(self, device_id: str):
        return super().definition(device_id) | {
            'p': 'sensor',
            'device_class': 'temperature',
            'unit_of_measurement': '°C',
        }

    @property
    def state(self) -> str | None:
        return str(self.value) if self.value is not None else None


class TemperatureDevice:
    def __init__(self, device_id: str = 'boiler_aquamqtt'):
        self.device_id = device_id
        entity_list = [
            BinarySensor('input_i2', 'Input I2'),
            BinarySensor('input_i1', 'Input I1'),
            BinarySensor('aux_heating', 'Auxiliary heating'),
            MovingAverageSensor(20, 'water_temperature', 'Water temperature', category=None),
            MovingAverageSensor(20, 'air_inlet_temperature', 'Air inlet'),
            MovingAverageSensor(20, 'compressor_outlet_temperature', 'Compressor outlet'),
            MovingAverageSensor(10, 'evaporator1_temperature', 'Evaporator 1'),
            MovingAverageSensor(10, 'evaporator2_temperature', 'Evaporator 2'),
            MovingAverageSensor(10, 'evaporator3_temperature', 'Evaporator 3'),
            Sensor('water_temperature_min', 'Water temperature (min)'),
            Sensor('water_temperature_max', 'Water temperature (max)'),
            Sensor('compressor_outlet_temperature_min', 'Compressor outlet (min)'),
            Sensor('compressor_outlet_temperature_max', 'Compressor outlet (max)'),
            Sensor('air_inlet_temperature_min', 'Air inlet (min)'),
            Sensor('air_inlet_temperature_max', 'Air inlet (max)'),
            Sensor('evaporator1_temperature_min', 'Evaporator 1 (min)'),
            Sensor('evaporator1_temperature_max', 'Evaporator 1 (max)'),
            Sensor('evaporator2_temperature_min', 'Evaporator 2 (min)'),
            Sensor('evaporator2_temperature_max', 'Evaporator 2 (max)'),
            Sensor('evaporator3_temperature_min', 'Evaporator 3 (min)'),
            Sensor('evaporator3_temperature_max', 'Evaporator 3 (max)'),
        ]
        self.entities = {e.id: e for e in entity_list}
        self.publish_rate_secs = 30
        self.last_published = time.time()

    @property
    def state_topic(self):
        return f'{self.device_id}/state'

    def announce_device(self, client):
        payload = {
            'device': {
                'ids': self.device_id,
                'name': 'Heat pump boiler',
                'mf': 'AquaMQTT',
            },
            'origin': {
                'name': 'boiler_aquamqtt',
            },
            'cmps': {
                entity.id: entity.definition(self.device_id) for entity in self.entities.values()
            },
            'state_topic': self.state_topic,
            'qos': 2,
        }
        client.publish(f'homeassistant/device/{self.device_id}/config', payload=json.dumps(payload), retain=True)

    def update_state(self, client, msg: GenericMessage):
        msg.update(self.entities)

        now = time.time()
        if now > self.last_published + self.publish_rate_secs:
            self.last_published = now
            payload = { entity.id: entity.state for entity in self.entities.values() }
            client.publish(self.state_topic, payload=json.dumps(payload), retain=True)


def payload_equals(payload1, payload2) -> bool:
    if payload1 == payload2:
        return True

    #if 'cv5/100/254/226/3' in topic or 'cv5/100/254/229/3' in topic or 'cv5/100/254/232/3' in topic or 'cv5/100/254/235/3' in topic or 'cv5/100/254/238/3' in topic or 'cv5/100/254/241/3' in topic or 'cv5/100/254/242/3' in topic:
    #    if payload1[:26] + payload1[28:] == payload2[:26] + payload2[28:]:
    #        return True

    return False


class App:
    def __init__(self, host, port, topic, csv_path: str, annotations: dict[str, str], dump_all: bool, publish: bool):
        self.host = host
        self.port = port
        self.topic = topic
        self.last_payloads = {}
        self.temperature_device = TemperatureDevice() if publish else None
        self._csv_file = open(csv_path, 'a')
        self.annotations = annotations
        self.dump_all = dump_all
        self.publish = publish

    def run(self):
        last_payloads = {}
        client = mqtt.Client()

        #client.username_pw_set(username='your-broker-user', password='your-broker-pass')
        client.connect(self.host, self.port)
        client.subscribe(self.topic)
        #client.subscribe('your-prefix/cv5/+/+/+/+/debug')

        if self.publish:
            self.temperature_device.announce_device(client)

        client.on_message = self.on_message

        client.loop_start()

    def dump_message(self, msg, parsed_message: GenericMessage | None):
        row = [msg.timestamp, msg.message_type, msg.length, msg.payload]

        if len(self.annotations) > 0:
            annotation = self.annotations.get(msg.message_type, 'none')
            row = [annotation] + row

        if parsed_message is not None:
            row = row + [str(parsed_message)]

        writer = csv.writer(self._csv_file)
        writer.writerow(row)
        self._csv_file.flush()

    def on_message(self, client, userdata, message):
        timestamp = datetime.datetime.now(tz=datetime.timezone.utc).astimezone().isoformat()
        payload = message.payload.decode('utf-8')

        encoded, msg = parse_message(timestamp, payload)

        if not self.dump_all and encoded.message_type in self.last_payloads and payload_equals(self.last_payloads[encoded.message_type], payload):
            return
        self.last_payloads[encoded.message_type] = payload

        if msg is not None and self.publish:
            self.temperature_device.update_state(client, msg)
        self.dump_message(encoded, msg)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', help='hostname of MQTT broker', required=True)
    parser.add_argument('--port', help='port of MQTT broker', default=1883, type=int)
    parser.add_argument('--subscribe', help='MQTT topic to subscribe to', default='v5alt/debug/+/+')
    parser.add_argument('--output', help='where messages are stored', default='messages.csv')
    parser.add_argument('--dump-all', help='also dump repeated messages with the same payload', action='store_true')
    parser.add_argument('--annotate', metavar='MESSAGE_TYPE=KEY', help='annotate specific topics', action='extend', nargs='+')
    parser.add_argument('--publish', help='publish entities to home assistant', action='store_true');
    args = parser.parse_args()

    logging.basicConfig(stream=sys.stderr, level=logging.INFO, format='%(asctime)s [%(levelname)s] %(message)s')

    annotations = dict(a.split('=') for a in args.annotate) if args.annotate else []

    app = App(args.host, args.port, args.subscribe, args.output,
        annotations=annotations,
        dump_all=args.dump_all,
        publish=args.publish,
    )
    LOG.info('launching')
    app.run()

    while True:
        time.sleep(1)

main()
