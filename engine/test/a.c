#include "ctest/test_api.h"

int pow(int a, int b) {
    int r = 1;
    for (int _ = 0; _ < b; _++) {
        r *= a;
    }
    return r;
}

TEST(sum) {
    ASSERT_EQ(pow(3, 5), 243);
}


int main(void) {
    test_sum();
    return 0;
}