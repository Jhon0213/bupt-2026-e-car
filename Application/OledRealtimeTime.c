#include "Application/OledRealtimeTime.h"

#include "Hardware/Diagnostics/ControlTimingDiag.h"
#include "Hardware/OledDisplay.h"
#include "Public/Board/board.h"

#define OLED_REALTIME_TIME_LINE          4U
#define OLED_REALTIME_TIME_COL           6U
#define OLED_REALTIME_TIME_LEN           3U
#define OLED_REALTIME_TIME_MAX_SECONDS   999U
#define OLED_REALTIME_TIME_GLYPH_COLUMNS 6U
#define OLED_REALTIME_TIME_COLUMNS_STEP  2U
#define OLED_REALTIME_TIME_INVALID_INDEX 0xFFU

typedef struct
{
    uint32_t start_ms;
    uint32_t last_requested_second;
    char desired_text[OLED_REALTIME_TIME_LEN + 1U];
    char displayed_text[OLED_REALTIME_TIME_LEN + 1U];
    uint8_t enabled;
    uint8_t dirty_search_index;
    uint8_t active_dirty_index;
    uint8_t active_column_offset;
} OledRealtimeTimeContext;

static OledRealtimeTimeContext g_realtime_time;

static void FormatSeconds(uint32_t elapsed_seconds, char *text)
{
    if (elapsed_seconds > OLED_REALTIME_TIME_MAX_SECONDS)
    {
        elapsed_seconds = OLED_REALTIME_TIME_MAX_SECONDS;
    }

    text[0] = (char)('0' + ((elapsed_seconds / 100U) % 10U));
    text[1] = (char)('0' + ((elapsed_seconds / 10U) % 10U));
    text[2] = (char)('0' + (elapsed_seconds % 10U));
    text[3] = '\0';
}

static uint8_t FindNextDirtyIndex(void)
{
    uint8_t checked;
    uint8_t index;

    for (checked = 0U; checked < OLED_REALTIME_TIME_LEN; checked++)
    {
        index = (uint8_t)((g_realtime_time.dirty_search_index + checked) %
                          OLED_REALTIME_TIME_LEN);
        if (g_realtime_time.desired_text[index] !=
            g_realtime_time.displayed_text[index])
        {
            return index;
        }
    }

    return OLED_REALTIME_TIME_INVALID_INDEX;
}

void OledRealtimeTime_Init(uint32_t start_ms)
{
    uint8_t i;

    g_realtime_time.start_ms = start_ms;
    g_realtime_time.last_requested_second = 0xFFFFFFFFUL;
    g_realtime_time.enabled = 1U;
    g_realtime_time.dirty_search_index = 0U;
    g_realtime_time.active_dirty_index = OLED_REALTIME_TIME_INVALID_INDEX;
    g_realtime_time.active_column_offset = 0U;

    FormatSeconds(0U, g_realtime_time.desired_text);
    for (i = 0U; i < OLED_REALTIME_TIME_LEN; i++)
    {
        g_realtime_time.displayed_text[i] = g_realtime_time.desired_text[i];
    }
    g_realtime_time.displayed_text[OLED_REALTIME_TIME_LEN] = '\0';
}

void OledRealtimeTime_Request(uint32_t now_ms)
{
    uint32_t elapsed_ms;
    uint32_t elapsed_seconds;

    if (g_realtime_time.enabled == 0U)
    {
        return;
    }

    elapsed_ms = now_ms - g_realtime_time.start_ms;
    elapsed_seconds = elapsed_ms / 1000U;
    if (elapsed_seconds == g_realtime_time.last_requested_second)
    {
        return;
    }

    g_realtime_time.last_requested_second = elapsed_seconds;
    FormatSeconds(elapsed_seconds, g_realtime_time.desired_text);
}

void OledRealtimeTime_ProcessOneStep(void)
{
    uint8_t index;
    uint8_t column_count;
    uint8_t column_start;
    uint32_t start_ms;

    if (g_realtime_time.enabled == 0U)
    {
        return;
    }

    if (g_realtime_time.active_dirty_index == OLED_REALTIME_TIME_INVALID_INDEX)
    {
        index = FindNextDirtyIndex();
        if (index == OLED_REALTIME_TIME_INVALID_INDEX)
        {
            return;
        }

        g_realtime_time.active_dirty_index = index;
        g_realtime_time.active_column_offset = 0U;
        OledDisplay_WriteChar(OLED_REALTIME_TIME_LINE,
                              (uint8_t)(OLED_REALTIME_TIME_COL + index),
                              g_realtime_time.desired_text[index]);
    }

    index = g_realtime_time.active_dirty_index;
    column_count = OLED_REALTIME_TIME_COLUMNS_STEP;
    if ((uint8_t)(g_realtime_time.active_column_offset + column_count) >
        OLED_REALTIME_TIME_GLYPH_COLUMNS)
    {
        column_count = (uint8_t)(OLED_REALTIME_TIME_GLYPH_COLUMNS -
                                 g_realtime_time.active_column_offset);
    }

    column_start = (uint8_t)(((OLED_REALTIME_TIME_COL + index) *
                              OLED_REALTIME_TIME_GLYPH_COLUMNS) +
                             g_realtime_time.active_column_offset);
    start_ms = board_millis();
    OledDisplay_UpdateRegion(OLED_REALTIME_TIME_LINE,
                             column_start,
                             (uint8_t)(column_start + column_count - 1U));
    ControlTimingDiag_RecordOledPartialDuration(board_millis() - start_ms);

    g_realtime_time.active_column_offset =
        (uint8_t)(g_realtime_time.active_column_offset + column_count);
    if (g_realtime_time.active_column_offset >= OLED_REALTIME_TIME_GLYPH_COLUMNS)
    {
        g_realtime_time.displayed_text[index] =
            g_realtime_time.desired_text[index];
        g_realtime_time.active_dirty_index = OLED_REALTIME_TIME_INVALID_INDEX;
        g_realtime_time.active_column_offset = 0U;
        g_realtime_time.dirty_search_index =
            (uint8_t)((index + 1U) % OLED_REALTIME_TIME_LEN);
    }
}

void OledRealtimeTime_ForceFinal(uint32_t elapsed_ms)
{
    uint8_t i;

    FormatSeconds(elapsed_ms / 1000U, g_realtime_time.desired_text);
    for (i = 0U; i < OLED_REALTIME_TIME_LEN; i++)
    {
        g_realtime_time.displayed_text[i] = g_realtime_time.desired_text[i];
        OledDisplay_WriteChar(OLED_REALTIME_TIME_LINE,
                              (uint8_t)(OLED_REALTIME_TIME_COL + i),
                              g_realtime_time.displayed_text[i]);
    }
    g_realtime_time.active_dirty_index = OLED_REALTIME_TIME_INVALID_INDEX;
    g_realtime_time.active_column_offset = 0U;
}

void OledRealtimeTime_Reset(void)
{
    g_realtime_time.enabled = 0U;
    g_realtime_time.start_ms = 0U;
    g_realtime_time.last_requested_second = 0xFFFFFFFFUL;
    g_realtime_time.dirty_search_index = 0U;
    g_realtime_time.active_dirty_index = OLED_REALTIME_TIME_INVALID_INDEX;
    g_realtime_time.active_column_offset = 0U;
}