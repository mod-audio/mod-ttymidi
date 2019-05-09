#!/usr/bin/env python

import serial
import random

#random.randint(0, 255)

s = serial.Serial('/dev/pts/6', 31250, timeout=0.1)


# noteon = b'\x80\x18\x00'
# noteoff = b'\x90\x18\x64'
# polyphonickeypressure = None
# controlchange = None
# programchange = None

noteon = bytes([
    int('10000000', 2),
    int('00000001', 2),
    int('00000001', 2)
])

noteoff = bytes([
    int('10010000', 2),
    int('00000001', 2),
    int('00000001', 2)
])

poly = bytes([
    int('10100000', 2),
    int('00000001', 2),
    int('00000001', 2)
])

controlchange = bytes([
    int('10110000', 2),
    int('00000001', 2),
    int('00000001', 2)
])

programchange = bytes([
    int('11000000', 2),
    int('00000001', 2)
])

channelpressure = bytes([
    int('11010000', 2),
    int('00000001', 2)
])

pitchbend = bytes([
    int('11100000', 2),
    int('00000001', 2),
    int('00000001', 2)
])

# same as control change? Yes, special controller numbers.
channelmodemessage = bytes([
    int('10110000', 2),
    120,
    0
])

sysex_begin = bytes([
    int('11110000', 2),
    int('01111111', 2),
    int('01111111', 2)
])
sysex_end = bytes([
    int('11110111', 2)
])

timecode_quarter_frame = bytes([
    int('11110001', 2),
    int('01110001', 2)
])

song_position_pointer = bytes([
    int('11110010', 2),
    int('01110010', 2),
    int('01110010', 2)
])

song_select = bytes([
    int('11110011', 2),
    int('01110010', 2)
])

time_clock = bytes([
    int('11111000', 2)
])

time_start = bytes([
    int('11111010', 2)
])

time_continue = bytes([
    int('11111011', 2)
])

time_stop = bytes([
    int('11111100', 2)
])

active_sensing = bytes([
    int('11111110', 2)
])

reset = bytes([
    int('11111111', 2)
])


s.write(noteon)
s.write(noteoff)
s.write(poly)
s.write(controlchange)

s.write(programchange)
s.write(channelpressure)
s.write(pitchbend)
s.write(channelmodemessage)

#s.write(sysex_begin)
#s.write(sysex_end)
s.write(timecode_quarter_frame)
s.write(song_position_pointer)
s.write(song_select)
s.write(time_clock)

s.write(time_start)
s.write(time_continue)
s.write(time_stop)
s.write(active_sensing)

s.write(reset)


# #msg = [90,18,64,80,18,0]
# #s.write(bytes(msg))

# #(4).to_bytes(1 ,byteorder='big')

for k in range(1, 100000000):
    values = random.sample(list(range(0, 256)), 3)
    for v in values:
        #print(bytes(v))
        print(v.to_bytes(1, byteorder='big'))
        s.write(bytes(v))
