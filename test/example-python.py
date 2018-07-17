#!/usr/bin/env python

import subprocess
import shlex
import time
import socket, os, shutil
import serial

env = os.environ.copy()
env['JACK_DEFAULT_SERVER'] = "test"
#env['LV2_PATH'] = "/usr/lib/lv2:/tmp/lv2path"

jack = None

# Start Jack
##  -r48000 -p128 -C2 -P2 ???
jack_args = shlex.split("/usr/bin/jackd -n {0} -R -P 80 -d dummy".format(
    env['JACK_DEFAULT_SERVER']))
jack = subprocess.Popen(jack_args, env=env)
assert jack.poll() == None #?

time.sleep(1)
# Start a MIDI listener
jack_midi_dump_args = shlex.split("/usr/bin/jack_midi_dump -r")
jack_midi_dump = subprocess.Popen(jack_midi_dump_args, env=env)
assert jack_midi_dump.poll() == None

time.sleep(1)

# # Install a virtual serial device
# socat_args = "socat -d -d pty,raw,echo=0 pty,raw,echo=0"
# socat = subprocess.check_output(socat_args)
# assert socat.poll() == None


# TODO get the device names.

#ttymidi_args = shlex.split("./ttymidi -s /dev/pts/7")
#ttymidi = subprocess.Popen(ttymidi_args, env=env)

# # Open catia to take a look
# catia_args = "/usr/bin/catia"
# catia = subprocess.Popen(catia_args, env=env)
# assert catia.poll() == None


# # Connect jack clients
# jack_connect_args = "jack_connect"
# jack_connect = subprocess.Popen(jack_connect_args, env=env)
# assert jack_connect.poll() == None


# # Write to the serial device
# s = serial.Serial('/dev/pts/8', 31250, timeout=0.1)
# midi_clock_msg = b'\xF8'
# s.write(midi_clock_msg)

time.sleep(10)


if catia:
    catia.terminate()
    catia.wait()

if jack_midi_dump:
    jack_midi_dump.terminate()
    jack_midi_dump.wait()  

# Kill Jack
if jack:
    jack.terminate()
    jack.wait()

                                  

