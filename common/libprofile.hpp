/*/////////////////////////////////////////////////////////////////////////////
///
///  @file: libprofile.hpp
///  Defines an interface to perform interactive, heirarchical profiling of
///  a program. The profiler is intrusive and requires manual instrumentation.
///  The implementation is based on a profiler by Sean Barret described in his
///  column in Game Developer Magazine and available from:
///  http://silverspaceship.com/src/iprof/
///
///////////////////////////////////////////////////////////////////////////80*/

#ifndef LIBPROFILE_HPP_INCLUDED
#define LIBPROFILE_HPP_INCLUDED

/*////////////////
//   Includes   //
////////////////*/
#include "common.hpp"

/*///////////////////////
//   Namespace Begin   //
///////////////////////*/
namespace profile {

/*////////////////////////////
//   Forward Declarations   //
////////////////////////////*/
struct zone_t;
struct scope_t;
struct stack_t;
struct record_t;
struct report_t;
struct scalar_t;
struct history_t;
struct text_item_t;
struct graph_item_t;
struct stack_t* push_zone(struct zone_t*);
extern float    current_time(void);
extern void     tick_count(int64_t*);

/*//////////////////////////////////
//   Public Types and Functions   //
//////////////////////////////////*/
/* INTERNAL. DO NOT USE. */
#define PROFILER_BEGIN_DATA(zone)                                             \
    /* declare a static cache of the zone stack */                            \
    static profile::stack_t *Profile_Cache_Stack = &Profile_Dummy_Stack

/* INTERNAL. DO NOT USE. */
#define PROFILER_BEGIN_RAW(zone)                                              \
    PROFILER_BEGIN_DATA(zone);                                                \
    PROFILER_BEGIN_CODE(zone)

/* INTERNAL. DO NOT USE. */
#define PROFILER_BEGIN_CODE(zone)                                             \
    (                                                                         \
    /* check the cached stack and update if needed */                         \
    (Profile_Cache_Stack->parent != Profile_Stack    ?                        \
     Profile_Cache_Stack = profile::push_zone(&zone) :                        \
     0),                                                                      \
    ++Profile_Cache_Stack->entry_count,                                       \
    profile::tick_count(&Profile_Time),                                       \
    /* stop the timer on the parent zone stack */                             \
    (Profile_Stack->self_ticks += Profile_Time - Profile_Stack->self_start),  \
    /* make the cached stack current */                                       \
    Profile_Stack = Profile_Cache_Stack,                                      \
    Profile_Stack->self_start = Profile_Time,                                 \
    0)

/* INTERNAL. DO NOT USE. */
#define PROFILER_END_RAW()                                                    \
    (profile::tick_count(&Profile_Time),                                      \
     /* stop the timer for the current zone stack */                          \
     Profile_Stack->self_ticks += Profile_Time - Profile_Stack->self_start,   \
     /* make the parent chain current */                                      \
     Profile_Stack = Profile_Stack->parent,                                   \
     /* start the timer for the parent zone stack */                          \
     Profile_Stack->self_start = Profile_Time);

#define PROFILER_DECLARE_ZONE(zone)                                           \
    profile::zone_t Profile_Zone_##zone

#define PROFILER_DEFINE_ZONE(zone)                                            \
    PROFILER_DECLARE_ZONE(zone) = { #zone }

/* INTERNAL. DO NOT USE. */
#define PROFILER_REGION(zone)                                                 \
    PROFILER_BEGIN_RAW(Profile_Zone_##zone);

#define PROFILER_DECLARE_SCOPE(zone)                                          \
    PROFILER_BEGIN_DATA(zone);                                                \
    profile::scope_t _scope_var(Profile_Zone_##zone, Profile_Cache_Stack)

/// Use this macro to define and enter a profiling zone. The macro parameter is
/// the name of the zone as it will appear in the profile report. Each instance
/// of PROFILER_ENTER_ZONE must be matched with a PROFILER_LEAVE_ZONE for each
/// point at which the enclosing scope will be exited. No semicolon should
/// appear at the end of the macro. This macro is typically used from C only.
#define PROFILER_ENTER_ZONE(zone)                                             \
    static PROFILER_DEFINE_ZONE(zone);                                        \
    PROFILER_REGION(zone)

/// Use this macro to exit the current profiling zone. This macro has no
/// parameter list, and no semicolon should appear at the end of the macro.
/// This macro is typically used from C only.
#define PROFILER_LEAVE_ZONE                                                   \
    PROFILER_END_RAW();

/// Use this macro to define and enter a profiling zone. The zone is exited
/// automatically whenever the enclosing scope is exited. This macro can only
/// be used from C++. Use a semicolon at the end of the macro.
#define PROFILER_ENTER_SCOPE(zone)                                            \
    static PROFILER_DEFINE_ZONE(zone);                                        \
    PROFILER_DECLARE_SCOPE(zone)

/// Function signature for a user-defined function that allocates a new stack_t
/// structure. Instances are always fixed-size.
///
/// @param size_in_bytes The size of a single stack_t instance, in bytes.
/// @param context Opaque data associated with the allocator. This value is
/// optional and may be NULL.
/// @return The newly allocated instance, or NULL if memory allocation failed.
typedef profile::stack_t* (CMN_CALL_C *alloc_fn)(
    size_t   size_in_bytes,
    void    *context);

/// Function signature for a user-defined function that releases a stack_t
/// instance. The function can release the memory, return the instance to a
/// free pool, etc.
///
/// @param memory The item being released, as returned by the alloc_fn.
/// @param size_in_bytes The size of a single stack_t instance, in bytes.
/// @param context Opaque data associated with the allocator. This value is
/// optional and may be NULL.
typedef void  (CMN_CALL_C *free_fn)(
    profile::stack_t *memory,
    size_t            size_in_bytes,
    void             *context);

/// Function signature for a user-defined function that is called by the
/// profiler to render a single line item in the profile report.
///
/// @param x The screen-space x-coordinate of the first character.
/// @param y The screen-space y-coordinate of the first character.
/// @param text The NULL-terminated ASCII string specifying the text to render.
/// @param attributes Text attributes, such as the hotness of the entry.
typedef void  (CMN_CALL_C *render_text_fn)(
    float                  x,
    float                  y,
    char const            *text,
    profile::text_item_t  *attributes);

/// Function signature for a user-defined function that is called by the
/// profiler to render a profile graph.
///
/// @param zone_id The zone identifier.
/// @param start_index The starting index within the array of time values.
/// @param end_index The ending index within the array of time values.
/// @param attributes Graph attributes.
typedef void  (CMN_CALL_C *render_graph_fn)(
    uint32_t               zone_id,
    size_t                 start_index,
    size_t                 end_index,
    float                 *time_values,
    profile::graph_item_t *item);

/// Function signature for a user-defined function that is called by the
/// profiler to measure the width of a text string.
///
/// @param text The NULL-terminated ASCII string specifying the text to
/// be measured.
/// @param context Additional opaque data passed to the callback function.
typedef float (CMN_CALL_C *text_width_fn)(char const *text, void *context);

/// An enumeration defining the update modes supported by the profiler.
enum update_mode_e
{
    /// Indicates that data for the current tick should be discarded and not
    /// added to the frame history.
    UPDATE_DISCARD              = 0,
    /// Indicates that data for the current tick should be added to the frame
    /// histroy data.
    UPDATE_ACCUMULATE           = 1,
    /// This value is unused and serves to force the enumeration to be stored
    /// in (at minimum) a 32-bit value.
    UPDATE_FORCE_32BIT          = CMN_FORCE_32BIT,
};

/// An enumeration defining the reporting modes supported by the profiler.
enum report_mode_e
{
    /// The report displays flattened times, sorted by self time.
    REPORT_SELF_TIME            = 0,
    /// The report displays flattened times, sorted by heirarchical time.
    REPORT_HEIRARCHICAL_TIME    = 1,
    /// The report displays a call-graph with parent and child timing data.
    REPORT_CALL_GRAPH           = 2,
    /// This value is unused and serves to force the enumeration to be stored
    /// in (at minimum) a 32-bit value.
    REPORT_FORCE_32BIT          = CMN_FORCE_32BIT,
};

/// An enumeration defining the recursion reporting modes reported by the
/// profiler.
enum recursion_mode_e
{
    /// Recursive calls appear as a single zone in the report.
    RECURSION_FLATTEN           = 0,
    /// Recursive calls are displayed as distinct zones in the report.
    RECURSION_SPREAD            = 1,
    /// This value is unused and serves to force the enumeration to be stored
    /// in (at minimum) a 32-bit value.
    RECURSION_FORCE_32BIT       = CMN_FORCE_32BIT,
};

/// Stores the callback functions necessary to allocate and release profile
/// stack_t instances. An instance of this structure should be passed to the
/// profiler initialization function.
struct alloc_t
{
    alloc_fn   alloc;
    free_fn    free;
    void      *context;
};

/// Represents a single sample value within the profiler. These structures are
/// used for both time values and for entry counts.
struct scalar_t
{
    float      values[3];    /// computed values for the previous 3 frames
    float      variances[3]; /// computed variances for the previous 3 frames
    float      history[128]; /// measured values for the previous 128 frames
};

/// Represents historical values for a single stack location.
struct history_t
{
    scalar_t   self_time;    /// self-time values for the location
    scalar_t   heir_time;    /// heirarchical time values for the location
    scalar_t   entry_count;  /// entry counts for the stack location
    uint32_t   max_depth;    /// maximum recursion depth for a stack location
};

/// Represents a single, named profile zone. Each zone has up to 3 report nodes
/// associated with it, but only the currently expanded zone uses all 3 nodes.
struct zone_t
{
    char const *name;         /// the null-terminated string name of the zone
    float      *history;      /// pointer to an item in the zone history array
    record_t   *report_nodes; /// nodes associated with this zone in the report
    uint32_t    initialized;  /// non-zero if the zone is in use
    uint32_t    visited;      /// non-zero if the zone has been visited
};

/// Represents a single entry location to a profile zone. The same profile zone
/// can be entered from multiple locations within the code; each one of these
/// locations is represented by a stack_t instance. These instances are
/// allocated on the heap.
struct stack_t
{
    stack_t    *parent;      /// pointer to the parent zone entry location data
    zone_t     *zone;        /// pointer to the zone associated with this entry
    history_t   history;     /// historical data for this location
    int64_t     self_start;  /// tick count for first entry at this location
    int64_t     self_ticks;  /// number of ticks spent only in self
    int64_t     heir_ticks;  /// number of ticks spent in self and descendants
    uint32_t    entry_count; /// number of times the zone has been entered here
    uint32_t    entry_depth; /// number of times the location has recursed
};

/// Represents the record for a single zone within a profile report. The
/// profile report is the mechanism used to present information on-screen.
struct record_t
{
    char const *name;        /// the name of the associated zone
    zone_t     *zone;        /// pointer to the associated zone
    uint32_t    max_depth;   /// maximum recursion depth for the zone
    uint32_t    indent;      /// level of indentation when in call-graph mode
    uint32_t    value_flags; /// bit flags indicating which values are specified
    float       hotness;     /// the level of "hotness" for the zone
    float       values[4];   /// computed values for each of the columns
    char        prefix;      /// the prefix character used in call-graph mode
};

/// Represents the profile report. The report presents information on a maximum
/// of 512 profile zones. When in call-graph view, up to three report nodes may
/// be displayed per-zone.
struct report_t
{
    char        title[256];     /// title string containing FPS and ms/frame
    char        headers[5][32]; /// strings for each of the column header fields
    int32_t     highlighted;    /// zero-based index of the highlighted record
    int32_t     record_count;   /// the total number of used report records
    record_t    records[512*3]; /// storage for maximum number of report records
};

/// Represents the information required to display a single on-screen line item
/// from the profile report.
struct text_item_t
{
    float       hotness;     /// the "hotness" of the text item
    int32_t     title;       /// non-zero if the item is a title
    int32_t     header;      /// non-zero if the item is a header
    int32_t     highlighted; /// non-zero if the item is currently highlighted
    void       *user_data;   /// user-supplied context value
};

/// Represents the information required to display the profile graph.
struct graph_item_t
{
    float       origin_x;    /// the x-coordinate of the graph origin, specified in pixels
    float       origin_y;    /// the y-coordinate of the graph origin, specified in pixels
    float       spacing_x;   /// the screen-space size of each sample, specified in pixels
    float       spacing_y;   /// the screen-space size of one millisecond of time, in pixels
    void       *user_data;   /// user-supplied context value
};

/// A module-local value used to temporarially store a sample time value,
/// exported here because it is referenced explicitly by the macros above.
static int64_t           Profile_Time;

/// A global value used to track the current top-of-stack for the profiling
/// information. Exported here due to an explicit reference by the macros.
extern profile::stack_t* Profile_Stack;

/// A global dummy stack entry. Exported here due to an explicit reference by
/// the macros above.
extern profile::stack_t  Profile_Dummy_Stack;

/// Initializes a stack_t allocator instance.
///
/// @param alloc The allocator structure to initialize.
/// @param alloc_func The allocator function. If this value is NULL, @a alloc
/// will be initialized to use standard C library malloc and free functions.
/// @param free_func The memory free function. This value is optional.
/// @param context Pointer to optional opaque application-specified data to
/// be passed back to the allocation and free functions.
CMN_PUBLIC void allocator_init(
    profile::alloc_t *alloc,
    profile::alloc_fn alloc_func,
    profile::free_fn  free_func = NULL,
    void             *context   = NULL);

/// Initializes the profiler. This function should be called prior to calling
/// any of the profiler functions or public macros besides allocator_init().
///
/// @param allocator The allocator to use when allocating memory for stack
/// entries. If NULL, the standard C library malloc() and free() functions
/// are used.
CMN_PUBLIC void initialize(profile::alloc_t *allocator);

/// Shuts down the profiler, freeing any allocated memory. After calling this
/// function, none of the public API functions or macros should be used except
/// for allocator_init() and initialize().
CMN_PUBLIC void shutdown(void);

/// Retrieves the current reporting mode value.
/// @return One of profile::report_mode_e.
CMN_PUBLIC int32_t current_reporting_mode(void);

/// Retrieves the default reporting mode value.
/// @return One of profile::report_mode_e.
CMN_PUBLIC int32_t default_reporting_mode(void);

/// Retrieves the current recursion mode value.
/// @return One of profile::recursion_mode_e.
CMN_PUBLIC int32_t current_recursion_mode(void);

/// Retrieves the default recursion mode value.
/// @return One of profile::recursion_mode_e.
CMN_PUBLIC int32_t default_recursion_mode(void);

/// Retrieves the current tick averaging value.
/// @return The number of ticks over which timing values are averaged.
CMN_PUBLIC size_t  current_smoothing_factor(void);

/// Retrieves the default tick averaging value.
/// @return The default number of ticks over which timing values are averaged.
CMN_PUBLIC size_t  default_smoothing_factor(void);

/// Configures the reporting mode.
///
/// @param reporting_mode One of profile::report_mode_e.
/// @param recursion_mode One of profile::recursion_mode_e.
/// @param smoothing_factor A value in [0, 3) specifying the number of ticks
/// over which timing data is averaged.
CMN_PUBLIC void configure_report(
    int32_t reporting_mode,
    int32_t recursion_mode,
    size_t  smoothing_factor);

/// Sets the currently displayed tick, where a value of 0 specifies the data
/// for the most recent tick.
///
/// @param tick_index The zero-based index of the tick to display. The maximum
/// value is 127. This value will be clamped to a valid range.
CMN_PUBLIC void display_specific_tick(size_t tick_index);

/// Sets the currently displayed tick by specifying an offset value.
///
/// @param tick_delta The number of ticks by which to move the display.
/// Negative values move backward in time, while positive values move forward.
CMN_PUBLIC void move_displayed_tick(int32_t tick_delta);

/// Sets the current cursor position indicating the currently hilighted line
/// in the report.
///
/// @param line_index The zero-based index of the line to hilight. This value
/// is clamped to a valid range.
CMN_PUBLIC void set_cursor(size_t line_index);

/// Sets the current cursor position indicating the currently hilighted line
/// in the report by specifying an offset from the current value.
///
/// @param line_delta The delta value, in lines. Positive values move up in
/// the report, while negative values move down.
CMN_PUBLIC void move_cursor(int32_t line_delta);

/// Selects the currently hilighted item within the profile report, expanding
/// it to display information about that zone and its children.
CMN_PUBLIC void select_item(void);

/// Returns to the parent of the currently selected zone.
CMN_PUBLIC void select_parent(void);

/// This function should be called once per-tick to collect timing information
/// and update the profile report data.
///
/// @param update_mode One of the profile::update_mode_e values.
CMN_PUBLIC void update(int32_t update_mode);

/// Renders a report of the profiling information by calling back into user-
/// specified functions.
///
/// @param origin_x The screenspace x-coordinate where the topmost line will
/// be rendered.
/// @param origin_y The screenspace y-coordinate where the topmost line will
/// be rendered.
/// @param width The full width of the render target, specified in pixels.
/// @param height The full height of the render target, specified in pixels.
/// @param line_spacing The number of pixels to move by after each line.
/// Specify a negative value if y-decreases when moving towards the bottom of
/// the screen.
/// @param precision A value in [1, 4] specifying the number of digits of
/// precision that should appear after the decimal point.
/// @param render_func The application-specified text rendering callback. This
/// is called once for each line in the report.
/// @param measure_func The application-specified text measuring callback. This
/// is called once for each line in the report.
/// @param context Opaque application-defined data to be passed to the callback
/// functions @a render_func and @a measure_func.
CMN_PUBLIC void render_report(
    float                     origin_x,
    float                     origin_y,
    float                     width,
    float                     height,
    float                     line_spacing,
    size_t                    precision,
    profile::render_text_fn   render_func,
    profile::text_width_fn    measure_func,
    void                     *context);

/// Renders a graph of data for the profile zones by calling back into user-
/// specified functions.
///
/// @param origin_x The screenspace x-coordinate of the graph origin.
/// @param origin_y The screenspace y-coordinate of the graph origin.
/// @param spacing_x The screenspace size of each history sample.
/// @param spacing_y The screenspace size representing 1 millisecond of time.
/// @param render_func The application-specified rendering callback.
/// @param context Opaque application-defined data to be passed to the callback
/// function @a render_func.
CMN_PUBLIC void render_graph(
    float                     origin_x,
    float                     origin_y,
    float                     spacing_x,
    float                     spacing_y,
    profile::render_graph_fn  render_func,
    void                     *context);

/// Defines a convenience structure instance created on the stack to enter a
/// profile zone when the PROFILE_ENTER_SCOPE() macro is used and exit the
/// zone automatically whenever the enclosing scope is exited.
struct scope_t
{
    /// Constructs a new instance and enters the specified profile zone.
    /// @param zone The zone being entered. Do not modify the name of this
    /// parameter as it is referenced within the PROFILER_BEGIN_CODE() macro.
    /// @param Profile_Cache_Stack The stack entry location. Do not modify the
    /// name of this parameter as it is referenced in PROFILER_BEGIN_CODE().
    CMN_FORCE_INLINE scope_t(zone_t &zone, stack_t *&Profile_Cache_Stack)
    {
        PROFILER_BEGIN_CODE(zone);
    }
    /// Type destructor. Exits the enclosing profile zone.
    CMN_FORCE_INLINE ~scope_t(void)
    {
        PROFILER_END_RAW();
    }
};

/*/////////////////////
//   Namespace End   //
/////////////////////*/
}; /* end namespace profile */

#endif /* LIBPROFILE_HPP_INCLUDED */

/*/////////////////////////////////////////////////////////////////////////////
//    $Id$
///////////////////////////////////////////////////////////////////////////80*/
