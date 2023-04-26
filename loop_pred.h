#include <inttypes.h>

#define ENTRIES 256 // Number of entries in the predictor table
#define WAY 4       // Associativity of the predictor table
#define LOGIND 6    // Number of bits required to index into the table
#define LOGWAY 2    // Number of bits required to determine the way at a particular index
#define TAGSIZE 14  // Number of bits to represent tag in the table
#define ITERSIZE 14 // Max size of the loop that the predictor can predict properly
#define AGE 31      // Intiial age of the the entry

class LoopEntry {
    public:
    uint16_t tag;          // Stores the 14-bit tag for the entry
    uint16_t past_iter;    // Stores the 14-bit count for the number of iterations seen in past
    uint16_t current_iter; // Stores the 14-bit count for the number of iterations seen currently
    uint8_t age;           // 8-bit counter signifying age of entry
    uint8_t confidence;    // 2-bit counter signifying confidence in prediction

    LoopEntry() {
        this->tag = 0;
        this->past_iter = 0;
        this->current_iter = 0;
        this->age = 0;
        this->confidence = 0;
    }

    void update(uint16_t ptag, uint16_t past_iter, uint16_t current_iter, uint8_t age, uint8_t confidence) {
        this->tag = ptag;
        this->past_iter = past_iter;
        this->current_iter = current_iter;
        this->age = age;
        this->confidence = confidence;
    }
};

class LoopPred {
    public:
    LoopEntry* table[ENTRIES]; // Predictor table
    int ind;              // Index in loop
    int hit;              // The way in the loop where we get a hit else -1
    int ptag;             // The tag calculated
    uint8_t seed;
    bool is_valid;     // Validity of prediction
    uint8_t loop_pred; // The prediction returned for current PC

 LoopPred() {
        this->ind = 0;
        this->hit = 0;
        this->ptag = 0;
        this->seed = 0;
        this->is_valid = false;
        this->loop_pred = 0;
        for (int i = 0; i < ENTRIES; i++) {
            this->table[i] = new LoopEntry();
        }
    }

    uint8_t get_prediction(uint64_t ip) {
        hit = -1;
        ind = (ip & ((1 << LOGIND) - 1)) << LOGWAY;         // Calculate index
        ptag = (ip >> LOGIND) & ((1 << TAGSIZE) - 1);       // Calculate tag
        is_valid = false;
        loop_pred = 0;

        for (int i = ind; i < ind + WAY; i++) {
            if (table[i]->tag == ptag) {
                hit = i;
                if (table[i]->confidence == 3) {
                    is_valid = true;
                }
                if (table[i]->current_iter == table[i]->past_iter - 1) {
                    loop_pred = 0;
                    return 0;
                }

                loop_pred = 1;
                return 1;
            }
        }

        return 0;

    }

    void update_entry(uint8_t taken, uint8_t tage_pred) {
        if (hit < 0) {
            if (taken) {
                seed = (seed + 1) & 3;
                for (int i = 0; i < WAY; i++) {
                    int j = ind + ((seed + i) & 3);
                    if (table[j]->age == 0) {
                        table[j]->update(ptag, 0, 1, AGE, 0);
                        break;
                    } else if (table[j]->age > 0) {
                        table[j]->age--;
                    }
                }
            }
        } else {
            LoopEntry* entry = table[hit];
            if (is_valid) {
                // If the predicton was wrong, free the entry
                if (taken != loop_pred)
                {
                    entry->update(entry->tag, 0, 0, 0, 0);
                    return;
                }

                if (taken != tage_pred)
                {
                    if (entry->age < AGE)
                        entry->age++;
                }
            }

            entry->current_iter += 1;
            entry->current_iter &= ((1 << ITERSIZE) - 1);

            // If the iteration is greater than what was seen last time, free the entry
            if (entry->current_iter > entry->past_iter) {
                entry->confidence = 0;
                if (entry->past_iter != 0) {
                    entry->update(entry->tag, 0, entry->current_iter, 0, 0);
                }
            }

            if (!taken) {
                if (entry->current_iter == entry->past_iter) {
                    // Increase the confidence if correct
                    if (entry->confidence < 3)
                        entry->confidence += 1;
                    
                    // We do not care for loops with < 3 iterations
                    if ((entry->past_iter > 0) && (entry->past_iter < 3)) {
                        entry->update(entry->tag, 0, entry->current_iter, 0, 0);
                    }
                } else {
                    // Set the newly allocated entry
                    if (entry->past_iter == 0) {
                        entry->update(entry->tag, entry->current_iter, entry->current_iter, entry->age, 0);
                    } else {
                        entry->update(entry->tag, 0, entry->current_iter, 0, 0);
                    }
                }
                
                entry->current_iter = 0;
            }
        }
    }
};