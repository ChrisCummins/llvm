#include <cstdio>

int foo() {
    return 0;
}

int main(int argc, char **argv) {
    std::printf("Hello, world!\n");

    int a = foo();

    return a;
}
