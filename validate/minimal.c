// minimal.c
int main() {
    // We point to arbitrary memory addresses. 
    // volatile ensures the compiler generates actual LOAD/STORE instructions.
    volatile int *ptr = (int*)0x10000;
    volatile int sink;

    // --- START TRACE ---
    
    // 1. LOAD A (Long Latency Event)
    // In LSC: Goes to B-Queue (or Load Queue)
    int a = ptr[0]; 

    // 2. ALU DEPENDENCY (The Stall)
    // Depends on 'a'. Must wait for Load A.
    // In LSC: Goes to A-Queue (Main Queue) and STALLS there.
    int b = a + 1;

    // 3. INDEPENDENT LOAD B (The Bypass)
    // Does NOT depend on 'a' or 'b'. 
    // In Standard Core: Blocked by 'b'.
    // In LSC: Goes to B-Queue and EXECUTES immediately.
    int c = ptr[1];

    // --- END TRACE ---

    return b + c; // Prevent dead code elimination
}
