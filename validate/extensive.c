#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

// --- CONFIGURATION ---
// 1 Million integers = 4MB Data (Larger than most L2 caches, fits in L3)
#define ARRAY_SIZE (1024 * 1024) 
#define STRIDE 16 // Jump 64 bytes (1 cache line) to force L1 misses

// --- SNIPER MAGIC MARKERS ---
void SimRoiStart() { __asm__ __volatile__("xchg %bx, %bx"); }
void SimRoiEnd()   { __asm__ __volatile__("xchg %cx, %cx"); }

int main() {
    printf("[BENCH] Allocating Memory...\n");
    // Use volatile to force compiler to generate actual load instructions
    volatile int *data_src = (int*)malloc(ARRAY_SIZE * sizeof(int));
    volatile int *data_dst = (int*)malloc(ARRAY_SIZE * sizeof(int));

    // --- PHASE 1: DISK SIMULATION ---
    // We create a temporary file, write data, and read it back.
    // This fills the 'data_src' buffer using OS syscalls.
    printf("[BENCH] Simulating Disk I/O...\n");
    
    FILE *f = fopen("temp_bench.dat", "wb+");
    if (!f) { perror("File error"); return 1; }

    // Write deterministic random-ish data to file
    int dummy_val = 0;
    for (int i = 0; i < ARRAY_SIZE; i++) {
        dummy_val = (i * 37) % ARRAY_SIZE; // Psuedo-random pattern
        fwrite(&dummy_val, sizeof(int), 1, f);
    }
    
    // Rewind and Read into Memory (Simulates loading benchmark input)
    fseek(f, 0, SEEK_SET);
    fread((void*)data_src, sizeof(int), ARRAY_SIZE, f);
    fclose(f);
    unlink("temp_bench.dat"); // Delete temp file

    printf("[BENCH] Starting LSC Stress Test (ROI)...\n");

    // --- PHASE 2: THE LOAD SLICE STRESS LOOP ---
    // We want to see if the Independent Store/Load (B) can run 
    // while the Pointer Chase (A) is stalled.
    
    int ptr_index = 0;      // Dependency pointer
    int total_sum = 0;      // Accumulator

    SimRoiStart(); // <--- SNIPER STATS START HERE

    // We iterate with a STRIDE to hit new cache lines constantly
    for (int i = 0; i < ARRAY_SIZE / STRIDE; i+=STRIDE) {
        
        // --- QUEUE A TASK: The Pointer Chase (High Latency) ---
        // 1. Load: depends on 'ptr_index' from previous loop.
        // 2. This load likely misses L1/L2 because it jumps around randomly.
        int val_a = data_src[ptr_index]; 
        
        // 3. Dependency Calculation (The Stall)
        // This math effectively blocks the A-Queue.
        ptr_index = (val_a + i) % (ARRAY_SIZE - 1);
        if (ptr_index < 0) ptr_index = 0; // Safety

        // --- QUEUE B TASK: The Stream (Independent) ---
        // These instructions rely on 'i' (induction var), NOT 'ptr_index'.
        // Your IBDA should detect this slice and put it in Queue B.
        
        // 1. Independent Load
        int val_b = data_src[i]; 
        
        // 2. Independent Math
        int val_c = val_b + 1;

        // 3. Independent Store
        data_dst[i] = val_c;

        // Prevent Dead code elimination for A
        total_sum += val_a;
    }

    SimRoiEnd(); // <--- SNIPER STATS END HERE

    printf("[BENCH] Complete. Checksum: %d\n", total_sum);
    
    free((void*)data_src);
    free((void*)data_dst);
    return 0;
}