#include <unity.h>
#include "ota/Version.h"

void setUp() {}
void tearDown() {}

void test_parse_release_and_build_versions() {
    Version::Semver version;
    TEST_ASSERT_TRUE(Version::parse("v1.2.3", version));
    TEST_ASSERT_EQUAL(1, version.major);
    TEST_ASSERT_EQUAL(2, version.minor);
    TEST_ASSERT_EQUAL(3, version.patch);
    TEST_ASSERT_TRUE(version.release);

    TEST_ASSERT_TRUE(Version::parse("10.20.30", version));
    TEST_ASSERT_EQUAL(20, version.minor);

    TEST_ASSERT_TRUE(Version::parse("v1.2.3-4-gabc1234", version));
    TEST_ASSERT_FALSE(version.release);
    TEST_ASSERT_TRUE(Version::parse("v1.2.3+20261025-1200", version));
    TEST_ASSERT_FALSE(version.release);
}

void test_parse_rejects_other_text() {
    Version::Semver version;
    TEST_ASSERT_FALSE(Version::parse("a25165f", version));
    TEST_ASSERT_FALSE(Version::parse("a25165f+20261025-1236", version));
    TEST_ASSERT_FALSE(Version::parse("v1.2", version));
    TEST_ASSERT_FALSE(Version::parse("v1..3", version));
    TEST_ASSERT_FALSE(Version::parse("", version));
    TEST_ASSERT_FALSE(Version::parse(nullptr, version));
}

void test_newer_releases() {
    TEST_ASSERT_TRUE(Version::isNewer("v1.0.1", "v1.0.0"));
    TEST_ASSERT_TRUE(Version::isNewer("v1.1.0", "v1.0.9"));
    TEST_ASSERT_TRUE(Version::isNewer("v2.0.0", "v1.99.99"));
    // Numbers, not text: 10 is more than 9
    TEST_ASSERT_TRUE(Version::isNewer("v1.10.0", "v1.9.0"));
}

void test_same_or_older_releases_are_not_offered() {
    TEST_ASSERT_FALSE(Version::isNewer("v1.0.0", "v1.0.0"));
    TEST_ASSERT_FALSE(Version::isNewer("v1.0.0", "v1.1.0"));
    TEST_ASSERT_FALSE(Version::isNewer("v1.9.0", "v1.10.0"));
}

void test_builds_after_a_release_already_contain_it() {
    TEST_ASSERT_FALSE(Version::isNewer("v1.0.0", "v1.0.0-3-gabc1234"));
    TEST_ASSERT_FALSE(Version::isNewer("v1.0.0", "v1.0.0+20261025-1200"));
    TEST_ASSERT_TRUE(Version::isNewer("v1.1.0", "v1.0.0-3-gabc1234"));
}

void test_builds_before_any_release_take_every_release() {
    TEST_ASSERT_TRUE(Version::isNewer("v1.0.0", "a25165f+20261025-1236"));
    TEST_ASSERT_TRUE(Version::isNewer("v1.0.0", "unknown"));
}

void test_candidates_must_be_releases() {
    TEST_ASSERT_FALSE(Version::isNewer("v2.0.0-beta", "v1.0.0"));
    TEST_ASSERT_FALSE(Version::isNewer("latest", "v1.0.0"));
    TEST_ASSERT_FALSE(Version::isNewer("", "v1.0.0"));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_parse_release_and_build_versions);
    RUN_TEST(test_parse_rejects_other_text);
    RUN_TEST(test_newer_releases);
    RUN_TEST(test_same_or_older_releases_are_not_offered);
    RUN_TEST(test_builds_after_a_release_already_contain_it);
    RUN_TEST(test_builds_before_any_release_take_every_release);
    RUN_TEST(test_candidates_must_be_releases);
    return UNITY_END();
}
