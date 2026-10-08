#ifndef TEST_API_H
#define TEST_API_H


#include <stdio.h>
#include <string.h>
#include <assert.h>

#define MAX_TESTS 1024

#define TEST_PASS 0
#define TEST_FAIL 1
typedef void (*test_fn)(void);

typedef struct TestCase {
    const char *name;
    test_fn fn;
    const char *file;
    int line;
} TestCase;



void test_fail(const char *file, int line, const char *msg);
void test_register(const char *name, test_fn fn, const char *file, int line);
int test_run_all(void);


#define ASSERT(expr) \
    do { \
        if (!(expr)) { \
            test_fail(__FILE__, __LINE__, "ASSERT( " #expr " )"); \
            return; \
        } \
    } while (0)

#define ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            test_fail(__FILE__, __LINE__, "ASSERT_EQ(" #a ", " #b ")"); \
            return; \
        } \
    } while (0)

#define ASSERT_EQ_STR(a, b) \
    do { \
        if (strcmp((a), (b)) != 0) { \
            test_fail(__FILE__, __LINE__, "ASSERT_EQ_STR(" #a ", " #b ")"); \
            return; \
        } \
    } while (0)

// #define ASSERT_EQ_U64

#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)
#define STR(str) #str

#define TEST(name) \
    static void CONCAT(test_, name)(void); \
    static void __attribute__((constructor)) CONCAT(register_, name)(void) { \
        test_register(STR(name), CONCAT(test_, name), __FILE__, __LINE__);\
    } \
    static void CONCAT(test_, name)(void)

#endif /* TEST_API_H */