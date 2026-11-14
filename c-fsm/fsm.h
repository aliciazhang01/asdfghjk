/**========================================================================
 *
 *                 ______         _______     _______..___  ___.
 *                /      |       |   ____|   /       ||   \/   |
 *               |  ,----' ______|  |__     |   (----`|  \  /  |
 *               |  |     |______|   __|     \   \    |  |\/|  |
 *               |  `----.       |  |    .----)   |   |  |  |  |
 *                \______|       |__|    |_______/    |__|  |__|
 *
 *                                  c-fsm
 *        A single-header, easy-to-use, and lightweight FSM library in C
 *
 * ?                                ABOUT
 * @author         :  Evan Bertis-Sample
 * @email          :  esample21@gmail.com
 * @repo           :  https://github.com/evan-bertis-sample/c-fsm.git
 * @createdOn      :  1/26/2025
 * @description    :  A Finite State Machine (FSM) implementation in C
 *
 * ?                                USAGE
 * 1. Include "fsm.h" in your project
 * 2. If you want to use the default implementation, define FSM_IMPL before
 *    including "fsm.h"
 * 3. Create a new FSM using fsm_create
 * 4. Add states and transitions to the FSM using fsm_add_state and
 *    fsm_add_transition
 * 5. Run the FSM using fsm_run
 * 6. Stop the FSM using fsm_stop
 * 7. Destroy the FSM using fsm_destroy
 *
 * For exampe usage please refer to:
 * https://github.com/Evan-Bertis-Sample/c-fsm/tree/main/examples
 *
 *========================================================================**/

#ifndef __FSM_H__
#define __FSM_H__

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

// Only used for bool & size_t
// If you don't have stdbool.h or stdint.h, you can define these yourself
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// Enable/disable debug logging for the internal FSM implementation
#ifndef FSM_DEBUG
#define FSM_DEBUG 1
#endif  // FSM_DEBUG

/**========================================================================
 *                           Types and Functions
 *========================================================================**/

/// @brief typedef for size_t, just in case it's not defined
typedef size_t fsm_size_t;

/// @brief typedef for bool, just in case it's not defined
typedef bool fsm_bool;

/// @brief Forward declaration of the FSM structure
struct fsm;

/// @brief Function pointer types for the state functions
typedef void (*fsm_state_fn)(struct fsm *fsm, void *context);

/// @brief Function pointer type for the transition predicates, which take an fsm, a context, and return a fsm_bool
typedef fsm_bool (*fsm_transition_predicate_fn)(struct fsm *fsm, void *context);

// typedef allocator/deallocator functions
typedef void *(*fsm_alloc_fn)(size_t size);
typedef void (*fsm_dealloc_fn)(void *ptr);

/// @brief Describes a state in the FSM
typedef struct fsm_state {
    char *name;
    fsm_state_fn on_enter;
    fsm_state_fn on_update;
    fsm_state_fn on_exit;
} fsm_state_t;

/// @brief Describes a transition in the FSM
typedef struct fsm_predicate_group {
    fsm_transition_predicate_fn *predicates;
    fsm_size_t predicate_count;
} fsm_predicate_group_t;

#define FSM_PREDICATE_GROUP(...)                                                                                      \
    (fsm_predicate_group_t) {                                                                                         \
        .predicates = (fsm_transition_predicate_fn[]){__VA_ARGS__},                                                   \
        .predicate_count = sizeof((fsm_transition_predicate_fn[]){__VA_ARGS__}) / sizeof(fsm_transition_predicate_fn) \
    }

/// @brief Describes a transition in the FSM
/// @note This is an internal structure used to store transitions
///      in the FSM, do not use this directly
typedef struct __fsm_transition {
    fsm_state_t *from;
    fsm_state_t *to;
    fsm_predicate_group_t *predicates;
} __fsm_transition_t;

/// @brief Describes a Finite State Machine
/// @note This is the main structure used to store the FSM
/// @note Please interact with the FSM using the functions provided
typedef struct fsm {
    /// @brief Context passed to state functions, could be anything
    void *context;

    /// @brief Array of states in the FSM -- strong reference, we own this memory
    fsm_state_t *states;

    /// @brief Array of transitions in the FSM -- strong reference, we own this memory
    __fsm_transition_t *transitions;

    /// @brief Memory allocation function
    fsm_alloc_fn __alloc_fn;
    fsm_dealloc_fn __dealloc_fn;

    fsm_size_t __context_size;
    fsm_size_t __state_count;
    fsm_size_t __transition_count;
    fsm_size_t __current_state_idx;

    fsm_bool __is_running;
} fsm_t;

/// @brief Creates a new FSM, starting with no states or transitions
/// @param alloc_fn Memory allocation function
/// @param dealloc_fn Memory deallocation function
/// @param context Context passed to state functions
/// @param context_size Size of the context
/// @return A new FSM, allocated using alloc_fn
fsm_t *fsm_create(fsm_alloc_fn alloc_fn, fsm_dealloc_fn dealloc_fn, void *context, size_t context_size);

/// @brief Runs the FSM, starting from the first state
/// @param fsm The FSM to run
void fsm_run(fsm_t *fsm);

/// @brief Stops the FSM, preventing it from running
/// @param fsm The FSM to stop
void fsm_stop(fsm_t *fsm);

/// @brief Destroys the FSM, freeing all memory associated with it
/// @param fsm The FSM to destroy
void fsm_destroy(fsm_t *fsm);

/// @brief Sets the current state of the FSM
/// @param fsm The FSM to set the state of
/// @param state_name The name of the state to set
/// @note This shouldn't be used to change the state of the FSM, use transitions instead
///       This is mainly so you can set the initial state of the FSM, which defaults to the first state
void fsm_set_state(fsm_t *fsm, char *state_name);

/*
 * Note about the fsm_add_xxx functions:
 * Whenever you add a state or transition to the FSM, the FSM will take ownership of the memory
 * and will free it when the FSM is destroyed. This means that you should not free the memory.
 * Modifying whatever fsm_state_t or fsm_predicate_group_t you pass to the fsm_add_xxx functions
 * after you pass them to the FSM will not have any effect on the FSM.
 * Instead, a copy of the data will be made and stored in the FSM.
 */

/// @brief Adds a state to the FSM
/// @param fsm The FSM to add the state to
/// @param state The state to add
/// @note The FSM will take ownership of the memory of the state, making a copy of it
void fsm_add_state(fsm_t *fsm, fsm_state_t state);

/// @brief Adds a transition to the FSM
/// @param fsm The FSM to add the transition to
/// @param from The name of the state to transition from
/// @param to The name of the state to transition to
/// @param predicates The predicates that must be true for the transition to occur
/// @note The FSM will take ownership of the memory of the predicates, making a copy of it
void fsm_add_transition(fsm_t *fsm, char *from, char *to, fsm_predicate_group_t predicates);

/// @brief Adds a transition from all states to a specific state
/// @param fsm The FSM to add the transition to
/// @param to The name of the state to transition to
/// @param predicates The predicates that must be true for the transition to occur
/// @note The FSM will take ownership of the memory of the predicates, making a copy of it
void fsm_add_transition_from_all(fsm_t *fsm, char *to, fsm_predicate_group_t predicates);

/// @brief Adds a transition to all states from a specific state
/// @param fsm The FSM to add the transition to
/// @param from The name of the state to transition from
/// @param predicates The predicates that must be true for the transition to occur
/// @note The FSM will take ownership of the memory of the predicates, making a copy of it
void fsm_add_transition_to_all(fsm_t *fsm, char *from, fsm_predicate_group_t predicates);

/// @brief Gets the number of states in the FSM
/// @param fsm The FSM to get the state count of
inline fsm_size_t fsm_state_count(fsm_t *fsm) { return fsm->__state_count; }

/// @brief Gets the number of transitions in the FSM
/// @param fsm The FSM to get the transition count of
inline fsm_size_t fsm_transition_count(fsm_t *fsm) { return fsm->__transition_count; }

/// @brief Gets the name of the current state of the FSM
/// @param fsm The FSM to get the current state of
inline char *fsm_current_state(fsm_t *fsm) { return fsm->states[fsm->__current_state_idx].name; }

/// @brief Checks if the FSM is running
/// @param fsm The FSM to check if it is running
inline fsm_bool fsm_is_running(fsm_t *fsm) { return fsm->__is_running; }

/// @brief Creates a new FSM given a context, using malloc and free as alloc/dealloc functions
#define FSM_CREATE(context) fsm_create(malloc, free, context, sizeof(context))

/// @brief Gets the context of the FSM as a specific type
/// @param fsm The FSM to get the context of
/// @param type The type to cast the context to
#define FSM_GET_CONTEXT(fsm, type) (type *)fsm->context

/**========================================================================
 *                           Macros and Logging
 *========================================================================**/

#define FSM_LINE __LINE__
#define FSM_STR_HELPER(x) #x
#define FSM_STR(x) FSM_STR_HELPER(x)

#define FSM_ERROR_COLOR "\033[0;31m"
#define FSM_RESET_COLOR "\033[0m"

#ifdef FSM_DEBUG
#define FSM_DEUBG_PREFIX "[fsm.h:" FSM_STR(FSM_LINE) "] "
#define FSM_LOG(fmt, ...) printf(FSM_DEUBG_PREFIX fmt, ##__VA_ARGS__)
#define FSM_LOG_ERROR(fmt, ...) fprintf(stderr, FSM_ERROR_COLOR FSM_DEUBG_PREFIX FSM_RESET_COLOR fmt, ##__VA_ARGS__)
#else
#define FSM_LOG(fmt, ...) void(0)
#define FSM_LOG_ERROR(fmt, ...) void(0)
#endif  // FSM_DEBUG

/**========================================================================
 *                           Implementation
 *========================================================================**/

#ifdef FSM_IMPL

#include <stdio.h>   // optional, for debug prints if needed
#include <string.h>  // for memcpy, strlen, etc.

#include "fsm.h"

/// @brief Copies a string using the FSM's allocator
/// @param fsm The FSM with the allocator
/// @param src The string to copy
/// @return A newly allocated copy of `src`, or NULL on failure
char *__fsm_strdup(fsm_t *fsm, const char *src) {
    if (!src) {
        return NULL;
    }
    size_t len = strlen(src) + 1;
    char *dst = (char *)fsm->__alloc_fn(len);
    if (dst) {
        memcpy(dst, src, len);
    }
    return dst;
}

/// @brief Helper to compare state pointers to find index
fsm_size_t __fsm_state_ptr_index(fsm_t *fsm, fsm_state_t *state_ptr) {
    for (fsm_size_t i = 0; i < fsm->__state_count; i++) {
        if (&fsm->states[i] == state_ptr) {
            return i;
        }
    }
    return (fsm_size_t)-1;  // Not found
}

fsm_size_t __fsm_state_index(fsm_t *fsm, char *name) {
    if (!fsm || !name) {
        return (fsm_size_t)-1;
    }
    for (fsm_size_t i = 0; i < fsm->__state_count; i++) {
        if (fsm->states[i].name && strcmp(fsm->states[i].name, name) == 0) {
            return i;
        }
    }
    return (fsm_size_t)-1;
}

fsm_size_t __fsm_transition_index(fsm_t *fsm, char *from, char *to) {
    if (!fsm || !from || !to) {
        return (fsm_size_t)-1;
    }
    fsm_size_t from_idx = __fsm_state_index(fsm, from);
    fsm_size_t to_idx = __fsm_state_index(fsm, to);
    if (from_idx == (fsm_size_t)-1 || to_idx == (fsm_size_t)-1) {
        return (fsm_size_t)-1;
    }

    for (fsm_size_t i = 0; i < fsm->__transition_count; i++) {
        __fsm_transition_t *t = &fsm->transitions[i];
        // Compare pointer addresses
        if (t->from == &fsm->states[from_idx] && t->to == &fsm->states[to_idx]) {
            return i;
        }
    }
    return (fsm_size_t)-1;
}

fsm_t *fsm_create(fsm_alloc_fn alloc_fn, fsm_dealloc_fn dealloc_fn, void *context, size_t context_size) {
    if (!alloc_fn || !dealloc_fn) {
        return NULL;
    }

    // Allocate the FSM structure
    fsm_t *fsm = (fsm_t *)alloc_fn(sizeof(fsm_t));
    if (!fsm) {
        return NULL;
    }

    // Initialize everything
    fsm->context = NULL;
    fsm->states = NULL;
    fsm->transitions = NULL;

    fsm->__alloc_fn = alloc_fn;
    fsm->__dealloc_fn = dealloc_fn;

    fsm->__context_size = context_size;
    fsm->__state_count = 0;
    fsm->__transition_count = 0;
    fsm->__current_state_idx = 0;
    fsm->__is_running = false;

    // If we have a context and a nonzero size, copy it into FSM->context
    if (context && context_size > 0) {
        fsm->context = alloc_fn(context_size);
        if (fsm->context == NULL) {
            // Allocation failed, clean up
            dealloc_fn(fsm);
            return NULL;
        }
        memcpy(fsm->context, context, context_size);
    }

    return fsm;
}

void fsm_run(fsm_t *fsm) {
    if (!fsm) return;

    // If we were not running before, mark running and call on_enter of the current state
    if (!fsm->__is_running) {
        if (fsm->__state_count == 0) {
            // No states? Nothing to run.
            return;
        }
        fsm->__is_running = true;

        fsm_state_t *initial_state = &fsm->states[fsm->__current_state_idx];
        if (initial_state->on_enter) {
            initial_state->on_enter(fsm, fsm->context);
        }
    }

    // If we're not running for some reason, just return
    if (!fsm->__is_running) {
        return;
    }

    // 1. Identify the current state
    fsm_state_t *current_state = &fsm->states[fsm->__current_state_idx];

    // 2. Check transitions out of the current state
    //    We'll apply the first valid transition encountered.
    for (fsm_size_t i = 0; i < fsm->__transition_count; i++) {
        __fsm_transition_t *transition = &fsm->transitions[i];
        if (transition->from == current_state) {
            // Check if all predicates in transition->predicates are satisfied
            fsm_bool transition_ok = true;
            for (fsm_size_t p = 0; p < transition->predicates->predicate_count; p++) {
                fsm_transition_predicate_fn predicate_fn = transition->predicates->predicates[p];
                if (!predicate_fn(fsm, fsm->context)) {
                    transition_ok = false;
                    break;
                }
            }

            // If all predicates are true, perform the transition
            if (transition_ok) {
                // on_exit of current state
                if (current_state->on_exit) {
                    current_state->on_exit(fsm, fsm->context);
                }

                // Switch current_state_idx to the transition target
                fsm_size_t new_idx = __fsm_state_ptr_index(fsm, transition->to);
                if (new_idx != (fsm_size_t)-1) {
                    fsm->__current_state_idx = new_idx;
                }

                // on_enter of new state
                if (transition->to->on_enter) {
                    transition->to->on_enter(fsm, fsm->context);
                }

                // We handle one transition per fsm_run call (break after the first match)
                break;
            }
        }
    }

    // 3. Call on_update of the (possibly new) current state
    current_state = &fsm->states[fsm->__current_state_idx];
    if (current_state->on_update) {
        current_state->on_update(fsm, fsm->context);
    }
}

void fsm_stop(fsm_t *fsm) {
    if (!fsm) return;
    if (!fsm->__is_running) return;

    fsm->__is_running = false;

    // Optionally, you could invoke the on_exit handler here if desired:
    /*
    fsm_state_t *current_state = &fsm->states[fsm->__current_state_idx];
    if (current_state->on_exit) {
        current_state->on_exit(fsm, fsm->context);
    }
    */
}

void fsm_destroy(fsm_t *fsm) {
    if (!fsm) return;

    // Free states
    if (fsm->states) {
        // Free each state's name
        for (fsm_size_t i = 0; i < fsm->__state_count; i++) {
            if (fsm->states[i].name) {
                fsm->__dealloc_fn(fsm->states[i].name);
                fsm->states[i].name = NULL;
            }
        }
        fsm->__dealloc_fn(fsm->states);
        fsm->states = NULL;
    }

    // Free transitions
    if (fsm->transitions) {
        for (fsm_size_t i = 0; i < fsm->__transition_count; i++) {
            __fsm_transition_t *t = &fsm->transitions[i];
            // Free the predicates array if it exists
            if (t->predicates) {
                if (t->predicates->predicates) {
                    fsm->__dealloc_fn(t->predicates->predicates);
                }
                fsm->__dealloc_fn(t->predicates);
                t->predicates = NULL;
            }
        }
        fsm->__dealloc_fn(fsm->transitions);
        fsm->transitions = NULL;
    }

    // Free context
    if (fsm->context) {
        fsm->__dealloc_fn(fsm->context);
        fsm->context = NULL;
    }

    // Finally, free the FSM structure itself
    fsm->__dealloc_fn(fsm);
}

void fsm_set_state(fsm_t *fsm, char *state_name) {
    if (!fsm || !state_name || fsm->__state_count == 0) {
        return;
    }

    fsm_size_t idx = __fsm_state_index(fsm, state_name);
    if (idx == (fsm_size_t)-1) {
        // State not found
        return;
    }

    // If the FSM is running and we have a different current state, handle on_exit/ on_enter
    if (fsm->__is_running && idx != fsm->__current_state_idx) {
        fsm_state_t *old_state = &fsm->states[fsm->__current_state_idx];
        fsm_state_t *new_state = &fsm->states[idx];

        if (old_state->on_exit) {
            old_state->on_exit(fsm, fsm->context);
        }
        fsm->__current_state_idx = idx;
        if (new_state->on_enter) {
            new_state->on_enter(fsm, fsm->context);
        }
    } else {
        // Not running, or same index, just set it
        fsm->__current_state_idx = idx;
    }
}

void fsm_add_state(fsm_t *fsm, fsm_state_t state) {
    if (!fsm) return;

    // Allocate space for one more state
    fsm_size_t new_count = fsm->__state_count + 1;
    fsm_state_t *new_states = (fsm_state_t *)fsm->__alloc_fn(sizeof(fsm_state_t) * new_count);
    if (!new_states) {
        return;  // Allocation failed
    }

    // Copy the old states
    if (fsm->states) {
        memcpy(new_states, fsm->states, sizeof(fsm_state_t) * fsm->__state_count);
        fsm->__dealloc_fn(fsm->states);
        fsm->states = NULL;
    }

    fsm->states = new_states;

    // Initialize the new state at the end
    fsm_size_t idx = fsm->__state_count;
    fsm->states[idx].name = __fsm_strdup(fsm, state.name);
    fsm->states[idx].on_enter = state.on_enter;
    fsm->states[idx].on_update = state.on_update;
    fsm->states[idx].on_exit = state.on_exit;

    fsm->__state_count = new_count;
}

void fsm_add_transition(fsm_t *fsm, char *from, char *to, fsm_predicate_group_t predicates) {
    if (!fsm || !from || !to) {
        return;
    }

    // Find the 'from' state index
    fsm_size_t from_idx = __fsm_state_index(fsm, from);
    fsm_size_t to_idx = __fsm_state_index(fsm, to);
    if (from_idx == (fsm_size_t)-1 || to_idx == (fsm_size_t)-1) {
        return;  // Invalid states
    }

    // Allocate space for one more transition
    fsm_size_t new_count = fsm->__transition_count + 1;
    __fsm_transition_t *new_transitions =
        (__fsm_transition_t *)fsm->__alloc_fn(sizeof(__fsm_transition_t) * new_count);
    if (!new_transitions) {
        return;  // Allocation failed
    }

    // Copy the old transitions
    if (fsm->transitions) {
        memcpy(new_transitions, fsm->transitions,
               sizeof(__fsm_transition_t) * fsm->__transition_count);
        fsm->__dealloc_fn(fsm->transitions);
        fsm->transitions = NULL;
    }

    fsm->transitions = new_transitions;

    // Set up the new transition
    __fsm_transition_t *t = &fsm->transitions[fsm->__transition_count];
    t->from = &fsm->states[from_idx];
    t->to = &fsm->states[to_idx];

    // Copy predicate group
    t->predicates = (fsm_predicate_group_t *)fsm->__alloc_fn(sizeof(fsm_predicate_group_t));
    if (!t->predicates) {
        // roll back?
        // For simplicity, do nothing more here. If you want to be robust, handle partial failures.
        return;
    }

    // Copy the array of predicate functions
    t->predicates->predicate_count = predicates.predicate_count;
    size_t pred_array_size = sizeof(fsm_transition_predicate_fn) * predicates.predicate_count;
    t->predicates->predicates =
        (fsm_transition_predicate_fn *)fsm->__alloc_fn(pred_array_size);
    if (!t->predicates->predicates) {
        // Similarly, handle partial fail if desired
        return;
    }

    memcpy(t->predicates->predicates, predicates.predicates, pred_array_size);

    fsm->__transition_count = new_count;
}

void fsm_add_transition_from_all(fsm_t *fsm, char *to, fsm_predicate_group_t predicates) {
    if (!fsm || !to || fsm->__state_count == 0) {
        return;
    }

    fsm_size_t to_idx = __fsm_state_index(fsm, to);
    if (to_idx == (fsm_size_t)-1) {
        return;  // Invalid target
    }

    // For each state in the FSM, add a transition to the "to" state
    for (fsm_size_t i = 0; i < fsm->__state_count; i++) {
        // Avoid creating self-transitions if undesired.  If you want to allow
        // from==to transitions, remove this check:
        /*
        if (i == to_idx) {
            continue;
        }
        */
        fsm_add_transition(fsm, fsm->states[i].name, to, predicates);
    }
}

void fsm_add_transition_to_all(fsm_t *fsm, char *from, fsm_predicate_group_t predicates) {
    if (!fsm || !from || fsm->__state_count == 0) {
        return;
    }

    fsm_size_t from_idx = __fsm_state_index(fsm, from);
    if (from_idx == (fsm_size_t)-1) {
        return;  // Invalid origin
    }

    // For each state in the FSM, add a transition from the "from" state
    for (fsm_size_t i = 0; i < fsm->__state_count; i++) {
        // Avoid creating self-transitions if undesired:
        /*
        if (i == from_idx) {
            continue;
        }
        */
        fsm_add_transition(fsm, from, fsm->states[i].name, predicates);
    }
}

#endif  // FSM_IMPL

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // __FSM_H__