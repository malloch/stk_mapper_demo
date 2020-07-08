#include <RtWvOut.h>
#include <cstdlib>
#include <cmath>
#include <BeeThree.h>
#include <stdio.h>
#include <math.h>

#include <unistd.h>
#include <arpa/inet.h>

extern "C" {
#include <mpr/mpr.h>
}

#define NUMVOICES 16

using namespace stk;

BeeThree *voices[NUMVOICES];
int active[NUMVOICES];
mpr_dev dev;
mpr_sig freq, gain, fbgain, lfo_speed, lfo_depth;
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

void release_instance(mpr_id instance)
{
    voices[instance]->noteOff(1);
    mpr_sig_release_inst(freq, instance);
    mpr_sig_release_inst(gain, instance);
    mpr_sig_release_inst(fbgain, instance);
    mpr_sig_release_inst(lfo_speed, instance);
    mpr_sig_release_inst(lfo_depth, instance);
    active[instance] = 0;
}

void freq_handler(mpr_sig sig, mpr_sig_evt evt, mpr_id inst, int len,
                  mpr_type type, const void *val, mpr_time time)
{
    switch (evt) {
        case MPR_SIG_UPDATE: {
            StkFloat f = *((float *)val);
            // need to check if voice is already active
            if (active[inst] == 0) {
                voices[inst]->noteOn(f, 1);
                active[inst] = 1;
                print_active_instances();
            }
            else {
                // can we perform polyphonic pitch-bend? use multiple channels?
                voices[inst]->setFrequency(f);
            }
            break;
        }
        case MPR_SIG_REL_UPSTRM:
            release_instance(inst);
            break;
        default:
            break;
    }
}

void feedback_gain_handler(mpr_sig sig, mpr_sig_evt evt, mpr_id inst, int len,
                           mpr_type type, const void *val, mpr_time time)
{
    if (MPR_SIG_UPDATE == evt) {
        StkFloat f = *((float *)val);
        voices[inst]->controlChange(2, f);
    }
    else if (MPR_SIG_REL_UPSTRM == evt) {
        // release note also?
        release_instance(inst);
    }
}

void gain_handler(mpr_sig sig, mpr_sig_evt evt, mpr_id inst, int len,
                  mpr_type type, const void *val, mpr_time time)
{
    if (MPR_SIG_UPDATE == evt) {
        StkFloat f = *((float *)val);
        voices[inst]->controlChange(4, f);
    }
    else if (MPR_SIG_REL_UPSTRM == evt) {
        release_instance(inst);
    }
}

void LFO_speed_handler(mpr_sig sig, mpr_sig_evt evt, mpr_id inst, int len,
                       mpr_type type, const void *val, mpr_time time)
{
    if (MPR_SIG_UPDATE == evt) {
        StkFloat f = *((float *)val);
        voices[inst]->controlChange(11, f);
    }
    else if (MPR_SIG_REL_UPSTRM == evt) {
        release_instance(inst);
    }
}

void LFO_depth_handler(mpr_sig sig, mpr_sig_evt evt, mpr_id inst, int len,
                       mpr_type type, const void *val, mpr_time time)
{
    if (MPR_SIG_UPDATE == evt) {
        StkFloat f = *((float *)val);
        voices[inst]->controlChange(1, f);
    }
    else if (MPR_SIG_REL_UPSTRM == evt) {
        release_instance(inst);
    }
}


/*! Creation of the device */
int setup()
{
    mpr_sig sig;
    float mn = 0.0;
    float mx = 20000.0;
    int inst = NUMVOICES, stl = MPR_STEAL_OLDEST;
    int evts = MPR_SIG_UPDATE | MPR_SIG_REL_UPSTRM;

    dev = mpr_dev_new("BeeThree", 0);
    if (!dev) goto error;
    printf("BeeThree device created.\n");

    freq = mpr_sig_new(dev, MPR_DIR_IN, "frequency", 1, MPR_FLT, "Hz",
                       &mn, &mx, &inst, freq_handler, evts);
    mpr_obj_set_prop(freq, MPR_PROP_DATA, NULL, 1, MPR_PTR, voices, 0);
    mpr_obj_set_prop(freq, MPR_PROP_STEAL_MODE, NULL, 1, MPR_INT32, &stl, 1);

    mx = 1.0;
    fbgain = mpr_sig_new(dev, MPR_DIR_IN, "feedback_gain", 1, MPR_FLT, 0,
                         &mn, &mx, &inst, feedback_gain_handler, evts);
    mpr_obj_set_prop(fbgain, MPR_PROP_DATA, NULL, 1, MPR_PTR, voices, 0);

    gain = mpr_sig_new(dev, MPR_DIR_IN, "gain", 1, MPR_FLT, 0,
                       &mn, &mx, &inst, gain_handler, evts);
    mpr_obj_set_prop(gain, MPR_PROP_DATA, NULL, 1, MPR_PTR, voices, 0);

    lfo_speed = mpr_sig_new(dev, MPR_DIR_IN, "LFO/speed", 1, MPR_FLT, "Hz",
                            &mn, 0, &inst, LFO_speed_handler, evts);
    mpr_obj_set_prop(lfo_speed, MPR_PROP_DATA, NULL, 1, MPR_PTR, voices, 0);

    lfo_depth = mpr_sig_new(dev, MPR_DIR_IN, "LFO/depth", 1, 'f', 0,
                            &mn, &mx, &inst, LFO_depth_handler, evts);
    mpr_obj_set_prop(lfo_depth, MPR_PROP_DATA, NULL, 1, MPR_PTR, voices, 0);
    return 0;

  error:
    return 1;
}

void cleanup()
{
    if (dev) {
        printf("Freeing device-> ");
        fflush(stdout);
        mpr_dev_free(dev);
        printf("ok\n");
    }
}

void wait_local_devices()
{
	while (!(mpr_dev_get_is_ready(dev)))
			mpr_dev_poll(dev, 100);
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
	Stk::setSampleRate(44100.0);
	Stk::showWarnings(true);
	Stk::setRawwavePath(RAWWAVE_PATH);

	int nFrames = (int)(2.5/1000*44100.0); // 5 ms
	RtWvOut *dac = 0;
	StkFrames frames(nFrames, 2);

    if (setup()) {
        printf("Error initializing device\n");
        result = 1;
        goto cleanup;
    }

  	try {
        // Define and open the default realtime output device for one-channel playback
        dac = new RtWvOut(2);
        for (int i = 0; i < NUMVOICES; i++)
            voices[i] = new BeeThree();
    }
 	catch ( StkError & ) {
        goto cleanup;
    }

	wait_local_devices();

	printf("-------------------- GO ! --------------------\n");
	while (!done) {
        if (mpr_dev_poll(dev, 0))
            print_active_instances();

        try {
            for (int i = 0; i < nFrames; i++) {
                float out = 0;
                for (int j = 0; j < NUMVOICES; j++)
                    out += voices[j]->tick(0);
                out /= NUMVOICES;
                if (out > 1) out = 1;
                if (out < -1) out = -1;
                frames(i, 1) = frames(i, 0) = out;
            }
            dac->tick(frames);
        }
        catch (StkError &) {
            goto cleanup;
        }
    }

cleanup:
    cleanup();
  	delete dac;
  	return result;
}
