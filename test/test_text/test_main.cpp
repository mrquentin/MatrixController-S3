#include <unity.h>
#include "../FakeDisplay.h"
#include "display/Shapes.h"
#include "display/Text.h"

void setUp() {}
void tearDown() {}

void test_text_size() {
    TEST_ASSERT_EQUAL_UINT16(0, textWidth(""));
    // No spacing after the last character
    const uint16_t one = textWidth("A");
    TEST_ASSERT_EQUAL_UINT16(Fonts::Font5x7.glyphWidth, one);
    TEST_ASSERT_EQUAL_UINT16(2 * one + Fonts::Font5x7.spacing, textWidth("AB"));
    TEST_ASSERT_EQUAL_UINT16(2 * one, textWidth("A", Fonts::Font5x7, 2));
    TEST_ASSERT_EQUAL_UINT16(Fonts::Font5x7.glyphHeight, textHeight());
}

void test_draw_text_returns_next_position() {
    FakeDisplay display(64, 32);
    const int32_t next = drawText(display, 3, 0, "AB", Colors::White);
    TEST_ASSERT_EQUAL_INT32(3 + textWidth("AB") + Fonts::Font5x7.spacing, next);
    TEST_ASSERT_TRUE(display.litPixels() > 0);
}

void test_draw_text_clips_to_the_display() {
    FakeDisplay display(16, 8);
    // Partly off every edge, as when scrolling
    drawText(display, -4, -3, "Hello world", Colors::White);
    drawText(display, 10, 5, "Hello", Colors::White, Fonts::Font5x7, 3);
    TEST_ASSERT_EQUAL(0, display.outOfBounds);
}

void test_unknown_characters_draw_as_question_marks() {
    FakeDisplay unknown(8, 8);
    FakeDisplay question(8, 8);
    drawText(unknown, 0, 0, "\xC3", Colors::White);
    drawText(question, 0, 0, "?", Colors::White);
    TEST_ASSERT_TRUE(question.litPixels() > 0);
    TEST_ASSERT_EQUAL(question.litPixels(), unknown.litPixels());
}

void test_shapes_clip_and_hit_their_ends() {
    FakeDisplay display(16, 16);
    drawLine(display, 0, 0, 15, 15, Colors::Red);
    TEST_ASSERT_TRUE(display.at(0, 0) == Colors::Red);
    TEST_ASSERT_TRUE(display.at(15, 15) == Colors::Red);
    TEST_ASSERT_EQUAL(16, display.litPixels());

    // Partly off screen
    drawLine(display, -20, 3, 40, 3, Colors::Green);
    fillCircle(display, 15, 15, 10, Colors::Blue);
    drawCircle(display, -2, 8, 6, Colors::White);
    TEST_ASSERT_EQUAL(0, display.outOfBounds);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_text_size);
    RUN_TEST(test_draw_text_returns_next_position);
    RUN_TEST(test_draw_text_clips_to_the_display);
    RUN_TEST(test_unknown_characters_draw_as_question_marks);
    RUN_TEST(test_shapes_clip_and_hit_their_ends);
    return UNITY_END();
}
