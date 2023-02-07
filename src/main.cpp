#include <list>
#include "mbed.h"
AnalogIn in(PTB1); // ADC
AnalogOut out(PTE30); // DAC

unsigned short pre_filter_buf[8];
std::list<unsigned short> filtered_in_buf = {};

const int BUFFER_LENGTH = 128;
const int FIRST_ORDER_INIT_LENGTH = 1;

//
//void safe_append(unsigned short element, std::list<unsigned short> append_to) {
//    if (append_to.size() >= BUFFER_LENGTH) {
//        append_to.pop_front();
//        append_to.push_back(element);
//    } else {
//        append_to.push_back(element);
//    }
//}

unsigned short filter(unsigned short to_filter[8]) {
    int sum = 0;
    int count = 0;
    while (count < 8) {
        sum += to_filter[count];
        count++;
    }
    return sum >> 3;
}

unsigned short average_list_static_len(std::list<unsigned short> to_avg) {
    int sum = 0;
    for (int i: to_avg) {
        sum += i;
    }
    return sum >> 7;
}

int main() {
    int count = 0;
    while (true) {
        unsigned short input = in.read_u16(); // varies from 0 - 65535, 65535 = 3.3V, 0 = 0V.
        pre_filter_buf[count] = input;

        count++;
        if (count == 8) {
            // If minimum # of elems in buf, start filtering output.
            if (filtered_in_buf.size() >= FIRST_ORDER_INIT_LENGTH) {
                unsigned short filtered_input = filter(pre_filter_buf);
                filtered_in_buf.push_back(
                        ((filtered_input >> 2) + (filtered_input >> 1))
                        + (filtered_in_buf.back())
                );
            } else {
                filtered_in_buf.push_back(filter(pre_filter_buf));
            }


            // Perform trend removal, otherwise write const val.
            unsigned short offset = 0;
            if (filtered_in_buf.size() >= 129) {
                filtered_in_buf.pop_front();
                unsigned short trend = average_list_static_len(filtered_in_buf);
                unsigned short next_out = filtered_in_buf.back();

                if (next_out + offset < trend) {
                    offset = trend - next_out;
                }
                out.write_u16(offset + next_out - trend);
            } else {
                out.write_u16(32767);
            }
            count = 0;
        }
        ThisThread::sleep_for(2ms);
    }
}