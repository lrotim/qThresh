#include <stdio.h>
#include <stdlib.h>
#include <boost/math/special_functions/binomial.hpp>
#include <unordered_map>
#define min(x,y) x < y ? x : y

using namespace std;

typedef unsigned long long ullong;

ullong* compute_bitmask_array(int* binom_coef_count, int k) {
    
    ullong* result = NULL;
    
    int total = 0;
    for(int i = 0; i < k; i++) {
        total += binom_coef_count[i];
    }

    result = (ullong*) malloc(sizeof(ullong) * total);
    
    ullong bot32_mask = (1LL << 32) - 1;

    int c = 0;
    for(int i = 0; i < k; i++) {
        
        ullong current = 1LL << i;
        current = current | (current - 1);

        for(int j = 0; j < binom_coef_count[i]; j++) {
            result[c++] = (~current) << 1;
            ullong tmp = current | (current - 1); 
            int shift = 0;
            uint input = (uint) current;
            if ((bot32_mask & current) == 0) { 
                input = (uint) (current >> 32);
                shift = 32;
            } 
            shift += __builtin_ctz(input) + 1;
            
            current = (tmp + 1) | (((~tmp & -~tmp) - 1)) >> shift;
        } 
    
    }
    
    return result;
}

int* calculateBinomCoef(int k, int s) {
    
    int* result = (int*) malloc(sizeof(int) * k);

    for(int i = 0; i < k; i++) {
        result[i] = boost::math::binomial_coefficient<double>(s-1,i);
    }
    
    return result;
}

int** malloc_int_arrays(int* row_size, int k){
    
    int** result = (int**) malloc(sizeof(int*) * k);

    for(int i = 0; i < k; i++) {
        result[i] = (int*) malloc(sizeof(int) * row_size[i]);
    }
    
    return result;
}

void free_int_arrays(int**data, int k) {
    for(int i = 0; i < k; i++) {
        free(data[i]);
    }
}

void fill(int** data, int* row_size, int k, int value) {
    for(int i = 0; i < k; i++) {
        for(int j = 0; j < row_size[i]; j++) {
            data[i][j] = value;
        }
    }
}

void print_array(int* data, int size) {
    for(int i = 0; i < size; i++) {
        printf("%d, ", data[i]);
    }
    printf("size: %d\n", size);
}

void print2D(int** data, int* row_size, int k) {
    for(int i = 0; i < k; i++) {
        print_array(data[i], row_size[i]);
    }
}

int find_threshold(int* shape, int size, int m, int k) {
    
    if (m > 64 || size > m || k > size) {
        fprintf(stderr,"Invalid input arguments. shape size: %d, m: %d, k: %d\n", size, m, k);
        exit(-1);
    }

    //to include k
    k++;

    ullong shape_mask = 0;

    int min = shape[0];
    int max = 0;

    for(int i = 0; i < size; i++) {
        shape_mask |= (1LL << shape[i]);
        if (shape[i] > max) {
            max = shape[i];
        } else {
            if(shape[i] < min) {
                min = shape[i];
            }
        }
    }

    int span = max - min + 1;
    
    ullong lastElemMask = 1LL << (span - 1);

    int* bin_coef = calculateBinomCoef(k, span);

    int bin_coef_pref_sum[k];

    bin_coef_pref_sum[0] = bin_coef[0];

    for(int i = 1; i < k; i++) {
        bin_coef_pref_sum[i] = bin_coef[i] + bin_coef_pref_sum[i-1];
    }

    int array_size = bin_coef_pref_sum[k-1];

    //printf("AS: %d\n", array_size);

    ullong* bit_mask_array = compute_bitmask_array(bin_coef, k); 
    
    unordered_map<ullong, int> mask_to_index;

    for(int i = 0; i < array_size; i++) {
        mask_to_index[bit_mask_array[i]] = i;
    }

    int **current_result, **next_result;
    current_result = malloc_int_arrays(bin_coef_pref_sum, k); 
    next_result = malloc_int_arrays(bin_coef_pref_sum, k);
    
    fill(current_result, bin_coef_pref_sum, k, 0);

    ullong lastElemMaskNot = ~lastElemMask;

    for(int i = span; i < m; i++) {
        
        //TODO not sure about this...
        fill(next_result, bin_coef_pref_sum, k, 0);

        //print2D(current_result, bin_coef_pref_sum, k);
            
        int offset = 0;
        for(int j = 0; j < k; j++) {
            int end = bin_coef_pref_sum[j];//offset + bin_coef[j];

            for(int l = offset; l < end; l++) {
                ullong mask = bit_mask_array[l];
                int target_j = j;
                if ((mask & lastElemMask) != 0) {
                    target_j--;
                }
                if(target_j == -1) {
                    //TODO or continue to next value?
                    target_j = 0;
                }
                ullong target_mask = (mask & lastElemMaskNot) << 1;
                ullong target_mask2 = target_mask | 2LL;
                int mask_ind = mask_to_index[target_mask];
                int mask_ind2 = mask_to_index[target_mask2];
                
                if(mask_ind > bin_coef_pref_sum[target_j] || mask_ind2 > bin_coef_pref_sum[target_j]){
                    //printf("mask_ind: %d, mask_ind2: %d, max: %d\n", mask_ind, mask_ind2, bin_coef_pref_sum[target_j]);
                    //TODO what to do here?
                    continue;
                }
                int hit = (shape_mask & (mask | 1LL)) ? 1 : 0;
                next_result[j][l] = min(current_result[target_j][mask_ind2], current_result[target_j][mask_ind]) + hit;                                     
            }

            offset = end;
        }

        int** tmp = current_result;
        current_result = next_result;
        next_result = tmp;
    }
   
    int offset = 0;
    if(k > 1) 
        offset = bin_coef_pref_sum[k-2];

    int result = current_result[k-1][offset];

    for(int i = offset+1; i < array_size; i++) {
        if(current_result[k-1][i] < result) {
            result = current_result[k-1][i];
        }
    }

    free_int_arrays(current_result, k);
    free_int_arrays(next_result, k);
    free(bit_mask_array); 
    free(bin_coef);

    return result;
}

int main() {
    
    int shape[12] = {0,1,2,4,7,8,9,11,14,15,16,18};

    printf("result: %d\n", find_threshold(shape, 12, 50, 5));

    return 0;
}
