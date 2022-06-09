#include <stdio.h>
#include "hl-mirage.h"

void heuristicTest(void (*fn)()) {
    printf("Heuristic test %p \n", fn);
    fn();
}
void heuristicTest2(float (*fn)(int)) {
    auto x = fn(5);
    printf("Heuristic test 2 %p %f\n", fn, x);
}