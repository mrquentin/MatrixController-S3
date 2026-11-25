#include <unity.h>
#include <string>
#include <ArduinoJson.h>
#include "apps/AppSettings.h"

void setUp() {}
void tearDown() {}

namespace {
    std::string upper(const std::string_view text) {
        std::string out(text);
        for (char& c : out) c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
        return out;
    }

    /// An app's settings, declared the way apps do
    struct Example {
        std::string message = "Hi";
        std::string code = "AB";
        int32_t speed = 40;
        bool seconds = false;
        Color color{1, 2, 3};
        std::string units = "metric";

        void settings(AppSettings& s) {
            s.text("message", "Message", message, 8);
            s.text("code", "Code", code, 2, upper);
            s.number("speed", "Speed", speed, 5, 200);
            s.toggle("seconds", "Show seconds", seconds);
            s.color("color", "Color", color);
            s.select("units", "Units", units, {{"metric", "Metric"}, {"imperial", "Imperial"}});
        }

        void apply(const char* json) {
            JsonDocument doc;
            deserializeJson(doc, json);
            AppSettings write = AppSettings::write(doc.as<JsonObjectConst>());
            settings(write);
        }
    };

    void assertColor(const Color expected, const Color actual) {
        TEST_ASSERT_EQUAL_HEX32(expected.toHex(), actual.toHex());
    }
}

void test_describe_lists_fields_for_the_form() {
    Example example;
    JsonDocument doc;
    AppSettings describe = AppSettings::describe(doc.to<JsonArray>());
    example.settings(describe);

    const JsonArray fields = doc.as<JsonArray>();
    TEST_ASSERT_EQUAL(6, fields.size());
    TEST_ASSERT_EQUAL_STRING("message", fields[0]["key"]);
    TEST_ASSERT_EQUAL_STRING("Message", fields[0]["label"]);
    TEST_ASSERT_EQUAL_STRING("text", fields[0]["type"]);
    TEST_ASSERT_EQUAL(8, fields[0]["maxLength"].as<int>());
    TEST_ASSERT_EQUAL_STRING("number", fields[2]["type"]);
    TEST_ASSERT_EQUAL(5, fields[2]["min"].as<int>());
    TEST_ASSERT_EQUAL(200, fields[2]["max"].as<int>());
    TEST_ASSERT_EQUAL_STRING("bool", fields[3]["type"]);
    TEST_ASSERT_EQUAL_STRING("color", fields[4]["type"]);
    TEST_ASSERT_EQUAL_STRING("select", fields[5]["type"]);
    TEST_ASSERT_EQUAL_STRING("imperial", fields[5]["options"][1]["value"]);
    TEST_ASSERT_EQUAL_STRING("Imperial", fields[5]["options"][1]["label"]);
}

void test_read_writes_current_values() {
    Example example;
    example.color = Color{0x12, 0xAB, 0x0F};
    JsonDocument doc;
    AppSettings read = AppSettings::read(doc.to<JsonObject>());
    example.settings(read);

    TEST_ASSERT_EQUAL_STRING("Hi", doc["message"]);
    TEST_ASSERT_EQUAL(40, doc["speed"].as<int>());
    TEST_ASSERT_FALSE(doc["seconds"].as<bool>());
    TEST_ASSERT_EQUAL_STRING("#12AB0F", doc["color"]);
    TEST_ASSERT_EQUAL_STRING("metric", doc["units"]);
}

void test_write_applies_new_values() {
    Example example;
    example.apply(R"({"message":"Hello","speed":60,"seconds":true,"color":"#FF8000","units":"imperial"})");
    TEST_ASSERT_EQUAL_STRING("Hello", example.message.c_str());
    TEST_ASSERT_EQUAL(60, example.speed);
    TEST_ASSERT_TRUE(example.seconds);
    assertColor(Color{255, 128, 0}, example.color);
    TEST_ASSERT_EQUAL_STRING("imperial", example.units.c_str());
}

void test_write_corrects_instead_of_refusing() {
    Example example;
    example.apply(R"({"message":"Much too long","code":"xyz","speed":500})");
    TEST_ASSERT_EQUAL_STRING("Much too", example.message.c_str());
    // Normalized first, then cut to the length
    TEST_ASSERT_EQUAL_STRING("XY", example.code.c_str());
    TEST_ASSERT_EQUAL(200, example.speed);
    example.apply(R"({"speed":-5})");
    TEST_ASSERT_EQUAL(5, example.speed);
}

void test_write_never_cuts_a_character_in_half() {
    Example example;
    // "Zürich" is 7 bytes: the ü takes two, and the limit of 8 falls inside the second "é"
    example.apply(R"({"message":"Zürich é"})");
    TEST_ASSERT_EQUAL_STRING("Zürich ", example.message.c_str());
}

void test_write_keeps_values_that_are_missing_or_wrong() {
    Example example;
    example.apply(R"({"speed":"50","seconds":"yes","units":"kelvin"})");
    TEST_ASSERT_EQUAL(40, example.speed);
    TEST_ASSERT_FALSE(example.seconds);
    TEST_ASSERT_EQUAL_STRING("metric", example.units.c_str());
    TEST_ASSERT_EQUAL_STRING("Hi", example.message.c_str());

    for (const char* bad : {R"({"color":"FF8000"})", R"({"color":"#FF80"})", R"({"color":"#FF80001"})",
                            R"({"color":"#GG0000"})", R"({"color":42})"}) {
        example.apply(bad);
        assertColor(Color{1, 2, 3}, example.color);
    }
}

void test_read_after_write_round_trips() {
    Example original;
    original.apply(R"({"message":"Round","color":"#12ab0f","units":"imperial","speed":77})");
    JsonDocument saved;
    AppSettings read = AppSettings::read(saved.to<JsonObject>());
    original.settings(read);

    std::string json;
    serializeJson(saved, json);
    Example restored;
    restored.apply(json.c_str());
    TEST_ASSERT_EQUAL_STRING("Round", restored.message.c_str());
    assertColor(Color{0x12, 0xAB, 0x0F}, restored.color);
    TEST_ASSERT_EQUAL_STRING("imperial", restored.units.c_str());
    TEST_ASSERT_EQUAL(77, restored.speed);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_describe_lists_fields_for_the_form);
    RUN_TEST(test_read_writes_current_values);
    RUN_TEST(test_write_applies_new_values);
    RUN_TEST(test_write_corrects_instead_of_refusing);
    RUN_TEST(test_write_never_cuts_a_character_in_half);
    RUN_TEST(test_write_keeps_values_that_are_missing_or_wrong);
    RUN_TEST(test_read_after_write_round_trips);
    return UNITY_END();
}
