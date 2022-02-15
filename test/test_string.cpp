#include <Arduino.h>
#include <unity.h>

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

void test_string_start_with(void) {
    String a, b, c, d;
    String command =  "fuse 0x84 0x99";
    TEST_ASSERT_TRUE(command.startsWith("fuse"));
    TEST_ASSERT_FALSE(command.startsWith("Fuse"));
    TEST_ASSERT_EQUAL(4, command.indexOf(" "));
    a = command.substring(4);
    TEST_ASSERT_EQUAL_STRING(" 0x84 0x99", a.c_str());
    a = " 0x84 0x99 ";
    a.trim();
    TEST_ASSERT_EQUAL_STRING("0x84 0x99", a.c_str());
    a = "0x84 0x99";
    TEST_ASSERT_EQUAL(4, a.indexOf(" "));
    a = "0x84 0x99";
    a = a.substring(0, 4);
    TEST_ASSERT_EQUAL_STRING("0x84", a.c_str());
    a = "0x84 0x99";
    a = a.substring(4);
    a.trim();
    TEST_ASSERT_EQUAL_STRING("0x99", a.c_str());

}

void test_string_to_int(void){
    String a = "84";
    int i = 0;
    TEST_ASSERT_EQUAL(84, a.toInt());

    a = "0x84";
    i = strtoul(a.c_str(), 0, 16);
    TEST_ASSERT_EQUAL(0x84, i);
}



void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_string_start_with);

    pinMode(LED_BUILTIN, OUTPUT);
}

uint8_t i = 0;
uint8_t max_blinks = 5;

void loop() {
    UNITY_END();
}
