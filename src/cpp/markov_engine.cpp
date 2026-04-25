#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <numeric>
#include <algorithm>

extern "C" {
    // Structure to represent a transition matrix result
    struct PredictionResult {
        double bearish_prob;
        double neutral_prob;
        double bullish_prob;
    };

    /**
     * Calculate Markov Chain transition probabilities
     * @param states: Array of integers representing states (0: Bearish, 1: Neutral, 2: Bullish)
     * @param size: Number of states in the array
     * @param last_state: The current/last observed state
     */
    PredictionResult calculate_markov_prediction(int* states, int size, int last_state) {
        // Initialize transition counts
        // Matrix index [current_state][next_state]
        double counts[3][3] = {0};
        
        for (int i = 0; i < size - 1; ++i) {
            int s1 = states[i];
            int s2 = states[i+1];
            if (s1 >= 0 && s1 < 3 && s2 >= 0 && s2 < 3) {
                counts[s1][s2]++;
            }
        }

        PredictionResult result = {0.0, 0.0, 0.0};
        
        if (last_state < 0 || last_state >= 3) return result;

        double total = counts[last_state][0] + counts[last_state][1] + counts[last_state][2];
        
        if (total > 0) {
            result.bearish_prob = counts[last_state][0] / total;
            result.neutral_prob = counts[last_state][1] / total;
            result.bullish_prob = counts[last_state][2] / total;
        } else {
            // Uniform distribution if no transitions observed from this state
            result.bearish_prob = 1.0/3.0;
            result.neutral_prob = 1.0/3.0;
            result.bullish_prob = 1.0/3.0;
        }

        return result;
    }
}
