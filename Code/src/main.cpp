#include "mbed.h"
#include <thread>

#define max7219_reg_noop         0x00
#define max7219_reg_digit0       0x01
#define max7219_reg_digit1       0x02
#define max7219_reg_digit2       0x03
#define max7219_reg_digit3       0x04
#define max7219_reg_digit4       0x05
#define max7219_reg_digit5       0x06
#define max7219_reg_digit6       0x07
#define max7219_reg_digit7       0x08
#define max7219_reg_decodeMode   0x09
#define max7219_reg_intensity    0x0a
#define max7219_reg_scanLimit    0x0b
#define max7219_reg_shutdown     0x0c
#define max7219_reg_displayTest  0x0f

#define LOW 0
#define HIGH 1

bool chars[10][15] = {
        {
                1,1,1,
                1,0,1,
                1,0,1,
                1,0,1,
                1,1,1
        },
        {
                0, 1, 0,
                0, 1, 0,
                0, 1, 0,
                0, 1, 0,
                0, 1, 0
        },
        {
                1,1,1,
                0,0,1,
                1,1,1,
                1,0,0,
                1,1,1
        },
        {
                1,1,1,
                0,0,1,
                1,1,1,
                0,0,1,
                1,1,1
        },
        {
                1,0,1,
                1,0,1,
                1,1,1,
                0,0,1,
                0,0,1
        },
        {
                1,1,1,
                1,0,0,
                1,1,1,
                0,0,1,
                1,1,1
        },
        {
                1,1,1,
                1,0,0,
                1,1,1,
                1,0,1,
                1,1,1
        },
        {
                1,1,1,
                0,0,1,
                0,1,0,
                0,1,0,
                0,1,0
        },
        {
                1,1,1,
                1,0,1,
                1,1,1,
                1,0,1,
                1,1,1
        },
        {
                1,1,1,
                1,0,1,
                1,1,1,
                0,0,1,
                1,1,1
        }
};

SPI max72_spi(PTD2, NC, PTD1);
DigitalOut load(PTD0); //will provide the load signal



// Screen Display Code

void write_to_max( int reg, int col)
{
    load = LOW;            // begin
    max72_spi.write(reg);  // specify register
    max72_spi.write(col);  // put data
    load = HIGH;           // make sure data is loaded (on rising edge of LOAD/CS)
}

//writes 8 bytes to the display
void pattern_to_display(char *testdata){
    int cdata;
    for(int idx = 0; idx <= 7; idx++) {
        cdata = testdata[idx];
        write_to_max(idx+1,cdata);
    }
}


void setup_dot_matrix ()
{
    // initiation of the max 7219
    // SPI setup: 8 bits, mode 0
    max72_spi.format(8, 0);



    max72_spi.frequency(100000); //down to 100khx easier to scope ;-)


    write_to_max(max7219_reg_scanLimit, 0x07);
    write_to_max(max7219_reg_decodeMode, 0x00);  // using an led matrix (not digits)
    write_to_max(max7219_reg_shutdown, 0x01);    // not in shutdown mode
    write_to_max(max7219_reg_displayTest, 0x00); // no display test
    for (int e=1; e<=8; e++) {    // empty registers, turn all LEDs off
        write_to_max(e,0);
    }
    // maxAll(max7219_reg_intensity, 0x0f & 0x0f);    // the first 0x0f is the value you can set
    write_to_max(max7219_reg_intensity,  0x08);

}

void clear(){
    for (int e=1; e<=8; e++) {    // empty registers, turn all LEDs off
        write_to_max(e,0);
    }
}
char head_and_foot[3] = {0x00, 0x00, 0x00};


// Define input output pins

AnalogIn in(PTB3); // ADC
AnalogOut out(PTE30); // DAC

// DEFINE CONSTANTS
unsigned char SAMPLE_BUFFER_SIZE = 128;
unsigned short TRIGGER_TOP = 31500; // TODO: Use log to investigate suitable value!
unsigned short TRIGGER_BOTTOM = 28700;

int main() {
//    unsigned short sample_buf[SAMPLE_BUFFER_SIZE];
//    bool _do_average = false;
    bool _do_lag_filter = false;
    unsigned short sample;
    Timer talwayson;

    unsigned char sample_buf_head = 0;
    unsigned char sample_buf_tail = 0;
    unsigned char sample_counter = 0;
    unsigned char gaining_count = 0;
    unsigned char falling_count = 0;
    unsigned char SAMPLESCOPE= 20;
    unsigned short last_sample = 0;
    unsigned char timercount = 0;
    unsigned char BPM;
    unsigned char to_display = 0;
    bool gaining_flag = false;
    unsigned char zero_samples = 0;
    bool zero_flag = true;

    setup_dot_matrix();
    while (true) {
        last_sample = sample;
        sample = in.read_u16();
        if ((sample > 30100) && (sample < 30250)) {
            zero_samples++;
        } else {
            zero_samples = 0;
        }

        if ((zero_samples < 30 && zero_flag == false) || (zero_samples < 10 && zero_flag == true)){
            zero_flag = false;
            if (sample_buf_tail >= 128) {
                sample_buf_tail = 0;
            }
            if (sample_buf_head >= 128) {
                sample_buf_head = 0;
            }
            if (sample_buf_head == sample_buf_tail) {
                sample_buf_head++;
            }

    //        sample_buf[sample_buf_tail] = sample;
            sample_buf_tail++;

            out.write_u16(sample);

            if (sample > last_sample) {
                if(gaining_flag){
                    gaining_count++;
                }
            } else {
                if(!gaining_flag){
                    falling_count++;
                }
            }
            timercount++;
            if (!gaining_flag && falling_count > 4){
                if (!_do_lag_filter) {
                    _do_lag_filter = true;
                    BPM = (60/( timercount * 0.03 ));
                } else {
                    BPM = (60 / (timercount * 0.03)) * 0.25 + (BPM * 0.75);
                }
                gaining_flag = !gaining_flag;
                falling_count = 0;
                timercount=0;

            }

            else if (gaining_count > 6 && gaining_flag) {
                gaining_flag = !gaining_flag;
                gaining_count =0;
            }

            to_display = BPM;
        } else {
            zero_flag = true;
            _do_lag_filter = false;
            to_display = 0;
        }


        unsigned char hundreds = to_display / 100 % 10;
        unsigned char tens = to_display / 10 % 10;
        unsigned char ones = to_display % 10;

        unsigned char build[5] = {
                0x00,
                0x00,
                0x00,
                0x00,
                0x00
        };
        for (char i = 0; i < 5; i++) {
            unsigned char line = 0;

            if (hundreds == 1) {
                line += (1 << 6);
            }
            line += (chars[tens][(i*3)] << 4);
            line += (chars[tens][(i*3)+1] << 3);
            line += (chars[tens][(i*3)+2] << 2);
            line += (chars[ones][(i*3)] << 1);
            line += (chars[ones][(i*3)+1]);
            line += (chars[ones][(i*3)+2] << 7);
            build[i] = line;
        }
        unsigned char final[8] = {
                head_and_foot[0],
                head_and_foot[1],
                build[0],
                build[1],
                build[2],
                build[3],
                build[4],
                head_and_foot[2]
        };

        pattern_to_display(final);
//
//
//
//        bool _Tflag = false;
//
//        if (_Tflag) {
//            if (sample < TRIGGER_BOTTOM) {
//                _Tflag = false;
//            }
//        } else {
//            if (sample > TRIGGER_TOP) {
//                _Tflag = true;
//            }
//        }
//
//        bool _timing = false;
//        bool _dipped = false;
//
//        if (_Tflag) {
//            if (!_timing) {
//                _timing = true;
//                _dipped = false;
//                talwayson.start();
//            } else {
//                if (_dipped) {
//                    _timing = false;
//                    _dipped = false;
//                    talwayson.stop();
//                    heart_rate = (unsigned char) (60*1000)/talwayson.read();
//                    talwayson.reset();
//                }
//            }
//        } else {
//            if ((_timing) && (!_dipped)) {
//                _dipped = true;
//            }
//        }
//
//        printf("Heart Rate: %d\n", heart_rate);

        // Activate filters.
//        if ((sample_buf_cursor == 1) && (!_do_lag_filter)) {
//            _do_lag_filter = true;
//        }
//        if (sample_buf_cursor == 127) {
//            //pisort(sample_buf, sample_buf+(sizeof(sample_buf)/sizeof(sample_buf[0])));
//            for (sample_buf_cursor = 0; sample_buf_cursor<=127; sample_buf_cursor++) {
//                printf("Sample: %d\n", sample_buf[sample_buf_cursor]);
//            }
//            sample_buf_cursor = 0;
//        } else {
//            // Counters
//            sample_buf_cursor++;
//        }

        // Sample rate
        ThisThread::sleep_for(30ms);
    }
}

// FOOO2


//
//
//
//
//#include <list>
//#include <thread>
//#include "mbed.h"
//
//
//
//
//// Screen Display Header
//#define max7219_reg_noop         0x00
//#define max7219_reg_digit0       0x01
//#define max7219_reg_digit1       0x02
//#define max7219_reg_digit2       0x03
//#define max7219_reg_digit3       0x04
//#define max7219_reg_digit4       0x05
//#define max7219_reg_digit5       0x06
//#define max7219_reg_digit6       0x07
//#define max7219_reg_digit7       0x08
//#define max7219_reg_decodeMode   0x09
//#define max7219_reg_intensity    0x0a
//#define max7219_reg_scanLimit    0x0b
//#define max7219_reg_shutdown     0x0c
//#define max7219_reg_displayTest  0x0f
//
//#define LOW 0
//#define HIGH 1
//
//char  pattern_square[8] = { 0xff, 0xff,0xff,0xff,0xff,0xff,0xff,0xff};
//char  pattern_off[8] = { 0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00};
//

//

//
//
//// Screen Head//footer
//char head_and_foot[3] = {0x00, 0x00, 0x00};
//
//
//
//
//AnalogIn in(PTB3); // ADC
//AnalogOut out(PTE30); // DAC
//
//unsigned short pre_filter_buf[8];
//unsigned short post_filter_buf[128];
//
//const unsigned short TREND_REMOVAL_INIT_LENGTH = 128;
//
//
//int main() {
//    setup_dot_matrix ();      /* setup matric */
//    unsigned char pre_filter_buf_cursor = 0;
//    bool lag_filter_enable = false;
//
//    unsigned char post_filter_buf_head = 0;
//    unsigned char post_filter_buf_tail = 0;
//
//    unsigned char to_display = 0;
//
//    unsigned short offset = 0;
//    // HT_ _ _ _ _
//    // H_1 T_ _ _ _
//    // H_1 2 T_ _ _
//    // H_1 2 3 T_ _
//    // _ H_2 3 T_ _
//    // _ H_2 3 4 T_5
//
//
//    while (true) {
//        unsigned short input = in.read_u16(); // Input from ADC.
//
//        pre_filter_buf[pre_filter_buf_cursor] = input;
//        pre_filter_buf_cursor++;
//
//        if (pre_filter_buf_cursor == 8) {
//            if (!lag_filter_enable) {
//                // printf("Waiting for lag filter.");
//                // Push to tail.
//                post_filter_buf[post_filter_buf_tail] = input;
//                post_filter_buf_tail++;
//            } else {
//                // printf("Starting Lag Filter\n");
//                // Do bounds-check.
//                if (post_filter_buf_tail >= TREND_REMOVAL_INIT_LENGTH) {
//                    post_filter_buf_tail = 0;
//                }
//                if (post_filter_buf_head >= TREND_REMOVAL_INIT_LENGTH) {
//                    post_filter_buf_head = 0;
//                }
//                if (post_filter_buf_tail == post_filter_buf_head) {
//                    post_filter_buf_head++;
//                }
//
//                // Average last 8 samples
//                unsigned int avg = 0;
//                pre_filter_buf_cursor = 0;
//                while (pre_filter_buf_cursor < 8) {
//                    avg += pre_filter_buf[pre_filter_buf_cursor];
//                    pre_filter_buf_cursor++;
//                }
//
//                avg = avg >> 3; // Cheap division by 8.
//                post_filter_buf[post_filter_buf_tail] = avg;
//                post_filter_buf_tail++;
//            }
//            // printf("Finished Lag Filter.\n");
//            // Only true when buf has 128; Can now do rolling avg.
//            if (post_filter_buf_tail == post_filter_buf_head) {
//                // printf("Starting rolling avg.\n");
//                unsigned int avg = 0;
//                unsigned char cursor = 0;
//                while (cursor < TREND_REMOVAL_INIT_LENGTH) {
//                    avg += post_filter_buf[cursor];
//                    cursor++;
//                }
//                avg = avg >> 7; // Cheap division by 128.
//                if (avg > post_filter_buf[post_filter_buf_tail] + offset) {
//                    offset = avg - post_filter_buf[post_filter_buf_tail];
//                }
//                // Display Counter
//                to_display += 1;

//                // printf("%d %d %d %d\n", offset, avg, post_filter_buf_tail, post_filter_buf_head);
//                out.write_u16(post_filter_buf[post_filter_buf_tail] - avg + offset);
//            } else {
//                out.write_u16(32768);
//            }
//
//            lag_filter_enable = true;
//            pre_filter_buf_cursor = 0;
//        }
//
//        ThisThread::sleep_for(5ms);
//    }
// }