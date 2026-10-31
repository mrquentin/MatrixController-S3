#include <unity.h>
#include "apps/F1Timing.h"

void setUp() {}
void tearDown() {}

void test_parse_iso_utc() {
    int64_t ms = 0;
    TEST_ASSERT_TRUE(F1::parseIsoUtc("1970-01-01T00:00:01Z", ms));
    TEST_ASSERT_EQUAL_INT64(1000, ms);
    TEST_ASSERT_TRUE(F1::parseIsoUtc("2024-09-01T13:05:12.345Z", ms));
    TEST_ASSERT_EQUAL_INT64(1725195912345LL, ms);
    // Fractions and the Z are optional
    TEST_ASSERT_TRUE(F1::parseIsoUtc("2024-09-01T13:05:12", ms));
    TEST_ASSERT_EQUAL_INT64(1725195912000LL, ms);
    TEST_ASSERT_FALSE(F1::parseIsoUtc("not a date", ms));
    TEST_ASSERT_FALSE(F1::parseIsoUtc(nullptr, ms));
}

void test_parse_duration() {
    int64_t ms = 0;
    TEST_ASSERT_TRUE(F1::parseDuration("01:23:45", ms));
    TEST_ASSERT_EQUAL_INT64(5025000, ms);
    TEST_ASSERT_TRUE(F1::parseDuration("23:45", ms));
    TEST_ASSERT_EQUAL_INT64(1425000, ms);
    TEST_ASSERT_TRUE(F1::parseDuration("01:23:45.678", ms));
    TEST_ASSERT_EQUAL_INT64(5025678, ms);
    TEST_ASSERT_FALSE(F1::parseDuration("45", ms));
    TEST_ASSERT_FALSE(F1::parseDuration("ab:cd", ms));
    TEST_ASSERT_FALSE(F1::parseDuration(nullptr, ms));
}

void test_format_clock() {
    char text[16];
    F1::formatClock(3723000, text, sizeof(text));
    TEST_ASSERT_EQUAL_STRING("1:02:03", text);
    F1::formatClock(754000, text, sizeof(text));
    TEST_ASSERT_EQUAL_STRING("12:34", text);
    // Rounds up: the clock only shows zero once time is really up
    F1::formatClock(1, text, sizeof(text));
    TEST_ASSERT_EQUAL_STRING("00:01", text);
    F1::formatClock(-500, text, sizeof(text));
    TEST_ASSERT_EQUAL_STRING("00:00", text);
}

void test_session_kind() {
    TEST_ASSERT_EQUAL(F1::SessionKind::Practice, F1::sessionKind("Practice", "Practice 2"));
    TEST_ASSERT_EQUAL(F1::SessionKind::Qualifying, F1::sessionKind("Qualifying", "Qualifying"));
    TEST_ASSERT_EQUAL(F1::SessionKind::SprintQualifying, F1::sessionKind("Qualifying", "Sprint Qualifying"));
    // 2023 name for sprint qualifying
    TEST_ASSERT_EQUAL(F1::SessionKind::SprintQualifying, F1::sessionKind("Race", "Sprint Shootout"));
    TEST_ASSERT_EQUAL(F1::SessionKind::Sprint, F1::sessionKind("Race", "Sprint"));
    TEST_ASSERT_EQUAL(F1::SessionKind::Race, F1::sessionKind("Race", "Race"));
    TEST_ASSERT_EQUAL(F1::SessionKind::Unknown, F1::sessionKind("Testing", "Day 1"));
}

void test_current_flag() {
    F1::LiveState state;
    state.trackStatus = 1;
    TEST_ASSERT_EQUAL(F1::Flag::Green, F1::currentFlag(state));
    state.trackStatus = 2;
    TEST_ASSERT_EQUAL(F1::Flag::Yellow, F1::currentFlag(state));
    state.doubleYellow = true;
    TEST_ASSERT_EQUAL(F1::Flag::DoubleYellow, F1::currentFlag(state));
    state.trackStatus = 5;
    TEST_ASSERT_EQUAL(F1::Flag::Red, F1::currentFlag(state));
    // The chequered flag outranks the track status
    state.sessionStatus = "Finished";
    TEST_ASSERT_EQUAL(F1::Flag::Chequered, F1::currentFlag(state));
}

void test_pit_messages() {
    F1::LiveState state;
    F1::applyPitMessage("PIT EXIT CLOSED", state);
    TEST_ASSERT_EQUAL(F1::PitLane::ExitClosed, F1::pitLane(state));
    // Announcements change nothing
    F1::applyPitMessage("PIT EXIT WILL OPEN AT 15:10", state);
    TEST_ASSERT_EQUAL(F1::PitLane::ExitClosed, F1::pitLane(state));
    F1::applyPitMessage("PIT ENTRY CLOSED", state);
    TEST_ASSERT_EQUAL(F1::PitLane::Closed, F1::pitLane(state));
    F1::applyPitMessage("GREEN LIGHT - PIT EXIT OPEN", state);
    TEST_ASSERT_EQUAL(F1::PitLane::EntryClosed, F1::pitLane(state));
    F1::applyPitMessage("PIT ENTRY OPEN", state);
    TEST_ASSERT_EQUAL(F1::PitLane::Open, F1::pitLane(state));
    F1::applyPitMessage("PIT LANE CLOSED", state);
    TEST_ASSERT_EQUAL(F1::PitLane::Closed, F1::pitLane(state));
    F1::applyPitMessage("PIT LANE OPEN", state);
    TEST_ASSERT_EQUAL(F1::PitLane::Open, F1::pitLane(state));
    // Unrelated messages mentioning the pits
    F1::applyPitMessage("CAR 4 (NOR) TIME 1:15.123 DELETED - PIT EXIT LINE CROSSED", state);
    TEST_ASSERT_EQUAL(F1::PitLane::Open, F1::pitLane(state));
}

void test_pit_lane() {
    F1::LiveState state;
    state.trackStatus = 1;
    TEST_ASSERT_EQUAL(F1::PitLane::Open, F1::pitLane(state));
    // Red flag: the pit exit is closed even without a message
    state.trackStatus = 5;
    TEST_ASSERT_EQUAL(F1::PitLane::ExitClosed, F1::pitLane(state));
    state.trackStatus = 1;
    state.sessionStatus = "Aborted";
    TEST_ASSERT_EQUAL(F1::PitLane::ExitClosed, F1::pitLane(state));
    // Nothing to show once the session is over
    state.pitEntryClosed = true;
    state.sessionStatus = "Finished";
    TEST_ASSERT_EQUAL(F1::PitLane::Open, F1::pitLane(state));
}

void test_parse_pit_messages() {
    JsonDocument doc;
    deserializeJson(doc, R"({"f1LiveTimingState": {
        "SessionInfo": {"Type": "Race", "Name": "Race"},
        "SessionStatus": {"Status": "Started"},
        "TrackStatus": {"Status": "4"},
        "RaceControlMessages": {"Messages": [
            {"Category": "SafetyCar", "Message": "SAFETY CAR DEPLOYED"},
            {"Category": "Other", "Message": "PIT ENTRY CLOSED"},
            {"Category": "Other", "Message": "PIT EXIT CLOSED"},
            {"Category": "Other", "Message": "PIT EXIT OPEN"}
        ]}
    }})");
    F1::LiveState state;
    TEST_ASSERT_TRUE(F1::parseLiveState(doc.as<JsonObjectConst>(), 0, 0, state));
    TEST_ASSERT_EQUAL(F1::PitLane::EntryClosed, F1::pitLane(state));
}

void test_query_always_has_messages() {
    TEST_ASSERT_NOT_EQUAL(std::string::npos, F1::buildQuery(nullptr).find("RaceControlMessages"));
    F1::LiveState state;
    state.kind = F1::SessionKind::Practice;
    state.trackStatus = 1;
    TEST_ASSERT_NOT_EQUAL(std::string::npos, F1::buildQuery(&state).find("RaceControlMessages"));
}

void test_remaining_time() {
    F1::LiveState state;
    TEST_ASSERT_EQUAL_INT64(-1, F1::remainingMs(state, 0));

    // Clock stopped: what was reported
    state.hasClock = true;
    state.remainingMs = 600000;
    TEST_ASSERT_EQUAL_INT64(600000, F1::remainingMs(state, 5000));

    // Clock running: counts down with track time, which moves on with the board's millis()
    state.extrapolating = true;
    state.clockUtcMs = 1000000;
    state.trackTimeMs = 1010000;
    state.fetchedAtMs = 2000;
    TEST_ASSERT_EQUAL_INT64(590000, F1::remainingMs(state, 2000));
    TEST_ASSERT_EQUAL_INT64(585000, F1::remainingMs(state, 7000));
    // Never below zero
    TEST_ASSERT_EQUAL_INT64(0, F1::remainingMs(state, 2000 + 3600000));
    // Paused replay: frozen
    state.clockPaused = true;
    TEST_ASSERT_EQUAL_INT64(590000, F1::remainingMs(state, 7000));
}

void test_short_meeting_name() {
    TEST_ASSERT_EQUAL_STRING("Italian GP", F1::shortMeetingName("Italian Grand Prix").c_str());
    TEST_ASSERT_EQUAL_STRING("Pre-Season Testing", F1::shortMeetingName("Pre-Season Testing").c_str());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_parse_iso_utc);
    RUN_TEST(test_parse_duration);
    RUN_TEST(test_format_clock);
    RUN_TEST(test_session_kind);
    RUN_TEST(test_current_flag);
    RUN_TEST(test_pit_messages);
    RUN_TEST(test_pit_lane);
    RUN_TEST(test_parse_pit_messages);
    RUN_TEST(test_query_always_has_messages);
    RUN_TEST(test_remaining_time);
    RUN_TEST(test_short_meeting_name);
    return UNITY_END();
}
