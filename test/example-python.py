#!/usr/bin/env python

import subprocess
import shlex
import time
import socket, os, shutil
import serial


class JackDummyServer:
    def __init__(self, env):
        self.env = env
        self.jack_args = shlex.split("/usr/bin/jackd -n {0} -R -P 80 -d dummy".format(
            env['JACK_DEFAULT_SERVER']))        
        
    def __enter__(self):
        # Start Jack service        
        self.jack = subprocess.Popen(self.jack_args, env=self.env)
        assert self.jack.poll() == None
        
        print("in")
        pass

    def __exit__(self, exc_type, exc_value, traceback):
        print("out")
        if self.jack:
            self.jack.terminate()
            self.jack.wait()

class JackMidiDump:
    def __init__(self, env):
        self.env = env
        self.args = shlex.split("/usr/bin/jack_midi_dump -r")
        
    def __enter__(self):
        self.client = subprocess.Popen(self.args, env=env)
        assert self.client.poll() == None

    def __exit__(self, exc_type, exc_value, traceback):
        if self.client:
            self.client.terminate()
            self.client.wait()
            


env = os.environ.copy()

# Clients read this at jack_client_open()
env['JACK_DEFAULT_SERVER'] = "test"
            
with JackDummyServer(env) as jack:
    with JackMidiDump(env) as mdmp:
        print("hello")

        time.sleep(10)


# #env['LV2_PATH'] = "/usr/lib/lv2:/tmp/lv2path"

# 
# ##  -r48000 -p128 -C2 -P2 ???
# jack = 


# time.sleep(1)
# # Start a MIDI listener
# jack_midi_dump_args = 
# jack_midi_dump = 
# # This means, the process has not returned but is still running.
# assert jack_midi_dump.poll() == None

# time.sleep(1)

# # # Install a virtual serial device
# # socat_args = "socat -d -d pty,raw,echo=0 pty,raw,echo=0"
# # socat = subprocess.check_output(socat_args)
# # assert socat.poll() == None

# ## TODO: Unidirectional mode: -u

# # The first address is only used for reading, and the second address
# # is only used for writing (example).

# # TODO get the device names.

# #ttymidi_args = shlex.split("./ttymidi -s /dev/pts/7")
# #ttymidi = subprocess.Popen(ttymidi_args, env=env)

# # # Open catia to take a look
# # catia_args = "/usr/bin/catia"
# # catia = subprocess.Popen(catia_args, env=env)
# # assert catia.poll() == None


# # # Connect jack clients
# # jack_connect_args = "jack_connect"
# # jack_connect = subprocess.Popen(jack_connect_args, env=env)
# # assert jack_connect.poll() == None


# # # Write to the serial device
# # s = serial.Serial('/dev/pts/8', 31250, timeout=0.1)
# # midi_clock_msg = b'\xF8'
# # s.write(midi_clock_msg)

# ## TODO:Parsing on the MIDI receive side.

# time.sleep(10)


# if catia:
#     catia.terminate()
#     catia.wait()

# if jack_midi_dump:
#     jack_midi_dump.terminate()
#     jack_midi_dump.wait()  

# # Kill Jack

