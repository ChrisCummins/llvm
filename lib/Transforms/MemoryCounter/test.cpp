#include <cstdio>

int g = 0;

int foo() {
    // Perform 10 STOREs:
    g = 1; g = 2; g = 3; g = 4; g = 5;
    g = 6; g = 7; g = 7; g = 9; g = 10;

    // throw 0;

    // Perform 10 STOREs:
    g = 1; g = 2; g = 3; g = 4; g = 5;
    g = 6; g = 7; g = 7; g = 9; g = 10;

    return 0;
}

int main(int argc, char **argv) {
    // 3 STOREs.

    //try {
        foo();
    //} catch (...) {}

    // Perform 1 LOAD:
    std::printf("g = %d\n", g);
    return 0;
}
