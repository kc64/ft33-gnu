#include "mbed.h"
#include "SDFileSystem.h"

#include "bits.h"
#include "types.h"
#include "sequences.h"
#include "dim_steps.h"

// #define DEBUG 0
#ifdef DEBUG
#warning "Using debug mode."
#endif

// #define VERBOSE 0
#ifdef VERBOSE
#warning "Using verbose mode."
#endif

// #define USB_TEST
#ifdef USB_TEST
#warning "Using USB test mode."
#endif


/* Serial debug port. */
Serial pc(P1_13, P1_14); // tx, rx

InterruptIn int_ZCD(P0_2);
Timeout tmo_FastInt;
Ticker tkr_Timer;
Timeout tmo_ZCD_sync;
Ticker tkr_FastInt;
Timeout tmo_switch;

/* Determines the fastest and slowest sequence step timing. Times are in
    1/60th of a second (one clock).*/
#define FASTEST_TIME 10.0
#define SLOWEST_TIME 300.0
/* The AnalogIn function scales the voltage input to a float 0.0-1.0. */
#define SLOPE (SLOWEST_TIME - FASTEST_TIME)

/* These coefficients are used to convert the potentiometer input to a 
exponetial curve that mimics the desired response. */
#define A_COEFF 0.0207
#define B_COEFF 3.9
#define C_COEFF -0.0207

/* The potentiometer input port to select the speed of the sequence steps. */
AnalogIn potentiometer(P0_11);

/* Setup the output pins. */
DigitalOut C0(P0_16);
DigitalOut C1(P0_17);
DigitalOut C2(P0_18);
DigitalOut C3(P0_19);
DigitalOut C4(P0_20);
DigitalOut C5(P0_21);
DigitalOut C6(P0_22);
DigitalOut C7(P0_23);

BusOut lights(P0_23, P0_19, P0_22, P0_18, P0_21, P0_17, P0_20, P0_16);

/* Setup the dipswitch input port. */
BusInOut dipswitch(P1_23, P0_12, P0_13, P0_14, P0_7, P0_8, P0_9, P1_24);
DigitalInOut master_slave(P0_4);
DigitalInOut local_slave_data(P0_5);

/* Setup the reset switch as an input to keep it from being a reset */
DigitalInOut reset(P0_0);

/* Setup the SD card detect input */
DigitalInOut sd_present(P1_15);

float speed;            /* The selected speed. */
int speed_clks;         /* speed in clocks (1/60th sec). */
int clocks;             /* Incremented everytime the zero cross interrupt is called. */
byte pattern;           /* The current output pattern. */
byte *ptrSequence;      /* A pointer to the desired sequence. */

word sequenceLength;    /* The length of the desired sequence. */
byte step;              /* The step in the current sequence. */
byte num_ticks_per_step;  /* Each step can have one or more ticks before it changes. */
char line[100];
byte master_sequence; 
int got_Z = 0;
int got_R = 0;
char stmp[10];
byte current_step, slave_channel;
sDimStep *ptrDimSequence;
int ok_to_switch;

byte ticks = 0;

/* The dimmer timers for each channel. */
uint8_t Dimmer[8] = {0, 0, 0, 0, 0, 0, 0, 0};
/* Scaled version of the the dimmer for fixed point calculations. */
int Dimmer_sc[8] = {0, 0, 0, 0, 0, 0, 0, 0};

void ZCD(void) {

    clocks++;
    if(clocks > speed_clks) {
        clocks = 0;
        step++;
        if(step >= sequenceLength) {
            step = 0;
			pc.putc('R');
        }
		else {
			pc.putc('Z');
		}
        pattern = ~ptrSequence[step];
        #ifdef VERBOSE
        //pc.printf("P:%02x ", ptrSequence[step]);
        #endif
        lights = pattern;  
    }
    
    #ifdef VERBOSE
    //pc.printf("Z. clocks: %d, speed_clks: %d\n\r", clocks, speed_clks);
    #endif
}

void ZCD_Slave(void) {
	if(got_R == 1) {
        step = 0;
	}
    else if(got_Z == 1) {
        step++;
        if(step >= sequenceLength) {
			step = 0;
        }
    }
	
	if ((got_R == 1) or (got_Z == 1)) {
        pattern = ~ptrSequence[step];
        #ifdef VERBOSE
        //pc.printf("P:%02x ", ptrSequence[step]);
        #endif
        lights = pattern;

		got_R = 0;
        got_Z = 0;		
	}
	
	#ifdef VERBOSE
    //pc.printf("Z. clocks: %d, speed_clks: %d\n\r", clocks, speed_clks);
    #endif
}

/* This routine is called about 255 times every 1/60 of a second. It is our chance to turn on a 
    channel based on the dimmer value. Dimmer is calculated at each good zero cross in TimeToSwitch(). */
void tmr_Main(void) {
    
    if (Dimmer[0] != 0) {
        Dimmer[0]--;
    } else {
        C0 = 0;
    }
    if (Dimmer[1] != 0) {
        Dimmer[1]--;
    } else {
        C1 = 0;
    }
    if (Dimmer[2] != 0) {
        Dimmer[2]--;
    } else {
        C2 = 0;
    }
    if (Dimmer[3] != 0) {
        Dimmer[3]--;
    } else {
        C3 = 0;
    }
    if (Dimmer[4] != 0) {
        Dimmer[4]--;
    } else {
        C4 = 0;
    }
    if (Dimmer[5] != 0) {
        Dimmer[5]--;
    } else {
        C5 = 0;
    }
    if (Dimmer[6] != 0) {
        Dimmer[6]--;
    } else {
        C6 = 0;
    }
    if (Dimmer[7] != 0) {
        Dimmer[7]--;
    } else {
        C7 = 0;
    }
}

/* Sync to every other zero cross. See ZCD_SD() below.*/
void sync(void) {
    
    ok_to_switch = TRUE;
    
}

/* Actually switch everything off. We are at a zero cross. Check th speed setting, find next ticks and steps. 
   Calculate the new dimmer setting based on the start and stop values. */
   
void TimeToSwitch(void) {
    int i;
    
    tkr_FastInt.attach_us(NULL, 66);    
    C0 = 1;
    C1 = 1;
    C2 = 1;
    C3 = 1;
    C4 = 1;
    C5 = 1;
    C6 = 1;
    C7 = 1;
    /* A clock is a zero cross (1/60 second). */
    clocks++;
    /* We need speed_clks number of clocks before we tick. speed_clks is from the speed pot. */
    if(clocks > speed_clks) {
        clocks = 0;
        ticks++;
        /* If we tick enough times for this step, goto the nect step. */
        if (ticks >= ptrDimSequence[step].ticks) {
            step++;
            ticks = 0;
        }
        /* If we step past the end of a sequence, restart the sequence. */
        if(step >= sequenceLength) {
            step = 0;
        }
        /* Put out the Z sync to the slaves. This tells them to step there sequence. */
		#ifdef DEBUG
		pc.printf("Z %02x\n", step);
		#endif
        // pc.putc('Z');

        for(i=0; i<8; i++) {
            Dimmer[i] = ptrDimSequence[step].Chan[i].start;
        }
    } else {
        /* If we don't need to tick or step, then find the new dimmer values for the next 1/60 second clock. This calcuation is a simple linear interperloation
            between the start and stop. At each 1/60 second interval we recalc the dimmer value along the line. */
        for(i=0; i<8; i++) {
            Dimmer[i] = ptrDimSequence[step].Chan[i].start + (((clocks << 8)/speed_clks * (ptrDimSequence[step].Chan[i].stop - ptrDimSequence[step].Chan[i].start)) >> 8);
        }   
    }    
    /* Timer for the 255 step dimmer reoutine. */
    tkr_FastInt.attach_us(&tmr_Main, 66);
    
    #ifdef VERBOSE
    //pc.printf("Z. clocks: %d, speed_clks: %d\n\r", clocks, speed_clks);
    #endif

}

/* Interrupt routine that is called by the master ion a zero cross event. */       
void ZCD_SD(void) {

    /* Check if it's ok to actually switch. The zero cross is only reliable in one direction of the sine wave
        crossing of zero. The sync routine delays the next good zero cross detection by 14ms. Once a zero cross
        happens, and the ok_to_switch flag is true, every other zero cross will be detected from that point on. */
    if (ok_to_switch) {
        
        ok_to_switch = FALSE;
        /* Don't actually switch here. We are too late. The zero cross happened a few 100 us ago. Wait until the next zero cross.
           This 8ms had been timed to be pretty close from observation on a scope. If you change this routine, you might 
           need to retune this number. */
        tmo_switch.attach(&TimeToSwitch, 0.008);
        /* Set up ok_to_switch routine to catch every other zero cross. */
        tmo_ZCD_sync.attach(&sync, 0.014);  
    }
}

void ZCD_SD_Slave(void) {
    
    clocks++;
    if(got_Z == 1) {
        got_Z = 0;
        clocks = 0;
        ticks++;
        if (ticks >= ptrDimSequence[step].ticks) {
            step++;
            ticks = 0;
        }
        if(step != current_step) {
            step = current_step;
        }
        if(step >= sequenceLength) {
            step = 0;
        }
        //pattern = ~ptrDimSequence[step].Chan[slave_channel];
        #ifdef VERBOSE
        //pc.printf("P:%02x %02x %02x, %02x", ptrDimSequence[step].Chan[0], step, ticks, current_step);
        #endif
        lights = pattern;
    } 
}

void vfnLoadSequencesFromSD(void) {
    /* Load one line at a time. Output each line in turn to the slaves. This is done to preserve memeory.
        NOT DONE YET */
    
    
    FILE *fp;
    int steps;
    int seq = 0;
    sDimStep *ptr = NULL;
    SDFileSystem sd(P1_22, P1_21, P1_20, P1_19, "sd"); // the pinout on the FT33 controller
    
    fp = fopen("/sd/seq.txt", "r");
    if(fp == NULL) {
        pc.printf("No file. Restarting...\n");
        // if the SD card is present but not responding, reset and try again
        NVIC_SystemReset();
    } else {
        
        while(fgets(line, 100, fp) != NULL) {
            if(line[0] == 'Q') {
                sscanf(line, "%*s %*s %d", &steps);
                ptr = (sDimStep *) malloc(sizeof(sDimStep) * steps);
                ptrDimSequences[seq] = ptr;
                DimSequenceLengths[seq] = steps;
                seq++;
            } else if(line[0] == 'S') {
                sscanf(line, "%*s %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu",
                            &ptr->ticks,
                            &ptr->Chan[0].start, &ptr->Chan[0].stop, 
                            &ptr->Chan[1].start, &ptr->Chan[1].stop,
                            &ptr->Chan[2].start, &ptr->Chan[2].stop,
                            &ptr->Chan[3].start, &ptr->Chan[3].stop,
                            &ptr->Chan[4].start, &ptr->Chan[4].stop,
                            &ptr->Chan[5].start, &ptr->Chan[5].stop,
                            &ptr->Chan[6].start, &ptr->Chan[6].stop,
                            &ptr->Chan[7].start, &ptr->Chan[7].stop);
                //for(i=0; i<8; i++) {            
                //    ptr->Chan[i].delta = (int) ptr->Chan[i].stop - (int) ptr->Chan[i].start;
               // }
                ptr++;
            }  
        }
        fclose(fp);
    }
}

void vfnBroadcastSequences(void) {
    
    int seq;
    unsigned int step;
    int chan;
    
    pc.printf("N\n");
    for(seq=0; seq<=15; seq++) {
        ptrDimSequence = (sDimStep *) ptrDimSequences[seq];
        sequenceLength = DimSequenceLengths[seq];
        pc.printf("Q %02x %02x\n", seq, sequenceLength);
        for(step=0; step < sequenceLength; step++) {
            pc.printf("S ");
            pc.printf("%02x ", ptrDimSequence[step].ticks);
            for(chan=0; chan<8; chan++) {
                pc.printf("%02x %02x ", ptrDimSequence[step].Chan[chan].start, ptrDimSequence[step].Chan[chan].stop);
            }
            pc.printf("\n");
        }
    }
}

void vfnGetLine(void) {
    
    int num = 0;
    char c;

    while(((c = pc.getc()) != '\n') && num < 98) {
        line[num] = c;
        num++;
    }
    line[num] = 0x00;
    // pc.printf("%s\n", line);
}

void vfnSlaveRecieveData(void) {

    #define MASTER 0
    #define SEQUENCE 1
    #define Q 3
    #define S 4
    
    #define FINISHED 255
    
    int state = MASTER;
    word num_steps;
    word step_number=0;
    sDimStep *ptr = NULL;
    byte seq_num = 0;
    //byte chan;

    while(state != FINISHED) {
        vfnGetLine(); 
        
        switch(state) {
            case MASTER:
                if(strcmp(line, "Master") == 0) {
                    state = Q;
                    //pc.putc('M');
                }
                break;
                
            case Q:
                if(strncmp(line, "Q", 1) == 0) {
                    sscanf(line, "%*s %hhx %x", &seq_num, &num_steps);
                    //pc.printf("\nseq_num: %hhx, num_steps: %x\n", seq_num, num_steps);
                    state = S;
                    step_number = 0;
                    ptr = (sDimStep *) malloc(sizeof(sDimStep) * num_steps);
                    if(ptr==NULL) {
                        while(1);
                    }
                    ptrDimSequences[seq_num] = ptr;
                    DimSequenceLengths[seq_num] = num_steps;
                    //pc.putc('Q');
                }    
                break;
            
            case S:
                if(strncmp(line, "S", 1) == 0) {
                    //pc.printf("\nline: %s\n", line);
                    sscanf(line, "%*s %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx",
                            &ptr->ticks,
                            &ptr->Chan[0].start, &ptr->Chan[0].stop, 
                            &ptr->Chan[1].start, &ptr->Chan[1].stop,
                            &ptr->Chan[2].start, &ptr->Chan[2].stop,
                            &ptr->Chan[3].start, &ptr->Chan[3].stop,
                            &ptr->Chan[4].start, &ptr->Chan[4].stop,
                            &ptr->Chan[5].start, &ptr->Chan[5].stop,
                            &ptr->Chan[6].start, &ptr->Chan[6].stop,
                            &ptr->Chan[7].start, &ptr->Chan[7].stop);
                                                
                    //pc.printf("\nSlave: ");
                    //for(chan=0; chan<16; chan++) {
                    //    pc.printf("%02x ", ptr->Chan[chan]);
                    //}      
                    //pc.putc('\n');
                    
                    step_number++;
                    ptr++;
                    if(step_number == num_steps) {
                        if(seq_num == 15) {
                            state = SEQUENCE;
                        } else {
                            state = Q;
                            seq_num++;
                             //pc.putc('s');
                        }
                    } else {
                        state = S;
                        //pc.putc('S');
                    }
                }                    
                break; 
                         
            case SEQUENCE:
                if(strncmp(line, "sequence", 8) == 0) {
                    sscanf(line, "%*s %hhx", &master_sequence);
                    pc.printf("\nseq: %hhx\n", master_sequence);
                    state = FINISHED;
                }
                break;
        }   
            
    }
}    
    
int main() {

    byte sequence;          /* The current sequence. */
    byte i;
    byte sd;
	byte sync_char;

    /* Basic initialization. */
    ok_to_switch = TRUE;
    clocks = 0;
    speed_clks = FASTEST_TIME;
    
    master_slave.mode(PullUp);
    master_slave.input();

    local_slave_data.mode(PullUp);
    local_slave_data.input();

    int_ZCD.mode(PullUp); 
    
    dipswitch.mode(PullUp);
    dipswitch.input();

    lights.write(0xFF); /* all off */
    /* Wait for the XBEE radio to get ready. It takes a while. */
    for(i=0xFF; i>=0xF4; i--) {
        wait(1.0);
//        lights = i;
    }
    
    sd_present.mode(PullUp);
    sd_present.input();
    sd = !sd_present.read();
    
    wait(1.0);

    /* Check master/slave and if a slave should use it's own sequence selection or get 
        it from a master. */
    if(master_slave.read() == 1) {
        /* Read the dipswitch */
        sequence = dipswitch.read();

        pc.printf("\nMaster\n");

        if (sd) {
            #ifdef DEBUG
            pc.printf("\nSD found\n");
            #endif
            vfnLoadSequencesFromSD();
            /* Broadcast all the SD card sequences. */
            vfnBroadcastSequences();
        }
        #ifdef DEBUG
        else {
            pc.printf("\nNo SD found\n");
        }
        #endif

        pc.printf("sequence: %02x\n", sequence);
        if(sequence < 240) {
            ptrSequence = (byte *) ptrSequences[sequence];
            sequenceLength = sequenceLengths[sequence];
            tkr_Timer.attach_us(&ZCD, 8333);
        }
		else {
            sequence = sequence - 240;
            ptrDimSequence = (sDimStep *) ptrDimSequences[sequence];
            sequenceLength = DimSequenceLengths[sequence];
            
            //tkr_Timer.attach_us(&ZCD_SD, 8333);
            /* This sets an interupt when a zero cross is detected. */
            int_ZCD.rise(&ZCD_SD);
        }

        clocks = SLOWEST_TIME;
        while(1) {
            /* Read the potentiometer. */
            
            speed = A_COEFF * exp(B_COEFF*(1.0-potentiometer)) + C_COEFF;
    
            __disable_irq();    // Disable Interrupts
            /* Changes the analog speed voltage to a time in clocks. */
            speed_clks = SLOPE * speed + FASTEST_TIME;
            __enable_irq();     // Enable Interrupts 
    
            //pc.printf("C %i\n", speed_clks);
            wait(0.5);
        }
    }
	else {
        /* This is a slave board. */
		#ifdef DEBUG
        pc.printf("Slave\n");
		#endif

        if(local_slave_data.read() == 1) {
			vfnSlaveRecieveData();
            sequence = master_sequence;    
			#ifdef DEBUG
            pc.printf("Use master data %d\n", sequence);
			#endif
        }
		else {
            /* Read the dipswitch */
            sequence = dipswitch.read();
			#ifdef DEBUG
            pc.printf("Use slave seqence %d\n", sequence);
			#endif
        }

        if(sequence < 240) {
            ptrSequence = (byte *) ptrSequences[sequence];
            sequenceLength = sequenceLengths[sequence];
			
            tkr_Timer.attach_us(&ZCD_Slave, 8333);
        }
		else {
            sequence = sequence - 240;
            ptrDimSequence = (sDimStep *) ptrDimSequences[sequence];
            sequenceLength = DimSequenceLengths[sequence];
                                  
            tkr_Timer.attach_us(&ZCD_SD_Slave, 8333);
        }

        clocks = SLOWEST_TIME;

        while(1) {
            // vfnGetLine();
            
            __disable_irq();
            // if( line[0] == 'Z') {
            //    sscanf(line, "%*s %hhx", &current_step);
                //pc.printf("\nStep: %s, %02x\n", line, current_step);
            //    got_Z = 1;
            // }
			sync_char = pc.getc();
			if (sync_char == 'R') {
				got_R = 1;
			}
			else if (sync_char == 'Z') {
				got_Z = 1;
			}
            __enable_irq();
        }
    }
}

