#include "ooo_cpu.h"
#include <iostream>

#define BIMODAL_TABLE_SIZE 16384
#define BIMODAL_PRIME 16381
#define MAX_COUNTER 3
int bimodal_table[NUM_CPUS][BIMODAL_TABLE_SIZE];

int bank0_predictions = 0;
int bank0_corr_predictions = 0;
int pred = 0;

void O3_CPU::initialize_branch_predictor()
{
    cout << "CPU " << cpu << " Bimodal branch predictor" << endl;

    for(int i = 0; i < BIMODAL_TABLE_SIZE; i++)
        bimodal_table[cpu][i] = 0;
}

uint8_t O3_CPU::predict_branch(uint64_t ip)
{
    uint32_t hash = ip % BIMODAL_PRIME;
    uint8_t prediction = (bimodal_table[cpu][hash] >= ((MAX_COUNTER + 1)/2)) ? 1 : 0;
    pred = prediction;
    return prediction;
}

void O3_CPU::last_branch_result(uint64_t ip, uint8_t taken)
{
    uint32_t hash = ip % BIMODAL_PRIME;

    bank0_predictions++;
    if (taken == pred) {
        bank0_corr_predictions++;
    }

    if (taken && (bimodal_table[cpu][hash] < MAX_COUNTER))
        bimodal_table[cpu][hash]++;
    else if ((taken == 0) && (bimodal_table[cpu][hash] > 0))
        bimodal_table[cpu][hash]--;

    cout << "bank0 " << bank0_predictions << endl;
    cout << "bank0 correct " << bank0_corr_predictions << endl;
}
