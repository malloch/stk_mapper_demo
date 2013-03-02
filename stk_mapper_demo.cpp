#include <RtWvOut.h>
#include <cstdlib>
#include <cmath>
#include <BeeThree.h>
#include <stdio.h>
#include <math.h>

#include <unistd.h>
#include <arpa/inet.h>

extern "C" {
#include <mapper/mapper.h>
}

#define NUMVOICES 16

using namespace stk;

BeeThree *voices[NUMVOICES];
int active[NUMVOICES];
mapper_device dev;
int done = 0;

void print_active_instances()
{
    int i;

    // clear screen & cursor to home
    printf("\e[2J\e[0;0H");

    printf("INSTANCES:");
    for (i=0; i<NUMVOICES; i++)
        printf("%4i", i);
    printf("\nSTATUS:   ");
    for (i=0; i<NUMVOICES; i++) {
        printf("   %c", active[i] ? 'X' : ' ');
    }
    printf("\n");
}

void freq_handler(mapper_signal sig, mapper_db_signal props,
                  int instance_id, void *value, int count,
                  mapper_timetag_t *tt)
{
	if (value) {
        StkFloat f = *((float *)value);
        // need to check if voice is already active
        if (active[instance_id] == 0) {
            voices[instance_id]->noteOn(f, 1);
            active[instance_id] = 1;
            print_active_instances();
        }
        else {
            // can we perform polyphonic pitch-bend? use multiple channels?
        }
    }
    else {
		voices[instance_id]->noteOff(1);
        msig_release_instance(sig, instance_id, *tt);
        active[instance_id] = 0;
        print_active_instances();
    }
}

void feedback_gain_handler(mapper_signal sig, mapper_db_signal props,
                           int instance_id, void *value, int count,
                           mapper_timetag_t *tt)
{
    if (value) {
        StkFloat f = *((float *)value);
        voices[instance_id]->controlChange(2, f);
    }
    else {
        // release note also?
        msig_release_instance(sig, instance_id, *tt);
    }
}

void gain_handler(mapper_signal sig, mapper_db_signal props,
                  int instance_id, void *value, int count,
                  mapper_timetag_t *tt)
{
    if (value) {
        StkFloat f = *((float *)value);
        voices[instance_id]->controlChange(4, f);
    }
    else {
        // release note also?
        msig_release_instance(sig, instance_id, *tt);
    }
}

void LFO_speed_handler(mapper_signal sig, mapper_db_signal props,
                       int instance_id, void *value, int count,
                       mapper_timetag_t *tt)
{
    if (value) {
        StkFloat f = *((float *)value);
        voices[instance_id]->controlChange(11, f);
    }
    else {
        // release note also?
        msig_release_instance(sig, instance_id, *tt);
    }
}

void LFO_depth_handler(mapper_signal sig, mapper_db_signal props,
                       int instance_id, void *value, int count,
                       mapper_timetag_t *tt)
{
    if (value) {
        StkFloat f = *((float *)value);
        voices[instance_id]->controlChange(1, f);
    }
    else {
        // release note also?
        msig_release_instance(sig, instance_id, *tt);
    }
}


/*! Creation of the mapper device*/
int setup_mapper()
{
    mapper_signal sig;
    float mn = 0.0;
    float mx = 20000.0;

    dev = mdev_new("BeeThree", 0, 0);
    if (!dev) goto error;
    printf("BeeThree device created.\n");

    sig = mdev_add_input(dev, "/frequency", 1, 'f', "Hz", &mn, &mx, freq_handler, voices);
    msig_reserve_instances(sig, NUMVOICES-1);
   // msig_set_instance_allocation_mode(sig, IN_STEAL_OLDEST);

    mx = 1.0;
    sig = mdev_add_input(dev, "/feedback_gain", 1, 'f', 0, &mn, &mx, feedback_gain_handler, voices);
    msig_reserve_instances(sig, NUMVOICES-1);

    sig = mdev_add_input(dev, "/gain", 1, 'f', 0, &mn, &mx, gain_handler, voices);
    msig_reserve_instances(sig, NUMVOICES-1);

    sig = mdev_add_input(dev, "/LFO/speed", 1, 'f', "Hz", &mn, 0, LFO_speed_handler, voices);
    msig_reserve_instances(sig, NUMVOICES-1);

    sig = mdev_add_input(dev, "/LFO/depth", 1, 'f', 0, &mn, &mx, LFO_depth_handler, voices);
    msig_reserve_instances(sig, NUMVOICES-1);

    printf("Number of inputs: %d\n", mdev_num_inputs(dev));
    return 0;

  error:
    return 1;
}

void cleanup_mapper()
{
    if (dev) {
        printf("Freeing mapper device-> ");
        fflush(stdout);
        mdev_free(dev);
        printf("ok\n");
    }
}

void wait_local_devices()
{
	while (!(mdev_ready(dev)))
			mdev_poll(dev, 100);
}

void ctrlc(int sig)
{
    done = 1;
}

int main()
{
	int result=0;
    float maxval = -100000;

    signal(SIGINT, ctrlc);
	
	// Set the global sample rate before creating class instances.
	Stk::setSampleRate( 44100.0 );
	Stk::showWarnings( true );
	Stk::setRawwavePath( RAWWAVE_PATH );

	int nFrames = (int)(2.5/1000*44100.0); // 5 ms
	RtWvOut *dac = 0;
	StkFrames frames( nFrames, 2 );

    if (setup_mapper()) {
        printf("Error initializing libmapper device\n");
        result = 1;
        goto cleanup;
    }

  	try {
        // Define and open the default realtime output device for one-channel playback
        dac = new RtWvOut( 2 );
        for (int i=0; i<NUMVOICES; i++)
            voices[i] = new BeeThree();
    }
 	catch ( StkError & ) {
        goto cleanup;
    }

	wait_local_devices();

	printf("-------------------- GO ! --------------------\n");
	while (!done) {
        mdev_poll(dev, 0);

        try {
            for (int i=0; i<nFrames; i++) {
                float out = 0;
                for (int j=0; j<NUMVOICES; j++)
                    out += voices[j]->tick(0);
                out /= NUMVOICES;
                if (out > 1) out = 1;
                if (out < -1) out = -1;
                frames(i, 1) = frames(i, 0) = out;
            }
            dac->tick( frames );
        }
        catch ( StkError & ) {
            goto cleanup;
        }
    }

cleanup:
    cleanup_mapper();
  	delete dac;
  	return result;
}
