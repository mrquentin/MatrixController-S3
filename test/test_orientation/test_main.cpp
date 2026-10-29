#include <unity.h>
#include "display/Orientation.h"

void setUp() {}
void tearDown() {}

void test_mode_names_round_trip() {
    for (const OrientationMode mode : {OrientationMode::Auto, OrientationMode::Upright, OrientationMode::UpsideDown}) {
        TEST_ASSERT_EQUAL(static_cast<int>(mode), static_cast<int>(parseOrientationMode(orientationModeName(mode))));
    }
    TEST_ASSERT_EQUAL(static_cast<int>(OrientationMode::Auto), static_cast<int>(parseOrientationMode("sideways")));
    TEST_ASSERT_EQUAL(static_cast<int>(OrientationMode::Upright),
                      static_cast<int>(parseOrientationMode("", OrientationMode::Upright)));
}

void test_upright_readings_keep_the_orientation() {
    OrientationTracker tracker;
    for (uint32_t now = 0; now < 2000; now += 100) {
        TEST_ASSERT_EQUAL(static_cast<int>(Orientation::Upright), static_cast<int>(tracker.update(1.0f, now)));
    }
}

void test_turning_over_flips_once_settled() {
    OrientationTracker tracker;
    tracker.update(-1.0f, 1000);
    tracker.update(-1.0f, 1000 + OrientationTracker::SettleMs - 100);
    TEST_ASSERT_EQUAL(static_cast<int>(Orientation::Upright), static_cast<int>(tracker.current()));
    tracker.update(-1.0f, 1000 + OrientationTracker::SettleMs);
    TEST_ASSERT_EQUAL(static_cast<int>(Orientation::UpsideDown), static_cast<int>(tracker.current()));

    // And back
    tracker.update(0.9f, 5000);
    tracker.update(0.9f, 5000 + OrientationTracker::SettleMs);
    TEST_ASSERT_EQUAL(static_cast<int>(Orientation::Upright), static_cast<int>(tracker.current()));
}

void test_a_bump_does_not_flip() {
    OrientationTracker tracker;
    tracker.update(-1.0f, 1000);
    // Back upright before settling: the next turn starts counting again
    tracker.update(1.0f, 1200);
    tracker.update(-1.0f, 1300);
    tracker.update(-1.0f, 1300 + OrientationTracker::SettleMs - 1);
    TEST_ASSERT_EQUAL(static_cast<int>(Orientation::Upright), static_cast<int>(tracker.current()));
}

void test_flat_or_portrait_keeps_the_orientation() {
    OrientationTracker tracker(Orientation::UpsideDown);
    // Lying flat or on its side: next to no gravity along the vertical axis
    for (uint32_t now = 0; now < 3000; now += 100) {
        tracker.update(now % 200 == 0 ? 0.3f : -0.3f, now);
    }
    TEST_ASSERT_EQUAL(static_cast<int>(Orientation::UpsideDown), static_cast<int>(tracker.current()));
}

void test_works_from_time_zero() {
    OrientationTracker tracker;
    tracker.update(-1.0f, 0);
    tracker.update(-1.0f, OrientationTracker::SettleMs);
    TEST_ASSERT_EQUAL(static_cast<int>(Orientation::UpsideDown), static_cast<int>(tracker.current()));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_mode_names_round_trip);
    RUN_TEST(test_upright_readings_keep_the_orientation);
    RUN_TEST(test_turning_over_flips_once_settled);
    RUN_TEST(test_a_bump_does_not_flip);
    RUN_TEST(test_flat_or_portrait_keeps_the_orientation);
    RUN_TEST(test_works_from_time_zero);
    return UNITY_END();
}
