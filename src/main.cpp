// Experiment 3

#include "mbed.h"
AnalogIn in(PTB1); // ADC
AnalogOut out(PTE30); // DAC
unsigned short in_buf[5];
unsigned short out_buf[5];

const int wait_time = 66000;
const float weighting = 0.2;

void rotate(int x, unsigned short y[5]) {
    int i = 0;
    while (i < x) {
        int j = 0;
        while (j < 4) {
            y[j] = y[j+1];
            j++;
        }
        i++;
    }
}

int main() {
    int i = 0;
    while (i < 5) {
        in_buf[i] = in.read_u16();
        out_buf[i] = in.read_u16();
        i++;
    }

    while (true) {
        rotate(1, in_buf);
        in_buf[4] = in.read_u16();

        unsigned short avg = 0;
        for (unsigned short x: out_buf) {
            avg += x;
        }
        avg = avg / 5;

        unsigned short to_out = (weighting * in_buf[4]) + ((1-weighting)*out_buf[4]) - avg + (short)((65535)*(0.5/3.3));
        rotate(1, out_buf);
        out_buf[4] = to_out;
        out.write_u16(to_out);
        wait_us(wait_time);
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

