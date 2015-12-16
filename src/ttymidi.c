/*
    This file is part of ttymidi.

    ttymidi is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    ttymidi is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ttymidi.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <argp.h>
#include <jack/jack.h>
#include <jack/intclient.h>
#include <jack/midiport.h>
#include <jack/ringbuffer.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
// Linux-specific
#include <linux/serial.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <asm/ioctls.h>

#define MAX_DEV_STR_LEN               32
#define MAX_MSG_SIZE                1024

/* change this definition for the correct port */
//#define _POSIX_SOURCE 1 /* POSIX compliant source */

volatile bool run;
int serial;

/* --------------------------------------------------------------------- */
// Program options

static struct argp_option options[] =
{
	{"serialdevice" , 's', "DEV" , 0, "Serial device to use. Default = /dev/ttyUSB0", 0 },
	{"baudrate"     , 'b', "BAUD", 0, "Serial port baud rate. Default = 115200", 0 },
	{"verbose"      , 'v', 0     , 0, "For debugging: Produce verbose output", 0 },
	{"printonly"    , 'p', 0     , 0, "Super debugging: Print values read from serial -- and do nothing else", 0 },
	{"quiet"        , 'q', 0     , 0, "Don't produce any output, even when the print command is sent", 0 },
	{"name"		, 'n', "NAME", 0, "Name of the JACK client. Default = ttymidi", 0 },
	{ 0 }
};

typedef struct _arguments
{
	int  silent, verbose, printonly;
	char serialdevice[MAX_DEV_STR_LEN];
	int  baudrate;
	char name[MAX_DEV_STR_LEN];
} arguments_t;

typedef struct _jackdata
{
    jack_client_t* client;
    jack_port_t* port_in;
    jack_port_t* port_out;
    jack_ringbuffer_t* ringbuffer_in;
    jack_ringbuffer_t* ringbuffer_out;
    sem_t sem;
    bool internal;
} jackdata_t;

void exit_cli(int sig)
{
        run = false;
        printf("\rttymidi closing down ... ");

        // unused
        return; (void)sig;
}

static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
	/* Get the input argument from argp_parse, which we
	   know is a pointer to our arguments structure. */
	arguments_t *arguments = state->input;
	int baud_temp;

	switch (key)
	{
		case 'p':
			arguments->printonly = 1;
			break;
		case 'q':
			arguments->silent = 1;
			break;
		case 'v':
			arguments->verbose = 1;
			break;
		case 's':
			if (arg == NULL) break;
			strncpy(arguments->serialdevice, arg, MAX_DEV_STR_LEN);
			break;
		case 'n':
			if (arg == NULL) break;
			strncpy(arguments->name, arg, MAX_DEV_STR_LEN);
			break;
		case 'b':
			if (arg == NULL) break;
			baud_temp = strtol(arg, NULL, 0);
			if (baud_temp != EINVAL && baud_temp != ERANGE)
				switch (baud_temp)
				{
					case 1200   : arguments->baudrate = B1200  ; break;
					case 2400   : arguments->baudrate = B2400  ; break;
					case 4800   : arguments->baudrate = B4800  ; break;
					case 9600   : arguments->baudrate = B9600  ; break;
					case 19200  : arguments->baudrate = B19200 ; break;
					case 38400  : arguments->baudrate = B38400 ; break;
					case 57600  : arguments->baudrate = B57600 ; break;
					case 115200 : arguments->baudrate = B115200; break;
					default: printf("Baud rate %i is not supported.\n",baud_temp); exit(1);
				}

		case ARGP_KEY_ARG:
		case ARGP_KEY_END:
			break;

		default:
			return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

void arg_set_defaults(arguments_t *arguments)
{
	char *serialdevice_temp = "/dev/ttyUSB0";
	arguments->printonly    = 0;
	arguments->silent       = 0;
	arguments->verbose      = 0;
	arguments->baudrate     = B115200;
	char *name_tmp		= (char *)"ttymidi";
	strncpy(arguments->serialdevice, serialdevice_temp, MAX_DEV_STR_LEN);
	strncpy(arguments->name, name_tmp, MAX_DEV_STR_LEN);
}

const char *argp_program_version     = "ttymidi 0.60";
const char *argp_program_bug_address = "tvst@hotmail.com";
static char doc[]       = "ttymidi - Connect serial port devices to JACK MIDI programs!";
static struct argp argp = { options, parse_opt, NULL, doc, NULL, NULL, NULL };
arguments_t arguments;

/* --------------------------------------------------------------------- */
// JACK stuff

static int process_client(jack_nframes_t frames, void* ptr)
{
        if (! run)
            return 0;

        jackdata_t* jackdata = (jackdata_t*) ptr;

        void* portbuf_in  = jack_port_get_buffer(jackdata->port_in,  frames);
        void* portbuf_out = jack_port_get_buffer(jackdata->port_out, frames);

        // MIDI from serial to JACK
        jack_midi_clear_buffer(portbuf_in);

        char bufc[4];
        jack_midi_data_t bufj[3];
        size_t bsize;
        while (jack_ringbuffer_read(jackdata->ringbuffer_in, bufc, 3) == 3)
        {
            switch (bufc[0] & 0xF0)
            {
            case 0xC0:
            case 0xD0:
                bsize = 2;
                break;
            default:
                bsize = 3;
                break;
            }

            for (size_t i=0; i<bsize; ++i)
                bufj[i] = bufc[i];

            jack_midi_event_write(portbuf_in, 0, bufj, bsize);
        }

        // MIDI from JACK to serial
        jack_midi_event_t event;
        for (uint32_t i=0, count = jack_midi_get_event_count(portbuf_out); i<count; ++i)
        {
            if (jack_midi_event_get(&event, portbuf_out, i) != 0)
                break;
            if (event.size > 3)
                continue;

            // set first byte as size
            bufc[0] = event.size;

            // copy the rest
            size_t j=0;
            for (; j<event.size; ++j)
                bufc[j+1] = event.buffer[j];
            for (; j<3; ++j)
                bufc[j+1] = 0;

            // ready for ringbuffer
            jack_ringbuffer_write(jackdata->ringbuffer_out, bufc, 4);
        }

        // Tell MIDI-out thread we're ready
        sem_post(&jackdata->sem);

        return 0;
}

bool open_client(jackdata_t* jackdata, jack_client_t* client)
{
        jack_port_t *port_in, *port_out;
        jack_ringbuffer_t *ringbuffer_in, *ringbuffer_out;

        if (client == NULL)
        {
            client = jack_client_open(arguments.name, JackNoStartServer|JackUseExactName, NULL);

            if (client == NULL)
            {
                    fprintf(stderr, "Error opening JACK client.\n");
                    return false;
            }
        }
        else
        {
            jackdata->internal = true;
        }

        if ((port_in = jack_port_register(client, "MIDI_in", JACK_DEFAULT_MIDI_TYPE,
                                          JackPortIsOutput|JackPortIsPhysical|JackPortIsTerminal,
                                          0x0)) == NULL)
        {
                fprintf(stderr, "Error creating input port.\n");
        }

        if ((port_out = jack_port_register(client, "MIDI_out", JACK_DEFAULT_MIDI_TYPE,
                                           JackPortIsInput|JackPortIsPhysical|JackPortIsTerminal,
                                           0x0)) == NULL)
        {
                fprintf(stderr, "Error creating output port.\n");
        }

        if ((ringbuffer_in = jack_ringbuffer_create(MAX_MSG_SIZE-1)) == NULL)
        {
                fprintf(stderr, "Error creating JACK input ringbuffer.\n");
        }

        if ((ringbuffer_out = jack_ringbuffer_create(MAX_MSG_SIZE-1)) == NULL)
        {
                fprintf(stderr, "Error creating JACK output ringbuffer.\n");
        }

        if (port_in == NULL || port_out == NULL || ringbuffer_in == NULL || ringbuffer_out == NULL)
        {
                jack_client_close(client);
                return false;
        }

        jackdata->client = client;
        jackdata->port_in = port_in;
        jackdata->port_out = port_out;
        jackdata->ringbuffer_in = ringbuffer_in;
        jackdata->ringbuffer_out = ringbuffer_out;

        jack_set_process_callback(client, process_client, jackdata);

        if (jack_activate(client) != 0)
        {
                fprintf(stderr, "Error activating JACK client.\n");
                jack_client_close(client);
                return false;
        }

        sem_init(&jackdata->sem, 0, 0);

        jack_ringbuffer_mlock(ringbuffer_in);
        jack_ringbuffer_mlock(ringbuffer_out);

        return true;
}

void close_client(jackdata_t* jackdata)
{
        jack_deactivate(jackdata->client);
        jack_port_unregister(jackdata->client, jackdata->port_in);
        jack_port_unregister(jackdata->client, jackdata->port_out);
        jack_ringbuffer_free(jackdata->ringbuffer_in);
        jack_ringbuffer_free(jackdata->ringbuffer_out);

        if (! jackdata->internal)
                jack_client_close(jackdata->client);

        sem_destroy(&jackdata->sem);
        bzero(jackdata, sizeof(*jackdata));
}

/* --------------------------------------------------------------------- */
// MIDI stuff

void* write_midi_from_jack(void* ptr)
{
        jackdata_t* jackdata = (jackdata_t*) ptr;

        char bufc[4];
        size_t size;
        struct timespec timeout;

        while (run)
        {
                clock_gettime(CLOCK_REALTIME, &timeout);
                timeout.tv_nsec += 100000000; // 100 ms

                if (sem_timedwait(&jackdata->sem, &timeout) != 0)
                        continue;

                if (! run)
                        break;

                if (jack_ringbuffer_read(jackdata->ringbuffer_out, bufc, 4) == 4)
                {
                        size = (size_t)bufc[0];
                        write(serial, bufc+1, size);
                }
        }

        return NULL;
}

void* read_midi_from_serial_port(void* ptr)
{
        char buf[3], msg[MAX_MSG_SIZE];
        int msglen;

        jackdata_t* jackdata = (jackdata_t*) ptr;

	/* Lets first fast forward to first status byte... */
	if (!arguments.printonly) {
		do read(serial, buf, 1);
		while (buf[0] >> 7 == 0);
	}

	while (run)
	{
		/*
		 * super-debug mode: only print to screen whatever
		 * comes through the serial port.
		 */

		if (arguments.printonly)
		{
			read(serial, buf, 1);
			printf("%x\t", (int) buf[0]&0xFF);
			fflush(stdout);
			continue;
		}

		/*
		 * so let's align to the beginning of a midi command.
		 */

		int i = 1;

		while (i < 3) {
			read(serial, buf+i, 1);

			if (buf[i] >> 7 != 0) {
				/* Status byte received and will always be first bit!*/
				buf[0] = buf[i];
				i = 1;
			} else {
				/* Data byte received */
				if (i == 2) {
					/* It was 2nd data byte so we have a MIDI event process! */
					i = 3;
				} else {
					/* Lets figure out are we done or should we read one more byte. */
					if ((buf[0] & 0xF0) == 0xC0 || (buf[0] & 0xF0) == 0xD0) {
						i = 3;
					} else {
						i = 2;
					}
				}
			}

		}

		/* print comment message (the ones that start with 0xFF 0x00 0x00 */
		if (buf[0] == (char) 0xFF && buf[1] == (char) 0x00 && buf[2] == (char) 0x00)
		{
			read(serial, buf, 1);
			msglen = buf[0];
			if (msglen > MAX_MSG_SIZE-1) msglen = MAX_MSG_SIZE-1;

			read(serial, msg, msglen);

			if (arguments.silent) continue;

			/* make sure the string ends with a null character */
			msg[msglen] = 0;

			puts("0xFF Non-MIDI message: ");
			puts(msg);
			putchar('\n');
			fflush(stdout);
		}

		/* parse MIDI message */
		else
                {
                    jack_ringbuffer_write(jackdata->ringbuffer_in, buf, 3);
                }
	}

	return NULL;
}

/* --------------------------------------------------------------------- */
// Main program

struct termios oldtio, newtio;
jackdata_t jackdata;
pthread_t midi_out_thread;

static bool _ttymidi_init(bool exit_on_failure, jack_client_t* client)
{
        /*
         * Open JACK stuff
         */

        open_client(&jackdata, client);

        /*
         *  Open modem device for reading and not as controlling tty because we don't
         *  want to get killed if linenoise sends CTRL-C.
         */

        serial = open(arguments.serialdevice, O_RDWR | O_NOCTTY);

        if (serial < 0)
        {
                if (exit_on_failure)
                {
                        perror(arguments.serialdevice);
                        exit(-1);
                }
                return false;
        }

        /* save current serial port settings */
        tcgetattr(serial, &oldtio);

        /* clear struct for new port settings */
        bzero(&newtio, sizeof(newtio));

        /*
         * BAUDRATE : Set bps rate. You could also use cfsetispeed and cfsetospeed.
         * CRTSCTS  : output hardware flow control (only used if the cable has
         * all necessary lines. See sect. 7 of Serial-HOWTO)
         * CS8      : 8n1 (8bit, no parity, 1 stopbit)
         * CLOCAL   : local connection, no modem contol
         * CREAD    : enable receiving characters
         */
        newtio.c_cflag = arguments.baudrate | CS8 | CLOCAL | CREAD; // CRTSCTS removed

        /*
         * IGNPAR  : ignore bytes with parity errors
         * ICRNL   : map CR to NL (otherwise a CR input on the other computer
         * will not terminate input)
         * otherwise make device raw (no other input processing)
         */
        newtio.c_iflag = IGNPAR;

        /* Raw output */
        newtio.c_oflag = 0;

        /*
         * ICANON  : enable canonical input
         * disable all echo functionality, and don't send signals to calling program
         */
        newtio.c_lflag = 0; // non-canonical

        /*
         * set up: we'll be reading 4 bytes at a time.
         */
        newtio.c_cc[VTIME] = 0;     /* inter-character timer unused */
        newtio.c_cc[VMIN]  = 1;     /* blocking read until n character arrives */

        /*
         * now clean the modem line and activate the settings for the port
         */
        tcflush(serial, TCIFLUSH);
        tcsetattr(serial, TCSANOW, &newtio);

        // Linux-specific: enable low latency mode (FTDI "nagling off")
        struct serial_struct ser_info;
        ioctl(serial, TIOCGSERIAL, &ser_info);
        ser_info.flags |= ASYNC_LOW_LATENCY;
        ioctl(serial, TIOCSSERIAL, &ser_info);

        if (arguments.printonly)
        {
                printf("Super debug mode: Only printing the signal to screen. Nothing else.\n");
        }

        /*
         * read commands
         */

        /* Starting thread that is writing jack port data */
        pthread_create(&midi_out_thread, NULL, write_midi_from_jack, (void*) &jackdata);

        run = true;

        /* And also thread for polling serial data. As serial is currently read in
           blocking mode, by this we can enable ctrl+c quiting and avoid zombie
           alsa ports when killing app with ctrl+z */
        pthread_t midi_in_thread;
        pthread_create(&midi_in_thread, NULL, read_midi_from_serial_port, (void*) &jackdata);

        return true;
}

void _ttymidi_finish(void)
{
        close_client(&jackdata);
        pthread_join(midi_out_thread, NULL);

        /* restore the old port settings */
        tcsetattr(serial, TCSANOW, &oldtio);
        printf("\ndone!\n");
}

int main(int argc, char** argv)
{
        arg_set_defaults(&arguments);
        argp_parse(&argp, argc, argv, 0, 0, &arguments);

        if (! _ttymidi_init(true, NULL))
                return 1;

        signal(SIGINT, exit_cli);
        signal(SIGTERM, exit_cli);

        while (run)
                usleep(100000); // 100 ms

        _ttymidi_finish();
}

__attribute__ ((visibility("default")))
int jack_initialize(jack_client_t* client, const char* load_init);

int jack_initialize(jack_client_t* client, const char* load_init)
{
        arg_set_defaults(&arguments);

        // MOD settings
        arguments.baudrate = B38400;

        if (load_init != NULL && load_init[0] != '\0')
            strncpy(arguments.serialdevice, load_init, MAX_DEV_STR_LEN);

        if (! _ttymidi_init(false, client))
                return 1;

        return 0;
}

__attribute__ ((visibility("default")))
void jack_finish(void);

void jack_finish(void)
{
        run = false;
        _ttymidi_finish();
}
