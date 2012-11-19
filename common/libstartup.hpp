/*/////////////////////////////////////////////////////////////////////////////
/// @summary Defines an interface to perform deterministic system startup
/// and shutdown for a process.
/// @author Russell Klenk (russ@ninjabirdstudios.com)
///////////////////////////////////////////////////////////////////////////80*/
#ifndef LIBSTARTUP_HPP_INCLUDED
#define LIBSTARTUP_HPP_INCLUDED

/*////////////////
//   Includes   //
////////////////*/
#include "common.hpp"

/*///////////////////////
//   Namespace Begin   //
///////////////////////*/
namespace application {

/*////////////////////////////
//   Forward Declarations   //
////////////////////////////*/

/*//////////////////////////////////
//   Public Types and Functions   //
//////////////////////////////////*/
/// A function signature for functions used to start up global systems in a
/// deterministic order. Startup functions are called in the order they are
/// registered.
///
/// @param argc The number of arguments passed in the @a argv array.
/// @param argv An array of null-terminated strings representing the arguments
/// passed to the application on the command line.
/// @param user A pointer to opaque user data associated with the system.
/// @return true if the system was started successfully, or false to halt
/// engine startup.
typedef bool (CMN_CALL_C *startup_fn) (size_t argc, char **argv, void *user);

/// A function signature for functions used to shut down global systems in a
/// deterministic order. Shutdown functions are called in the reverse order
/// of registration.
///
/// @param user A pointer to opaque user data associated with the system.
/// @return true if the system shutdown completed successfully.
typedef bool (CMN_CALL_C *cleanup_fn)(void *user);

/// A simple intrinsic linked list node used for describing a runtime system
/// that needs to be started up and shut down in a deterministic order. Memory
/// for these structures is generally statically defined by the application.
struct runtime_system_t
{
    application::startup_fn  startup; /// Pointer to the startup function.
    application::cleanup_fn  cleanup; /// Pointer to the cleanup function.
    struct runtime_system_t *next;    /// Pointer to the next item.
    struct runtime_system_t *prev;    /// Pointer to the previous item.
    void                    *user;    /// Opaque user data pointer.
};

/// Registers a runtime system for deterministic startup and shutdown. This
/// function should be called prior to calling application::startup(). The
/// startup functions are called in order of registration; cleanup functions
/// are called in reverse order. All invocations of this function must occur
/// on the same thread. Each process maintains its own system list.
///
/// @param startup Pointer to a function to call to initialize the system.
/// @param cleanup Pointer to a function to call to tear down the system. This
/// function will be called after the application is initialized, and
/// after application::shutdown() is called, but before application::shutdown()
/// returns. This function is optional and may be null.
/// @param user_data An optional opaque pointer to be passed to @a startup and
/// @a cleanup. This value may be null.
/// @param system_node A pointer to storage representing the runtime system.
/// This storage is typically allocated in static memory by the application.
/// This value is required and cannot be null.
/// @return true if the system was registered and @a system_node was populated
/// with data describing the system.
CMN_PUBLIC bool   register_system(
    application::startup_fn        startup,
    application::cleanup_fn        cleanup,
    void                          *user_data,
    application::runtime_system_t *system_node);

/// Retrieves the number of command-line arguments passed to the application.
/// This function only returns a valid value if application::startup() has been
/// called successfully.
///
/// @return The number of arguments in the command line argument list.
CMN_PUBLIC size_t argument_count(void);

/// Retrieves the command-line argument list passed to the application. This
/// function only returns a valid value if application::startup() has been
/// called successfully.
///
/// @return The command-line argument list, or NULL.
CMN_PUBLIC char** argument_list(void);

/// Performs application initialization. This function must be called once
/// per-process before calling any other runtime function, except for
/// application::register_system(), to initialize any runtime systems.
///
/// @param argc The number of arguments passed in the @a argv array.
/// @param argv An array of null-terminated strings representing the arguments
/// passed to the application on the command-line.
/// @return true if the application was initialized successfully.
CMN_PUBLIC bool   startup(size_t argc, char **argv);

/// Performs application system cleanup. This should be the last function
/// called before the process exits.
///
/// @param reset Specify true to completely reset the system to its initial
/// state. Specify false to preserve the list of registered runtime systems,
/// but reset everything else.
/// @return true if shutdown completed successfully. Call the function
/// application::startup() again to re-initialize the system.
CMN_PUBLIC bool   shutdown(bool reset = true);

/*/////////////////////
//   Namespace End   //
/////////////////////*/
}; /* end namespace application */

#endif /* LIBSTARTUP_HPP_INCLUDED */

/*/////////////////////////////////////////////////////////////////////////////
//    $Id$
///////////////////////////////////////////////////////////////////////////80*/
