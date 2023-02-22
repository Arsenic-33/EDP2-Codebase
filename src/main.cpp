#include <list>
#include <thread>
#include "mbed.h"

//Virtual serial port over USB
AnalogIn in(PTB1); // ADC
AnalogOut out(PTE30); // DAC

unsigned short pre_filter_buf[8];
unsigned short post_filter_buf[128];

const unsigned short TREND_REMOVAL_INIT_LENGTH = 128;


int main() {
    unsigned char pre_filter_buf_cursor = 0;
    bool lag_filter_enable = false;

    unsigned char post_filter_buf_head = 0;
    unsigned char post_filter_buf_tail = 0;

    unsigned short offset = 0;
    // HT_ _ _ _ _
    // H_1 T_ _ _ _
    // H_1 2 T_ _ _
    // H_1 2 3 T_ _
    // _ H_2 3 T_ _
    // _ H_2 3 4 T_5


    while (true) {
        unsigned short input = in.read_u16(); // Input from ADC.

        pre_filter_buf[pre_filter_buf_cursor] = input;
        pre_filter_buf_cursor++;

        if (pre_filter_buf_cursor == 8) {
            if (!lag_filter_enable) {
                // printf("Waiting for lag filter.");
                // Push to tail.
                post_filter_buf[post_filter_buf_tail] = input;
                post_filter_buf_tail++;
            } else {
                // printf("Starting Lag Filter\n");
                // Do bounds-check.
                if (post_filter_buf_tail >= TREND_REMOVAL_INIT_LENGTH) {
                    post_filter_buf_tail = 0;
                }
                if (post_filter_buf_head >= TREND_REMOVAL_INIT_LENGTH) {
                    post_filter_buf_head = 0;
                }
                if (post_filter_buf_tail == post_filter_buf_head) {
                    post_filter_buf_head++;
                }

                // Average last 8 samples
                unsigned int avg = 0;
                pre_filter_buf_cursor = 0;
                while (pre_filter_buf_cursor < 8) {
                    avg += pre_filter_buf[pre_filter_buf_cursor];
                    pre_filter_buf_cursor++;
                }

                avg = avg >> 3; // Cheap division by 8.
                post_filter_buf[post_filter_buf_tail] = avg;
                post_filter_buf_tail++;
            }
            // printf("Finished Lag Filter.\n");
            // Only true when buf has 128; Can now do rolling avg.
            if (post_filter_buf_tail == post_filter_buf_head) {
                // printf("Starting rolling avg.\n");
                unsigned int avg = 0;
                unsigned char cursor = 0;
                while (cursor < TREND_REMOVAL_INIT_LENGTH) {
                    avg += post_filter_buf[cursor];
                    cursor++;
                }
                avg = avg >> 7; // Cheap division by 128.
                if (avg > post_filter_buf[post_filter_buf_tail] + offset) {
                    offset = avg - post_filter_buf[post_filter_buf_tail];
                }

                // printf("%d %d %d %d\n", offset, avg, post_filter_buf_tail, post_filter_buf_head);
                out.write_u16(post_filter_buf[post_filter_buf_tail] - avg + offset);
            } else {
                out.write_u16(32768);
            }

            lag_filter_enable = true;
            pre_filter_buf_cursor = 0;
        }
        ThisThread::sleep_for(5ms);
    }
}