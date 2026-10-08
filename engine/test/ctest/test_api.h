#ifndef TEST_API_H
#define TEST_API_H


#include <stdio.h>
#include <assert.h>

#define MAX_TESTS 1024


typedef void (*test_fn)(void);

typedef struct TestCase {
    const char *name;
    test_fn fn;
    const char *file;
    int line;
} TestCase;

static TestCase g_tests[MAX_TESTS];
static size_t g_ntest = 0;



void test_register(const char *name, test_fn fn, const char *file, int line);



#define ASSERT(expr) \
    do { \
        if (!(expr)) { \
            fprintf(stderr, "[ASSERT FAILED] \nexpr:(%s) \nfile: %s \nline: %d \n", \
                    #expr, __FILE__, __LINE__); \
            return; \
        } \
    } while (0)

#define ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            fprintf(stderr, "[ASSERT EQUAL FAILED] \nexpr:(%s == %s) \nfile: %s \nline: %d \n", \
                    #a, #b, __FILE__, __LINE__); \
        } \
    } while (0)


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