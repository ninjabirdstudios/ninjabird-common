/*/////////////////////////////////////////////////////////////////////////////
/// @summary Implements an interface to perform interactive, heirarchical
/// profiling of a program. The profiler is intrusive and requires manual
/// instrumentation. The implementation is based on a profile by Sean Barret
/// described in his column in Game Developer Magazine and available from:
/// http://silverspaceship.com/src/iprof/
/// @author Russell Klenk (russ@ninjabirdstudios.com)
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////
//   Includes   //
////////////////*/
#if   CMN_IS_APPLE
    #include <math.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <mach/mach_time.h>
#elif CMN_IS_LINUX
    #include <math.h>
    #include <time.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
#elif CMN_IS_WINDOWS
    #include <math.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <windows.h>
#endif

#include "libprofile.hpp"

/*//////////////////////////
//   Using Declarations   //
//////////////////////////*/

/*//////////////////////
//   Implementation   //
//////////////////////*/

/*/////////////////////////////////////////////////////////////////////////80*/

/// the size of the zone hash table - must be power-of-two.
#define HASH_TABLE_SIZE       2048
/// the number of calls to update before averages are computed.
#define THROWAWAY_COUNT       3
/// the frame time used when the frame delta is zero.
#define INITIAL_FRAME_TIME    0.001f
/// threshold for a moving average of an integer to be "zero".
#define INT_ZERO_THRESHOLD    0.25f
/// the tolerance in variance used when computing "hotness".
#define VARIANCE_TOLERANCE    0.5f
/// convert struct timespec time value to an absolute nanosecond time value.
#define TIMESPEC_TO_NS(tm)    (uint64_t(tm.tv_sec) * 1000000000 + tm.tv_nsec)

/*/////////////////////////////////////////////////////////////////////////80*/

/// the empty stack location.
profile::stack_t         Profile_Dummy_Stack        = {0};
/// the initial stack location.
profile::stack_t         Profile_Dummy_Stack2       = {0};
/// the current stack location.
profile::stack_t        *Profile_Stack              =  &Profile_Dummy_Stack2;
/// the global allocator instance.
static profile::alloc_t  Profile_Alloc              = {0};
/// number of items in the hash table.
static int32_t           Hash_Count                 =  1;
/// the current hash table index mask.
static int32_t           Hash_Mask                  =  0;
/// maximum number of items in the hash table.
static int32_t           Hash_Max                   =  1;
/// the hash table storage.
static profile::stack_t *Hash_Table[2048]           = {&Profile_Dummy_Stack};
/// the number of profile zones in use.
static int32_t           Zone_Count                 =  0;
/// the zone expanded in call-graph view.
static profile::zone_t  *Expanded_Zone              =  NULL;
/// the array of pointers to zones.
static profile::zone_t  *Zones[512]                 = {NULL};
/// the current reporting mode.
static int32_t           Report_Mode                = profile::REPORT_SELF_TIME;
/// the current recursion mode (no effect).
static int32_t           Recursion_Mode             = profile::RECURSION_FLATTEN;
/// zero-based index of cursor report record.
static size_t            Cursor_Index               =  0;
/// non-zero if cursor index has changed.
static size_t            Update_Cursor              =  0;
/// number of times the profile::update() function has been called.
static uint64_t          Update_Count               =  0;
/// index of the historical frame being viewed.
static size_t            Display_Frame              =  0;
/// index of the current history item.
static size_t            History_Index              =  0;
/// index into the filter window, in [0, 3).
static size_t            Smoothing_Factor           =  1;
/// accumulated self-times in a zone stack.
static uint64_t          Time_Accumulator           =  0;
/// a value used to convert ticks to seconds.
static float             Ticks_To_Seconds           =  0.0f;
/// the last update time, in seconds.
static float             Last_Time_In_Seconds       =  0.0f;
/// magical values of unknown origin (?).
static float             Time_To_90Percent[3]       = {0.1f, 0.8f, 2.5f};
/// not sure what these values do either.
static float             Factors[3]                 = {0};
/// zone history data; 256 KB.
static float             History[512][128];
/// values & variances for the current frame.
static profile::scalar_t Frame_Times;
/// a global profile report instance.
static profile::report_t Report ;
/// used when performing fast float->string.
static char              Int2Str[100][4];
/// used when performing fast float->string.
static char              Int2Str_Dec[100][4];
/// used when performing fast float->string.
static char              Int2Str_Mid[100][5];
/// format strings selected depending on the current precision setting.
static char const       *Format_Strings[5]  =
{
    "%.0f",
    "%.1f",
    "%.2f",
    "%.3f",
    "%.4f"
};

/*/////////////////////////////////////////////////////////////////////////80*/

static inline uint32_t zone_id(profile::zone_t *zone)
{
    uint32_t hash = 0x55555555;
    // compute a zone id based on the zone name
    // so that the id is consistent between runs.
    // this function is used when graphing data.
    if (zone != NULL)
    {
        char const *name  = zone->name;
        if (name != NULL)
        {
            while (*name)
            {
                hash = (hash << 5) + (hash >> 27) + *name++;
            }
        }
    }
    return hash;
}

/*/////////////////////////////////////////////////////////////////////////80*/

static inline uint32_t hash_zone(profile::zone_t *z, profile::stack_t *s)
{
    // compute a hash value based on a specific zone entry location.
    // this is the combination of a zone AND a stack.
    size_t n = ((size_t)z) + ((size_t)s);
    return uint32_t(n + (n >> 8));
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void initialize_zone(profile::zone_t *zone)
{
    // store a pointer to a zone in the table of zones and
    // mark the zone as initialized. when a zone is declared
    // it is declared as static, so this pointer remains
    // valid throughout the lifetime of the application.
    Zones[Zone_Count++] = zone;
    zone->initialized   = 1;
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void clear_history_scalar(profile::scalar_t *s)
{
    s->values[0]    = 0.0f;
    s->values[1]    = 0.0f;
    s->values[2]    = 0.0f;
    s->variances[0] = 0.0f;
    s->variances[1] = 0.0f;
    s->variances[2] = 0.0f;
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void update_history_scalar(
    profile::scalar_t *s,
    float              new_val,
    float             *history)
{
    // recompute history values and variances based
    // on the specified value for the current frame.
    size_t   h       = History_Index;
    float    k0      = history[0];
    float    k1      = history[1];
    float    k2      = history[3];
    float    new_var = new_val * new_val;
    s->values[0]    = s->values[0]    * k0 + new_val * (1.0f - k0);
    s->values[1]    = s->values[1]    * k1 + new_val * (1.0f - k1);
    s->values[2]    = s->values[2]    * k2 + new_val * (1.0f - k2);
    s->variances[0] = s->variances[0] * k0 + new_var * (1.0f - k0);
    s->variances[1] = s->variances[1] * k1 + new_var * (1.0f - k1);
    s->variances[2] = s->variances[2] * k2 + new_var * (1.0f - k2);
    s->history[h]   = new_val;
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void eternity_set_scalar(profile::scalar_t *s, float new_val)
{
    // sets all history values and variances to the same value.
    size_t   h       = History_Index;
    float    new_var = new_val * new_val;
    s->values[0]     = new_val;
    s->values[1]     = new_val;
    s->values[2]     = new_val;
    s->variances[0]  = new_var;
    s->variances[1]  = new_var;
    s->variances[2]  = new_var;
    s->history[h]    = new_val;
}

/*/////////////////////////////////////////////////////////////////////////80*/

static float history_value(profile::scalar_t *s)
{
    if (Display_Frame != 0)
    {
        // report a history value - never averaged.
        size_t n = (History_Index - Display_Frame + 128) % 128;
        return s->history[n];
    }
    // report the value for the current frame - possibly averaged.
    return s->values[Smoothing_Factor];
}

/*/////////////////////////////////////////////////////////////////////////80*/

static uint32_t count_recursion_depth(profile::stack_t *s, profile::zone_t *z)
{
    uint32_t n = 0;
    while  (s != NULL)
    {
        if (z == s->zone) ++n;
        s = s->parent;
    }
    return  n;
}

/*/////////////////////////////////////////////////////////////////////////80*/

static profile::stack_t* create_stack_node(
    profile::zone_t  *zone,
    profile::stack_t *parent)
{
    // this function gets called from the push_zone() function
    // and is the only function that allocates memory dynamically
    // allocation can happen on zone entry, so we can't eliminate
    // this dynamic allocation - but it shouldn't happen very often
    // (happens when a zone is entered from a particular location
    //  the very first time, and during recursion)
    profile::stack_t *stack = Profile_Alloc.alloc(
        sizeof(profile::stack_t),
        Profile_Alloc.context);
    stack->parent      = parent;
    stack->zone        = zone;
    stack->self_start  = 0;
    stack->self_ticks  = 0;
    stack->heir_ticks  = 0;
    stack->entry_count = 0;
    stack->entry_depth = count_recursion_depth(parent, zone);
    clear_history_scalar(&stack->history.self_time);
    clear_history_scalar(&stack->history.heir_time);
    clear_history_scalar(&stack->history.entry_count);
    stack->history.max_depth = 0;
    return stack;
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void propagate_stack_times(void)
{
    profile::stack_t *dummy = &Profile_Dummy_Stack;
    for (int32_t i = 0; i < Hash_Max; ++i)
    {
        if (Hash_Table[i] != dummy)
        {
            profile::stack_t *s = Hash_Table[i];
            profile::stack_t *p = Hash_Table[i];
            // propagate times up the stack for heirarchical
            // times, but take care to account for recursion
            while (p->zone != NULL)
            {
                if (!p->zone->visited)
                {
                    p->heir_ticks    += s->self_ticks;
                    p->zone->visited  = 1;
                }
                p = p->parent;
            }
            p = s;
            while (p->zone != NULL)
            {
                p->zone->visited = 0;
                p                = p->parent;
            }
        }
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void clear_stack_times(void)
{
    for (int32_t i = 0; i < Hash_Max; ++i)
    {
        Hash_Table[i]->self_ticks  = 0;
        Hash_Table[i]->heir_ticks  = 0;
        Hash_Table[i]->entry_count = 0;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void accumulate_stack_times(void)
{
    profile::stack_t *dummy = &Profile_Dummy_Stack;
    // accumulate self time values in order to determine
    // the total amount of time taken by all sections of
    // code currently being profiled.
    // this function is only called on the first update
    for (int32_t i = 0; i < Hash_Max; ++i)
    {
        if (Hash_Table[i] != dummy)
        {
            Time_Accumulator += Hash_Table[i]->self_ticks;
        }
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void update_stack_history(void)
{
    profile::stack_t *dummy = &Profile_Dummy_Stack;
    for (int32_t i = 0; i < Hash_Max; ++i)
    {
        if (Hash_Table[i] != dummy)
        {
            profile::stack_t   *stack       = Hash_Table[i];
            profile::history_t *history     = &stack->history;
            profile::zone_t    *zone        =  stack->zone;
            float               self_time   = 0.0f;
            float               heir_time   = 0.0f;
            float               entry_count = 0.0f; // averaged

            if (stack->entry_depth > history->max_depth)
            {
                history->max_depth = stack->entry_depth;
            }
            self_time   = stack->self_ticks * Ticks_To_Seconds;
            heir_time   = stack->heir_ticks * Ticks_To_Seconds;
            entry_count = (float) stack->entry_count;

            if (Update_Count < THROWAWAY_COUNT)
            {
                eternity_set_scalar(&history->self_time,   self_time);
                eternity_set_scalar(&history->heir_time,   heir_time);
                eternity_set_scalar(&history->entry_count, entry_count);
            }
            else
            {
                // @note: Factors set in profile::update().
                update_history_scalar(&history->self_time,   self_time,   Factors);
                update_history_scalar(&history->heir_time,   heir_time,   Factors);
                update_history_scalar(&history->entry_count, entry_count, Factors);
            }
            *zone->history += self_time; // updates a value in global History
        }
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void accumulate_times_to_zones(void)
{
    profile::stack_t *dummy = &Profile_Dummy_Stack;

    // accumulate times for each stack location to
    // their associated zone for reporting purposes
    // this function is called when creating a report
    // and the current view mode is NOT call-graph
    // there is one node associated with each zone
    for (int32_t i = 0; i < Hash_Max; ++i)
    {
        if (Hash_Table[i] != dummy)
        {
            float               t       = 0.0;
            profile::stack_t   *stack   = Hash_Table[i];
            profile::history_t *history = &stack->history;
            profile::zone_t    *zone    = stack->zone;
            profile::record_t  *records = zone->report_nodes;

            records->values[0] += 1000.0f * history_value(&history->self_time);
            records->values[1] += 1000.0f * history_value(&history->heir_time);
            records->values[2] += history_value(&history->entry_count);
            if (history_value(&history->entry_count) > INT_ZERO_THRESHOLD)
            {
                if (history->max_depth > records->max_depth)
                {
                    records->max_depth = history->max_depth;
                }
                if (stack->parent->zone != NULL)
                {
                    stack->parent->zone->report_nodes->prefix = '+';
                }
            }

            if (Display_Frame != 0)
            {
                // no variances when examining history
                continue;
            }

            if (profile::REPORT_HEIRARCHICAL_TIME == Report_Mode)
            {
                t = history->heir_time.variances[Smoothing_Factor];
            }
            else
            {
                t = history->self_time.variances[Smoothing_Factor];
            }

            t = 1000.0f * 1000.0f * t;
            if (0.0f == records->hotness)
            {
                records->hotness = t;
            }
            else
            {
                float h          = records->hotness;
                records->hotness = h + t * 2.0f * sqrtf(h * t);
            }
        }
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void accumulate_times_to_zones_expanded(void)
{
    profile::stack_t *dummy = &Profile_Dummy_Stack;
    // accumulate times for each stack location to
    // their associated zone for reporting purposes
    // this function is called when creating a report
    // and the current view mode is call-graph
    // there are three nodes associated with each zone
    for (int32_t i = 0; i < Hash_Max; ++i)
    {
        if (Hash_Table[i] != dummy)
        {
            profile::stack_t   *stack     = Hash_Table[i];
            profile::history_t *history   = &stack->history;
            float               self_val  = 0.0f;
            float               heir_val  = 0.0f;
            float               entry_val = 0.0f;

            self_val  = history_value(&history->self_time);
            heir_val  = history_value(&history->heir_time);
            entry_val = history_value(&history->entry_count);

            if (stack->parent->zone != NULL)
            {
                // this stack location is *not* top-of-stack
                if (history_value(&history->entry_count) > INT_ZERO_THRESHOLD)
                {
                    profile::zone_t   *zone    = stack->parent->zone;
                    profile::record_t *records = zone->report_nodes;
                    records[0].prefix = '+';
                    records[1].prefix = '+';
                    records[2].prefix = '+';
                }
            }
            if (stack->zone == Expanded_Zone)
            {
                // this stack location represents an entry
                // into the currently expanded zone
                profile::record_t *records = Expanded_Zone->report_nodes;

                // accumulate the time to ourselves.
                records[2].values[0]  += 1000.0f * self_val;
                records[2].values[1]  += 1000.0f * heir_val;
                records[2].values[2]  += entry_val;
                if (history->max_depth > records[2].max_depth)
                {
                    if (entry_val > INT_ZERO_THRESHOLD)
                    {
                        records[2].max_depth = history->max_depth;
                    }
                }

                // propagate the time to parent.
                if (stack->parent->zone != NULL)
                {
                    records = stack->parent->zone->report_nodes;
                    records[1].values[0]  += 1000.0f * self_val;
                    records[1].values[1]  += 1000.0f * heir_val;
                    records[1].values[2]  += entry_val;
                    if (history->max_depth > records[1].max_depth)
                    {
                        if (entry_val > INT_ZERO_THRESHOLD)
                        {
                            records[1].max_depth = history->max_depth;
                        }
                    }
                }
            }
            if (stack->parent->zone == Expanded_Zone)
            {
                // the parent stack location represents an
                // entry into the currently expanded zone
                profile::record_t *records = stack->zone->report_nodes;
                records[0].values[0]  += 1000.0f * self_val;
                records[0].values[1]  += 1000.0f * heir_val;
                records[0].values[2]  += entry_val;
                if (history->max_depth > records[0].max_depth)
                {
                    if (entry_val > INT_ZERO_THRESHOLD)
                    {
                        records[0].max_depth = history->max_depth;
                    }
                }
            }
        }
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

static float compute_hotness(float variance, float value)
{
    float factor = 0.0;
    float stddev = 0.0;
    float absval = fabsf(value);
    variance = variance - value * value;
    if (variance < 0.0f)  variance = 0.0f;
    stddev = sqrtf(variance);
    if  (absval < 0.000001f) return 0.0f;
    else factor = (stddev / absval) * (1.0f / VARIANCE_TOLERANCE);
    if  (factor < 0.0f) return 0.0f;
    if  (factor > 1.0f) return 1.0f;
    return factor;
}

/*/////////////////////////////////////////////////////////////////////////80*/

static int compare_records(void const *p, void const *q)
{
    // qsort callback function for sorting report
    // records when *not* in call-graph view mode
    profile::record_t *a = (profile::record_t*) p;
    profile::record_t *b = (profile::record_t*) q;
    float va = a->values[0];
    float vb = b->values[0];
    return (vb < va) ? -1 : ((vb > va) ? +1 : 0);
}

/*/////////////////////////////////////////////////////////////////////////80*/

static int compare_expanded(void const *p, void const *q)
{
    // qsort callback function for sorting report
    // records when in call-graph view mode
    profile::record_t *a = (profile::record_t*) p;
    profile::record_t *b = (profile::record_t*) q;
    if (a->indent != b->indent)
    {
        if (5 == a->indent) return -1;
        if (5 == b->indent) return +1;
        if (3 == a->indent) return +1;
        if (3 == b->indent) return -1;
        return 0;
    }
    if (a->values[1] == b->values[1]) return 0;
    if (a->values[1] <  b->values[1])
    {
        if (5 == a->indent) return -1;
        else                return +1;
    }
    if (5 == a->indent) return +1;
    else                return -1;
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void float_to_string(
    char   *buf,
    size_t  buf_size_in_bytes,
    float   num,
    int     precision)
{
    int x = 0;
    int y = 0;

    switch (precision)
    {
    case 2:
        {
            if (num < 0.0f || num >= 100.0f) break;
            x = (int)  num;
            y = (int) (num - x) * 100;
            strcpy(buf, Int2Str[x]);
            strcat(buf, Int2Str_Dec[y]);
        }
        return;

    case 3:
        {
            if (num < 0.0f || num >= 10.0f) break;
            num *=  10;
            x    = (int)  num;
            y    = (int) (num - x) * 100;
            strcpy(buf, Int2Str_Mid[x]);
            strcat(buf, Int2Str_Dec[y] + 1);
        }
        return;

    case 4:
        {
            if (num < 0.0f || num >= 1.0f) break;
            num    *=  100.0f;
            x       = (int)  num;
            y       = (int) (num - x) * 100;
            buf[0]  = '0';
            strcpy(buf + 1, Int2Str_Dec[x]);
            strcat(buf,     Int2Str_Dec[y] + 1);
        }
        return;
    }
    sprintf(buf, Format_Strings[precision], num);
}

/*/////////////////////////////////////////////////////////////////////////80*/

static profile::report_t* initialize_report(int32_t recordCount)
{
    profile::report_t *report = &Report;
    memset(report->title,   0, sizeof(report->title));
    memset(report->headers, 0, sizeof(report->headers));
    report->highlighted   = 0;
    report->record_count  = recordCount;
    for (int32_t i = 0; i < recordCount; ++i)
    {
        report->records[i].name        = NULL;
        report->records[i].zone        = NULL;
        report->records[i].max_depth   = 0;
        report->records[i].indent      = 0;
        report->records[i].value_flags = 0;
        report->records[i].hotness     = 0.0f;
        report->records[i].values[0]   = 0.0f;
        report->records[i].values[1]   = 0.0f;
        report->records[i].values[2]   = 0.0f;
        report->records[i].values[3]   = 0.0f;
        report->records[i].prefix      = 0;
    }
    return report;
}

/*/////////////////////////////////////////////////////////////////////////80*/

static profile::report_t* create_report(void)
{
    profile::report_t *report           = NULL;
    size_t             record_size      = sizeof(profile::record_t);
    char const        *displayed_name   = NULL;
    float              frame_time_avg   = 0.0;
    float              frames_per_sec   = 0.0;
    int32_t            i                = 0;
    int32_t            s                =
        (profile::REPORT_CALL_GRAPH == Report_Mode) ? 3 : 1;

    report = initialize_report(Zone_Count * s);
    for (i = 0; i < Zone_Count; ++i)
    {
        profile::zone_t   *zone    =  Zones[i];
        profile::record_t *records = &report->records[i * s];

        zone->report_nodes = records;
        if (profile::REPORT_CALL_GRAPH == Report_Mode)
        {
            records[0].name        = zone->name;
            records[0].zone        = zone;
            records[0].indent      = 3;
            records[0].value_flags = 1 | 2 | 4;
            records[0].prefix      = 0;
            records[1].name        = zone->name;
            records[1].zone        = zone;
            records[1].indent      = 5;
            records[1].value_flags = 1 | 2 | 4;
            records[1].prefix      = 0;
            records[2].name        = zone->name;
            records[2].zone        = zone;
            records[2].indent      = 0;
            records[2].value_flags = 1 | 2 | 4;
            records[2].prefix      = 0;
        }
        else
        {
            records[0].name        = zone->name;
            records[0].zone        = zone;
            records[0].indent      = 0;
            records[0].value_flags = 1 | 2 | 4;
            records[0].prefix      = 0;
        }
    }

    // compute average frame time (ms/frame)
    // compute frames-per-second
    frame_time_avg = Frame_Times.values[Smoothing_Factor];
    if (0.0f == frame_time_avg) frame_time_avg = 0.1f;
    frames_per_sec = 1.0f / frame_time_avg;

    // build the title string; for example
    // 12.201 ms/frame (fps: 81.96) SORT [SELF] - [CURRENT FRAME]
    displayed_name = "*ERROR*";
    switch (Report_Mode)
    {
    case profile::REPORT_SELF_TIME:
        displayed_name = "SORT [SELF]";
        break;

    case profile::REPORT_HEIRARCHICAL_TIME:
    case profile::REPORT_CALL_GRAPH:
        displayed_name = "SORT [HEIR]";
        break;
    }

    sprintf(
        report->title,
        "%3.3f ms/frame (fps: %3.2f)  %s",
        frame_time_avg * 1000.0f,
        frames_per_sec,
        displayed_name);

    if (Display_Frame != 0)
    {
        char   *end    = report->title + strlen(report->title);
        size_t  frame  = Display_Frame;
        sprintf(end, " - [%d FRAME(S) AGO]", (int) frame);
    }
    else
    {
        strcat(report->title, " - [CURRENT FRAME]");
    }

    // build the header strings - strings are constant
    // but the order of the strings may vary based on
    // the current report viewing mode
    strcpy(report->headers[0], "ZONE");
    strcpy(report->headers[3], "COUNT");
    if (profile::REPORT_HEIRARCHICAL_TIME == Report_Mode)
    {
        strcpy(report->headers[1], "HEIR");
        strcpy(report->headers[2], "SELF");
    }
    else
    {
        strcpy(report->headers[1], "SELF");
        strcpy(report->headers[2], "HEIR");
    }

    // accumulate times and sort all of the records
    // based on the current report viewing mode
    if (profile::REPORT_CALL_GRAPH == Report_Mode)
    {
        int32_t            j       = 0;
        profile::record_t *records = Expanded_Zone->report_nodes;

        accumulate_times_to_zones_expanded();
        records[2].prefix = '-';

        for (i = 0; i < report->record_count; ++i)
        {
            if (report->records[i].values[0] != 0.0f ||
                report->records[i].values[1] != 0.0f ||
                report->records[i].values[2] != 0.0f)
            {
                report->records[j] = report->records[i];
                ++j;
            }
        }
        report->record_count = j;
        qsort(report->records, report->record_count, record_size, compare_expanded);
        for (i = 0; i < report->record_count; ++i)
        {
            if (5 == report->records[i].indent)
            {
                report->records[i].indent = 3;
            }
        }
    }
    else
    {
        accumulate_times_to_zones();
        for (i = 0; i < Zone_Count; ++i)
        {
            profile::record_t *record = &report->records[i];

            if (profile::REPORT_HEIRARCHICAL_TIME == Report_Mode)
            {
                // swap self time and heirarchical time
                float temp = record->values[0];
                record->values[0] = record->values[1];
                record->values[1] = temp;
            }
            record->hotness = compute_hotness(record->hotness, record->values[0]);
        }
        qsort(report->records, report->record_count, record_size, compare_records);
    }

    // update the cursor index if necessary
    if (Update_Cursor)
    {
        for (i = 0; i < report->record_count; ++i)
        {
            if (report->records[i].zone == Expanded_Zone)
            {
                Cursor_Index = (size_t) i;
                break;
            }
        }
        Update_Cursor = 0;
    }

    // pin the cursor to the end of the viewable record range
    // the record the cursor is on becomes the highlighted line
    if (Cursor_Index >= (size_t) report->record_count)
    {
        if (0 == report->record_count)
        {
            Cursor_Index = (size_t) 0;
        }
        else
        {
            Cursor_Index = (size_t) report->record_count - 1;
        }
    }
    report->highlighted  = Cursor_Index;
    return report;
}

/*/////////////////////////////////////////////////////////////////////////80*/

static profile::stack_t* CMN_CALL_C libc_alloc(
    size_t  size_in_bytes,
    void   *context)
{
    CMN_UNUSED(context);
    return (profile::stack_t*) ::malloc(size_in_bytes);
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void CMN_CALL_C libc_free(
    profile::stack_t *item,
    size_t            size_in_bytes,
    void             *context)
{
    CMN_UNUSED(context);
    CMN_UNUSED(size_in_bytes);
    if (item != NULL) ::free(item);
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void CMN_CALL_C null_free(
    profile::stack_t *item,
    size_t            size_in_bytes,
    void             *context)
{
    CMN_UNUSED(item);
    CMN_UNUSED(size_in_bytes);
    CMN_UNUSED(context);
}

/*/////////////////////////////////////////////////////////////////////////80*/

static void fixup_allocator(profile::alloc_t *allocator)
{
    if (NULL == allocator->alloc && NULL == allocator->free)
    {
        allocator->alloc = libc_alloc;
        allocator->free  = libc_free;
    }
    if (NULL != allocator->alloc && NULL == allocator->free)
    {
        // free is a no-op, but we don't want to have to constantly NULL-check.
        allocator->free  = null_free;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

profile::stack_t* profile::push_zone(profile::zone_t *zone)
{
    uint32_t          hash  = hash_zone(zone, Profile_Stack);
    uint32_t          mask  = hash & Hash_Mask;
    uint32_t          shuf  = 0;
    profile::stack_t *stack = Hash_Table[mask];

    if (stack->parent == Profile_Stack && stack->zone == zone)
    {
        return stack;
    }
    if (stack != &Profile_Dummy_Stack)
    {
        // compute secondary hash function; force it to
        // be odd so it's relatively prime to table size
        shuf = ((hash << 4) + (hash >> 4)) | 1;

        // @note: guaranteed to terminate since the hash
        // table is guaranteed to never be totally full.
        for (;;)
        {
            mask  = (mask + shuf) & Hash_Mask;
            stack = Hash_Table[mask];
            if (stack->parent == Profile_Stack && stack->zone == zone)
            {
                return stack;
            }
            if (stack == &Profile_Dummy_Stack)
            {
                break;
            }
        }
    }
    // initialize the zone (and the rest of the system)
    // if initialization hasn't been performed yet.
    if (0 == zone->initialized)
    {
        initialize_zone(zone);
    }
    // insert the new entry into the hash table.
    Hash_Count       += 1;
    stack             = create_stack_node(zone, Profile_Stack);
    Hash_Table[mask]  = stack;
    return stack;
}

/*/////////////////////////////////////////////////////////////////////////80*/

float profile::current_time(void)
{
#if   CMN_IS_APPLE
    static bool                      _get_freq   = true;
    static mach_timebase_info_data_t _time_scale = {0};
    if (_get_freq)
    {
        _get_freq = false;
        (void) mach_timebase_info(&_time_scale);
    }
    uint64_t ns = mach_absolute_time() * _time_scale.numer / _time_scale.denom;
    return  (ns * 0.000000001f); // one billion nanoseconds in one second.
#elif CMN_IS_LINUX
    struct timespect ts   = {0};
    clock_gettime(CLOCK_REALTIME, &ts);
    uint64_t ns = TIMESPEC_TO_NS(ts);
    return  (ns * 0.000000001f); // one billion nanoseconds in one second.
#elif CMN_IS_WINDOWS
    static bool          _get_freq   = true;
    static LARGE_INTEGER _time_scale = {0};
    if (_get_freq)
    {
        _get_freq = false;
        (void) QueryPerformanceFrequency(&_time_scale);
        // convert 1 second (expressed as nanoseconds) to time units.
        _time_scale.QuadPart = 1000000000 / _time_scale.QuadPart;
    }
    LARGE_INTEGER qpc;
    QueryPerformanceCounter(&qpc);
    uint64_t ns = qpc.QuadPart * _time_scale.QuadPart;
    return  (ns * 0.000000001f); // one billion nanoseconds in one second.
#else
    #error No implementation of profile::current_time() for your platform!
#endif
}

/*/////////////////////////////////////////////////////////////////////////80*/

void profile::tick_count(int64_t *out_ticks)
{
#if   CMN_IS_APPLE
    static bool     _get_start = true;
    static uint64_t _start_tck = 0;
    // @note: the following is not thread-safe.
    if (_get_start)
    {
        // get the initial tick count so we start counting from zero.
        // this prevents issues due to timer wrap-around.
        _get_start = false;
        _start_tck = mach_absolute_time();
    }
    *out_ticks = mach_absolute_time() - _start_tck;
#elif CMN_IS_LINUX
    static bool            _get_start = true;
    static uint64_t        _start_tck =  0;
    static struct timespec _start_tms = {0};
    // @note: the following is not thread-safe.
    if (_get_start)
    {
        // get the initial tick count so we start counting from zero.
        // this prevents issues due to timer wrap around.
        _get_start = false;
        clock_gettime(CLOCK_REALTIME, &_start_tms);
        _start_tck = TIMESPEC_TO_NS(_start_tms);
    }
    struct timespec ts = {0};
    clock_gettime(CLOCK_REALTIME, &ts);
    *out_ticks = TIMESPEC_TO_NS(ts) - _start_tck;
#elif CMN_IS_WINDOWS
    static bool          _get_start = true;
    static LARGE_INTEGER _start_tck = {0};
    // @note: the following is not thread-safe.
    if (_get_start)
    {
        // get the initial tick count so we start counting from zero.
        // this prevents issues due to timer wrap-around.
        _get_start = false;
        QueryPerformanceCounter(&_start_tck);
    }
    LARGE_INTEGER qpc;
    QueryPerformanceCounter(&qpc);
    *out_ticks = qpc.QuadPart - _start_tck.QuadPart;
#else
    #error No implementation of profile::tick_count() for your platform!
#endif
}

/*/////////////////////////////////////////////////////////////////////////80*/

void profile::allocator_init(
    profile::alloc_t  *alloc,
    profile::alloc_fn  alloc_func,
    profile::free_fn   free_func, /* = NULL */
    void              *context    /* = NULL */)
{
    if (alloc != NULL)
    {
        alloc->alloc   = alloc_func;
        alloc->free    = free_func;
        alloc->context = context;
        fixup_allocator(alloc);
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

void profile::initialize(profile::alloc_t *allocator)
{
    if (allocator != NULL)
    {
        // copy the caller-supplied allocator.
        Profile_Alloc = *allocator;
    }
    else
    {
        // use the libc allocator.
        Profile_Alloc.alloc   = libc_alloc;
        Profile_Alloc.free    = libc_free;
        Profile_Alloc.context = NULL;
    }

    // initialize/reset global data.
    Profile_Stack             = &Profile_Dummy_Stack2;
    Zone_Count                = 0;
    Expanded_Zone             = NULL;
    Report_Mode               = profile::REPORT_HEIRARCHICAL_TIME;
    Recursion_Mode            = profile::RECURSION_FLATTEN;
    Cursor_Index              = 0;
    Update_Cursor             = 0;
    Update_Count              = 0;
    Display_Frame             = 0;
    History_Index             = 0;
    Smoothing_Factor          = 1;
    Time_Accumulator          = 0;
    Ticks_To_Seconds          = 0.0f;
    Last_Time_In_Seconds      = 0.0f;
    Time_To_90Percent[0]      = 0.1f;
    Time_To_90Percent[1]      = 0.8f;
    Time_To_90Percent[2]      = 2.5f;
    Factors[0]                = 0.0f;
    Factors[1]                = 0.0f;
    Factors[2]                = 0.0f;
    Frame_Times.values[0]     = INITIAL_FRAME_TIME;
    Frame_Times.values[1]     = INITIAL_FRAME_TIME;
    Frame_Times.values[2]     = INITIAL_FRAME_TIME;
    Frame_Times.variances[0]  = 0.0f;
    Frame_Times.variances[1]  = 0.0f;
    Frame_Times.variances[2]  = 0.0f;

    Hash_Max   = HASH_TABLE_SIZE;
    Hash_Mask  = HASH_TABLE_SIZE - 1;
    Hash_Count = 1;
    for (int32_t i = 0; i < HASH_TABLE_SIZE; ++i)
    {
        Hash_Table[i] = &Profile_Dummy_Stack;
    }
    for (int32_t i = 0; i < 512; ++i)
    {
        Zones[i] = NULL;
    }

    memset(&Profile_Dummy_Stack,  0, sizeof(Profile_Dummy_Stack));
    memcpy(&Profile_Dummy_Stack2, &Profile_Dummy_Stack, sizeof(Profile_Dummy_Stack));
    memset(History,               0, sizeof(History));

    for (int32_t i = 0; i < 100; ++i)
    {
        sprintf(Int2Str[i],     "%d",    i);
        sprintf(Int2Str_Dec[i], ".%02d", i);
        sprintf(Int2Str_Mid[i], "%d.%d", i / 10, i % 10);
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

void profile::shutdown(void)
{
    // free memory allocated for stack locations.
    Hash_Max   = HASH_TABLE_SIZE;
    Hash_Mask  = HASH_TABLE_SIZE - 1;
    Hash_Count = 1;
    for (size_t i = 0; i < HASH_TABLE_SIZE; ++i)
    {
        if (Hash_Table[i] != &Profile_Dummy_Stack)
        {
            if (Hash_Table[i] != NULL)
            {
                Profile_Alloc.free(
                    Hash_Table[i],
                    sizeof(profile::stack_t),
                    Profile_Alloc.context);
                Hash_Table[i] = &Profile_Dummy_Stack;
            }
        }
    }

    // zero the zone table.
    Zone_Count    = 0;
    Expanded_Zone = NULL;
    for (size_t i = 0; i < 512; ++i)
    {
        Zones[i]  = NULL;
    }

    // reset other state to safe values.
    Report_Mode    = profile::REPORT_HEIRARCHICAL_TIME;
    Recursion_Mode = profile::RECURSION_FLATTEN;
    Cursor_Index   = 0;
    Update_Cursor  = 0;
    Display_Frame  = 0;
    History_Index  = 0;
}

/*/////////////////////////////////////////////////////////////////////////80*/

int32_t profile::current_reporting_mode(void)
{
    return Report_Mode;
}

/*/////////////////////////////////////////////////////////////////////////80*/

int32_t profile::default_reporting_mode(void)
{
    return profile::REPORT_HEIRARCHICAL_TIME;
}

/*/////////////////////////////////////////////////////////////////////////80*/

int32_t profile::current_recursion_mode(void)
{
    return Recursion_Mode;
}

/*/////////////////////////////////////////////////////////////////////////80*/

int32_t profile::default_recursion_mode(void)
{
    return profile::RECURSION_FLATTEN;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t profile::current_smoothing_factor(void)
{
    return Smoothing_Factor;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t profile::default_smoothing_factor(void)
{
    return 1;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void profile::configure_report(
    int32_t reporting_mode,
    int32_t recursion_mode,
    size_t  smoothing_factor)
{
    if (smoothing_factor >= 3)
    {
        smoothing_factor  = 2;
    }
    Report_Mode      = reporting_mode;
    Recursion_Mode   = recursion_mode;
    Smoothing_Factor = smoothing_factor;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void profile::display_specific_tick(size_t tick_index)
{
    if (tick_index > 127) tick_index = 127;
    Display_Frame  = tick_index;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void profile::move_displayed_tick(int32_t tick_delta)
{
    profile::display_specific_tick(Display_Frame - tick_delta);
}

/*/////////////////////////////////////////////////////////////////////////80*/

void profile::set_cursor(size_t line_index)
{
    Cursor_Index = line_index;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void profile::move_cursor(int32_t line_delta)
{
    Cursor_Index += line_delta;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void profile::select_item(void)
{
    profile::report_t *report = create_report();
    if (report->highlighted  >= 0)
    {
        int32_t          n    = report->highlighted;
        profile::zone_t *zone = report->records[n].zone;
        if (zone != NULL)
        {
            Expanded_Zone = zone;
            Report_Mode   = profile::REPORT_CALL_GRAPH;
        }
    }
    Update_Cursor = 1;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void profile::select_parent(void)
{
    profile::zone_t   *old_zone = Expanded_Zone;
    profile::report_t *report   = create_report();
    for (int32_t i = 0; i < report->record_count; ++i)
    {
        if (0 == report->records[i].indent)      break;
        if (old_zone == report->records[i].zone) continue;
        Expanded_Zone = report->records[i].zone;
    }
    Update_Cursor = 1;
}

/*/////////////////////////////////////////////////////////////////////////80*/

void profile::update(int32_t update_mode)
{
    PROFILER_ENTER_SCOPE(_PROFILE_UPDATE_);

    static profile::scalar_t _timestamps_per_sec;
    static int64_t           _last_timestamp;
    static int64_t           _curr_timestamp;

    float   now   = 0.0; // current time, in seconds
    float   dt    = 0.0; // delta time, in seconds
    float   tsps  = 0.0; // timestamps per-second
    int64_t delta = 0;   // delta time, in ticks

    // accumulate times up the known stacks for all zones.
    propagate_stack_times();

    // compute the time delta (seconds) and use that to
    // pre-compute values used during history update
    now = profile::current_time();
    if (0 == Update_Count)
    {
        // first update; no delta - set to small non-zero value
        dt = INITIAL_FRAME_TIME;
    }
    else
    {
        // this function could be getting called very rapidly
        // or the timer could be very low-resolution, which
        // could cause the current and previous timestamp to
        // have the same value - if so, set to small value
        dt = now - Last_Time_In_Seconds;
        if (0.0f == dt)
        {
            dt = INITIAL_FRAME_TIME;
        }
    }
    Last_Time_In_Seconds = now;
    Factors[0]           = 0.0f; // instantaneous
    Factors[1]           = powf(0.1f, dt / Time_To_90Percent[1]);
    Factors[2]           = powf(0.1f, dt / Time_To_90Percent[2]);

    // compute the time delta (ticks) and use that
    // to update history values and variances
    profile::tick_count(&_curr_timestamp);
    if (0 == Update_Count)
    {
        // first update; no delta - sum all current tick counts
        Time_Accumulator = 0;
        accumulate_stack_times();
        if (0 == Time_Accumulator)
        {
            delta = 1;
        }
        else
        {
            delta = (int64_t) Time_Accumulator;
        }
    }
    else
    {
        // this function could be getting called very rapidly
        // or the timer could be very low-resolution, which
        // could cause the current and previous timestamp to
        // have the same value - if so, set to small value
        delta = _curr_timestamp - _last_timestamp;
        if (0 == delta)
        {
            delta = 1;
        }
    }
    _last_timestamp = _curr_timestamp;
    tsps            = (float) delta / dt;

    if (Update_Count < THROWAWAY_COUNT)
    {
        eternity_set_scalar(&_timestamps_per_sec, tsps);
    }
    else
    {
        update_history_scalar(&_timestamps_per_sec, tsps, Factors);
    }

    if (profile::UPDATE_DISCARD == update_mode)
    {
        clear_stack_times();
        return;
    }

    if (tsps != 0.0f) Ticks_To_Seconds = 1.0f / tsps;
    else              Ticks_To_Seconds = 0.0f;
    for (int32_t i = 0; i < Zone_Count; ++i)
    {
        Zones[i]->history = &History[i][History_Index];
        History[i][History_Index] = 0.0f;
    }

    update_stack_history();
    update_history_scalar(&Frame_Times, dt, Factors);

    Update_Count += 1;
    History_Index = (History_Index + 1) % 128;
    clear_stack_times();
}

/*/////////////////////////////////////////////////////////////////////////80*/

void profile::render_report(
    float                    origin_x,
    float                    origin_y,
    float                    width,
    float                    height,
    float                    line_spacing,
    size_t                   precision,
    profile::render_text_fn  render_func,
    profile::text_width_fn   measure_func,
    void                    *context)
{
    PROFILER_ENTER_SCOPE(_PROFILE_REPORT_);

    void                 *c                = context;
    profile::report_t    *report           = NULL;
    profile::text_item_t  ctx              = {0};
    float                 sx               = origin_x;
    float                 sy               = origin_y;
    float                 field_width      = measure_func("5555.55", c);
    float                 name_width       = width - field_width * 3;
    float                 plus_width       = measure_func("+", c);
    int32_t               i                = 0;
    int32_t               j                = 0;
    int32_t               n                = 0;
    int32_t               o                = 0;
    int32_t               max_records      = 0;
    int32_t               highlighted_item = 0;

    // clamp the precision value into the valid range.
    if (precision < 1) precision = 1;
    if (precision > 4) precision = 4;

    // create a report object holding the current snapshot.
    report = create_report();

    // render the title string.
    ctx.hotness      = 0;
    ctx.title        = 1;
    ctx.header       = 0;
    ctx.highlighted  = 0;
    ctx.user_data    = c;
    render_func(sx + 2, sy, report->title, &ctx);
    sy     += 1.5f * line_spacing;
    height -= 1.5f * fabsf(line_spacing);

    // determine the number of records that will fit on-screen
    // render any zone record entries that are visible.
    highlighted_item = report->highlighted;
    max_records      = (int) height / (int) fabsf(line_spacing);
    o                = 0;
    n                = (int) report->record_count;
    if (n > max_records)           n = max_records;
    if (highlighted_item >= o + n) o = highlighted_item - n + 1;

    // render the header strings.
    ctx.hotness     = 0.0f;
    ctx.title       = 0;
    ctx.header      = 1;
    ctx.highlighted = 0;
    ctx.user_data   = c;
    render_func(sx + 8, sy, report->headers[0], &ctx);
    for (j = 1;  j < 5; ++j)
    {
        ctx.hotness     = 0.0f;
        ctx.title       = 0;
        ctx.header      = 1;
        ctx.highlighted = 0;
        render_func(
            sx + name_width + field_width * (j - 1) +
            field_width / 2 - measure_func(report->headers[j], c) / 2,
            sy,
            report->headers[j],
            &ctx);
    }

    sy += line_spacing;

    for (i = 0; i < n; ++i)
    {
        profile::record_t *record   = &report->records[i + o];
        char               buf[256] = {0};
        char              *b        = buf;
        float              x        = 0.0;

        x = sx + measure_func(" ", c) * record->indent + plus_width / 2;

        if (record->prefix != 0)
        {
            buf[0] = record->prefix;
            ++b;
        }
        else x += plus_width;
        if (record->max_depth != 0) sprintf(b, "%s (%d)", record->name, record->max_depth);
        else                        sprintf(b, "%s",      record->name);

        ctx.hotness     = record->hotness;
        ctx.title       = 0;
        ctx.header      = 0;
        ctx.highlighted = (highlighted_item == i + o) ? 1 : 0;
        ctx.user_data   = c;
        render_func(x + 1, sy, buf, &ctx);

        for (j = 0; j < 4; ++j)
        {
            if (record->value_flags & (1 << j))
            {
                float pad = 0.0;
                float val = (float) record->values[j];

                float_to_string(buf, 255, val, (j == 2) ? 2 : precision);

                pad = field_width - plus_width - measure_func(buf, c);
                if (record->indent != 0)
                {
                    pad += plus_width;
                }

                ctx.user_data = c;
                render_func(
                    sx + pad + name_width + field_width * j,
                    sy,  buf, &ctx);
            }
        }

        sy += line_spacing;
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

void profile::render_graph(
    float                     origin_x,
    float                     origin_y,
    float                     spacing_x,
    float                     spacing_y,
    profile::render_graph_fn  render_func,
    void                     *context)
{
    PROFILER_ENTER_SCOPE(_PROFILE_GRAPH_);

    profile::graph_item_t ctx;
    int                   n = 128;
    int                   h = History_Index;
    int                   i = 0;

    ctx.origin_x  = origin_x;
    ctx.origin_y  = origin_y;
    ctx.spacing_x = spacing_x;
    ctx.spacing_y = spacing_y;
    ctx.user_data = context;

    for (i = 0; i < Zone_Count; ++i)
    {
        uint32_t id = zone_id(Zones[i]);

        if (h >= n)
        {
            render_func(id, 0, n, &History[i][h - n], &ctx);
        }
        else
        {
            render_func(id, n - h, n,     &History[i][0],           &ctx);
            render_func(id, 0,     n - h, &History[i][n - (n - h)], &ctx);
        }
    }

    if (Display_Frame != 0)
    {
        float values[2] = {2.0f, 0.0f};
        int   x0        = n - 1 - Display_Frame;
        int   x1        = n - 1 - Display_Frame;
        render_func(0, x0, x1, values, &ctx);
    }
}

/*/////////////////////////////////////////////////////////////////////////80*/

/*/////////////////////////////////////////////////////////////////////////////
//    $Id$
///////////////////////////////////////////////////////////////////////////80*/
