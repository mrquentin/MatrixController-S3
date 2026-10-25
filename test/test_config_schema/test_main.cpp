#include <unity.h>
#include <ArduinoJson.h>
#include "apps/ConfigSchema.h"

void setUp() {}
void tearDown() {}

static void assertColor(const Color expected, const Color actual) {
    TEST_ASSERT_EQUAL_UINT8(expected.r, actual.r);
    TEST_ASSERT_EQUAL_UINT8(expected.g, actual.g);
    TEST_ASSERT_EQUAL_UINT8(expected.b, actual.b);
}

void test_read_color_parses_hex() {
    JsonDocument doc;
    doc["color"] = "#FF8000";
    assertColor(Color{255, 128, 0}, readColor(doc["color"], Colors::Black));
}

void test_read_color_falls_back_on_bad_values() {
    const Color fallback{1, 2, 3};
    JsonDocument doc;
    for (const char* bad : {"FF8000", "#FF80", "#FF80001", "#GG0000", ""}) {
        doc["color"] = bad;
        assertColor(fallback, readColor(doc["color"], fallback));
    }
    doc["color"] = 42;
    assertColor(fallback, readColor(doc["color"], fallback));
    assertColor(fallback, readColor(doc["missing"], fallback));
}

void test_write_color_round_trips() {
    JsonDocument doc;
    const JsonObject config = doc.to<JsonObject>();
    writeColor(config, "color", Color{0x12, 0xAB, 0x0F});
    TEST_ASSERT_EQUAL_STRING("#12AB0F", config["color"]);
    assertColor(Color{0x12, 0xAB, 0x0F}, readColor(config["color"], Colors::Black));
}

void test_read_number_clamps_and_falls_back() {
    JsonDocument doc;
    doc["n"] = 50;
    TEST_ASSERT_EQUAL_INT32(50, readNumber(doc["n"], 7, 0, 100));
    doc["n"] = 500;
    TEST_ASSERT_EQUAL_INT32(100, readNumber(doc["n"], 7, 0, 100));
    doc["n"] = -5;
    TEST_ASSERT_EQUAL_INT32(0, readNumber(doc["n"], 7, 0, 100));
    doc["n"] = "50";
    TEST_ASSERT_EQUAL_INT32(7, readNumber(doc["n"], 7, 0, 100));
    TEST_ASSERT_EQUAL_INT32(7, readNumber(doc["missing"], 7, 0, 100));
}

void test_schema_describes_fields() {
    JsonDocument doc;
    ConfigSchema schema(doc.to<JsonArray>());
    schema.text("message", "Message", 32);
    schema.number("speed", "Speed", 1, 10);
    schema.toggle("seconds", "Show seconds");
    schema.select("units", "Units", {{"metric", "Metric"}, {"imperial", "Imperial"}});

    const JsonArray fields = doc.as<JsonArray>();
    TEST_ASSERT_EQUAL(4, fields.size());
    TEST_ASSERT_EQUAL_STRING("text", fields[0]["type"]);
    TEST_ASSERT_EQUAL(32, fields[0]["maxLength"].as<int>());
    TEST_ASSERT_EQUAL(1, fields[1]["min"].as<int>());
    TEST_ASSERT_EQUAL(10, fields[1]["max"].as<int>());
    TEST_ASSERT_EQUAL_STRING("bool", fields[2]["type"]);
    TEST_ASSERT_EQUAL_STRING("select", fields[3]["type"]);
    TEST_ASSERT_EQUAL(2, fields[3]["options"].size());
    TEST_ASSERT_EQUAL_STRING("imperial", fields[3]["options"][1]["value"]);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_read_color_parses_hex);
    RUN_TEST(test_read_color_falls_back_on_bad_values);
    RUN_TEST(test_write_color_round_trips);
    RUN_TEST(test_read_number_clamps_and_falls_back);
    RUN_TEST(test_schema_describes_fields);
    return UNITY_END();
}
