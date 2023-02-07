// Experiment 3

#include <list>
#include "mbed.h"
AnalogIn in(PTB1); // ADC
AnalogOut out(PTE30); // DAC

unsigned short pre_filter_buf[8];
std::list<unsigned short> filtered_in_buf = {};
std::list<unsigned short> out_buf = {};

const int BUFFER_LENGTH = 100;
const int INIT_LENGTH = 8;
const int wait_time = 66000;
const float weighting = 0.2;

void safe_append(unsigned short element, std::list<unsigned short> append_to) {
    if (append_to.size() >= BUFFER_LENGTH) {
        append_to.pop_front();
        append_to.push_back(element);
    } else {
        append_to.push_back(element);
    }
}

unsigned short filter(unsigned short to_filter[8]) {
    int sum = 0;
    int count = 0;
    while (count < 8) {
        sum += to_filter[count];
        count++;
    }
    return sum >> 3;
}


int main() {
    int count = 0;
    int count_2 = 0;
    while (true) {
        unsigned short input = in.read_u16(); // varies from 0 - 65535, 65535 = 3.3V, 0 = 0V.
        pre_filter_buf[count] = input;

        count++;
        if (count == 8) {
            // Write to filtered buf.
            // ...
            filtered_in_buf.push_back(filter(pre_filter_buf));

            // If minimum # of elems in buf, start outputting.
            if (count_2 < INIT_LENGTH) {
                out_buf.push_back(((filtered_in_buf.back() >> 2) + (filtered_in_buf.back() >> 1))+(out_buf.back()>>2));
            } else {
                out_buf.push_back(filtered_in_buf.back());
            }
            out.write_u16(out_buf.back());
            count = 0;
            count_2++;
        }
        ThisThread::sleep_for(2ms);
    }

//    int i = 0;
//    while (i < 5) {
//        in_buf[i] = in.read_u16();
//        out_buf[i] = in.read_u16();
//        i++;
//    }
//
//    while (true) {
//        rotate(1, in_buf);
//        in_buf[4] = in.read_u16();
//
//        unsigned short avg = 0;
//        for (unsigned short x: out_buf) {
//            avg += x;
//        }
//        avg = avg / 5;
//
//        unsigned short to_out = (weighting * in_buf[4]) + ((1-weighting)*out_buf[4]) - avg + (short)((65535)*(0.5/3.3));
//        rotate(1, out_buf);
//        out_buf[4] = to_out;
//        out.write_u16(to_out);
//        wait_us(wait_time);
    }




//    while (i < 5) {
//        x[i] = in.read_u16();
//        wait_us(wait_time);
//        i++;
//    }
//    while (true) {
//        d = (unsigned short)(weighting * (float)in.read_u16()) + (unsigned short)((1 - weighting)*((float)x[4]));
//        unsigned short avg = 0;
//        i = 0;
//        d -= avg;
//        i = 0;
//        while (i < 4) {
//            x[i] = x[i+1];
//            i++;
//        }
//        x[i] = d; // CHANGE?
//        out.write_u16(d);
//        wait_us(wait_time);
//    }
}

