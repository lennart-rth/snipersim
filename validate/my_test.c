#define SIZE (1024 * 1024)
#define STRIDE 16

volatile int data[SIZE];

void _start() {
    int value;
    for (int i = 0 ; i < SIZE; i += STRIDE) {
        value = data[i];
        value += 10;
        value *= 20;
        value += 30;
        value *= 40;
        data[i] = value;
    }
}
