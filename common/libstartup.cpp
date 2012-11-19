/*/////////////////////////////////////////////////////////////////////////////
///
///  @file: libstartup.cpp
///  Implements an interface to perform deterministic system startup and
///  shutdown for a process.
///
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////
//   Includes   //
////////////////*/
#include <cassert>
#include "libstartup.hpp"

/*//////////////////////////
//   Using Declarations   //
//////////////////////////*/

/*//////////////////////
//   Implementation   //
//////////////////////*/

/*/////////////////////////////////////////////////////////////////////////80*/

/// A pointer to the head of the linked list used to store information about
/// registered runtime systems.
application::runtime_system_t *Runtime_System_Head    = NULL;

/// A pointer to the tail of the linked list used to store information about
/// registered runtime systems.
application::runtime_system_t *Runtime_System_Tail    = NULL;

/// A pointer to the first system to shut down when application::shutdown() is
/// called, and not all systems may have initialized successfully.
application::runtime_system_t *Runtime_Cleanup_Node   = NULL;

/// A flag specifying whether or not application::startup() has been called. If
/// this flag is true, we do not allow additional runtime systems to be
/// registered.
bool                           Runtime_Startup_Flag = false;

/*/////////////////////////////////////////////////////////////////////////80*/

/// The number of command-line arguments passed to application::startup().
size_t                         Command_Line_Argc    = 0;

/// An array of NULL-terminated strings representing command-line arguments
/// passed to application::startup().
char                         **Command_Line_Argv    = NULL;

/*/////////////////////////////////////////////////////////////////////////80*/

bool application::register_system(
    application::startup_fn        startup,
    application::cleanup_fn        cleanup,
    void                          *user_data,
    application::runtime_system_t *system_node)
{
    if (Runtime_Startup_Flag)
    {
        // startup has already completed. fail immediately.
        return false;
    }
    if (NULL == startup || NULL == system_node)
    {
        // one or more arguments are invalid.
        return false;
    }
    // initialize the system_node fields.
    system_node->startup = startup;
    system_node->cleanup = cleanup;
    system_node->user    = user_data;
    system_node->next    = NULL;
    system_node->prev    = NULL;
    // insert the system into the linked list.
    if (Runtime_System_Tail != NULL)
    {
        // inserting into a non-empty list.
        system_node->next         = NULL;
        system_node->prev         = Runtime_System_Tail;
        Runtime_System_Tail->next = system_node;
        Runtime_System_Tail       = system_node;
    }
    else
    {
        // inserting into an empty list.
        system_node->next         = NULL;
        system_node->prev         = NULL;
        Runtime_System_Head       = system_node;
        Runtime_System_Tail       = system_node;
    }
    return true;
}

/*/////////////////////////////////////////////////////////////////////////80*/

size_t application::argument_count(void)
{
    return Command_Line_Argc;
}

/*/////////////////////////////////////////////////////////////////////////80*/

char** application::argument_list(void)
{
    return Command_Line_Argv;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool application::startup(size_t argc, char **argv)
{
    if (Runtime_Startup_Flag)
    {
        // startup already completed successfully. call application::shutdown()
        // before calling application::startup() again.
        return false;
    }

    // cache values for future use:
    Command_Line_Argc  = argc;
    Command_Line_Argv  = argv;

    // basic runtime system startup has completed.
    Runtime_Startup_Flag = true;
    Runtime_Cleanup_Node = NULL;

    // initialize any registered systems before returning:
    application::runtime_system_t *system_iter = Runtime_System_Head;
    while (system_iter != NULL)
    {
        bool result = system_iter->startup(argc, argv, system_iter->user);
        if (!result)
        {
            // fail immediately.
            return false;
        }
        Runtime_Cleanup_Node = system_iter;
        system_iter          = system_iter->next;
    }
    return true;
}

/*/////////////////////////////////////////////////////////////////////////80*/

bool application::shutdown(bool reset /* = true */)
{
    bool result = true;

    // iterate backwards through the systems that started up successfully
    // and allow them to shutdown properly before we proceed with cleanup.
    application::runtime_system_t *cleanup_iter = Runtime_Cleanup_Node;
    while (cleanup_iter != NULL)
    {
        if (cleanup_iter->cleanup != NULL)
        {
            bool cleanup = cleanup_iter->cleanup(cleanup_iter->user);
            if (!cleanup)  result = false;
        }
        cleanup_iter = cleanup_iter->prev;
    }
    Runtime_Cleanup_Node   = NULL;

    // are we performing a complete shutdown of the runtime?
    if (reset)
    {
        // reset our linked list of runtime systems.
        Runtime_Cleanup_Node = NULL;
        Runtime_System_Head  = NULL;
        Runtime_System_Tail  = NULL;
        Command_Line_Argv    = NULL;
        Command_Line_Argc    = 0;
    }
    // we've completed shutdown.
    Runtime_Startup_Flag = false;
    return result;
}

/*/////////////////////////////////////////////////////////////////////////80*/

/*/////////////////////////////////////////////////////////////////////////////
//    $Id$
///////////////////////////////////////////////////////////////////////////80*/
