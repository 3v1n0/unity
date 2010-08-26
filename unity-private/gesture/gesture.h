/*
 * This file generated automatically from gesture.xml by c_client.py.
 * Edit at your peril.
 */

/**
 * @defgroup XCB_gesture_API XCB gesture API
 * @brief gesture XCB Protocol Implementation.
 * @{
 **/

#ifndef __GESTURE_H
#define __GESTURE_H

#include "xcb/xcb.h"
#include "X11/Xproto.h"

#ifdef __cplusplus
extern "C" {
#endif

#define XCB_GESTURE_MAJOR_VERSION 0
#define XCB_GESTURE_MINOR_VERSION 5
  
extern xcb_extension_t xcb_gesture_id;

typedef float xcb_gesture_float32_t;

/**
 * @brief xcb_gesture_float32_iterator_t
 **/
typedef struct xcb_gesture_float32_iterator_t {
    xcb_gesture_float32_t *data; /**<  */
    int                    rem; /**<  */
    int                    index; /**<  */
} xcb_gesture_float32_iterator_t;

/**
 * @brief xcb_gesture_event_mask_t
 **/
typedef struct xcb_gesture_event_mask_t {
    uint16_t device_id; /**<  */
    uint16_t mask_len; /**<  */
} xcb_gesture_event_mask_t;

/**
 * @brief xcb_gesture_event_mask_iterator_t
 **/
typedef struct xcb_gesture_event_mask_iterator_t {
    xcb_gesture_event_mask_t *data; /**<  */
    int                       rem; /**<  */
    int                       index; /**<  */
} xcb_gesture_event_mask_iterator_t;

/**
 * @brief xcb_gesture_query_version_cookie_t
 **/
typedef struct xcb_gesture_query_version_cookie_t {
    unsigned int sequence; /**<  */
} xcb_gesture_query_version_cookie_t;

/** Opcode for xcb_gesture_query_version. */
#define XCB_GESTURE_QUERY_VERSION 1

/**
 * @brief xcb_gesture_query_version_request_t
 **/
typedef struct xcb_gesture_query_version_request_t {
    uint8_t  major_opcode; /**<  */
    uint8_t  minor_opcode; /**<  */
    uint16_t length; /**<  */
    uint16_t major_version; /**<  */
    uint16_t minor_version; /**<  */
} xcb_gesture_query_version_request_t;

/**
 * @brief xcb_gesture_query_version_reply_t
 **/
typedef struct xcb_gesture_query_version_reply_t {
    uint8_t  response_type; /**<  */
    uint8_t  pad0; /**<  */
    uint16_t sequence; /**<  */
    uint32_t length; /**<  */
    uint16_t major_version; /**<  */
    uint16_t minor_version; /**<  */
    uint8_t  pad1[20]; /**<  */
} xcb_gesture_query_version_reply_t;

/** Opcode for xcb_gesture_select_events. */
#define XCB_GESTURE_SELECT_EVENTS 2

/**
 * @brief xcb_gesture_select_events_request_t
 **/
typedef struct xcb_gesture_select_events_request_t {
    uint8_t      major_opcode; /**<  */
    uint8_t      minor_opcode; /**<  */
    uint16_t     length; /**<  */
    xcb_window_t window; /**<  */
    uint16_t     device_id; /**<  */
    uint16_t     mask_len; /**<  */
} xcb_gesture_select_events_request_t;

/**
 * @brief xcb_gesture_get_selected_events_cookie_t
 **/
typedef struct xcb_gesture_get_selected_events_cookie_t {
    unsigned int sequence; /**<  */
} xcb_gesture_get_selected_events_cookie_t;

/** Opcode for xcb_gesture_get_selected_events. */
#define XCB_GESTURE_GET_SELECTED_EVENTS 3

/**
 * @brief xcb_gesture_get_selected_events_request_t
 **/
typedef struct xcb_gesture_get_selected_events_request_t {
    uint8_t      major_opcode; /**<  */
    uint8_t      minor_opcode; /**<  */
    uint16_t     length; /**<  */
    xcb_window_t window; /**<  */
} xcb_gesture_get_selected_events_request_t;

/**
 * @brief xcb_gesture_get_selected_events_reply_t
 **/
typedef struct xcb_gesture_get_selected_events_reply_t {
    uint8_t  response_type; /**<  */
    uint8_t  pad0; /**<  */
    uint16_t sequence; /**<  */
    uint32_t length; /**<  */
    uint16_t num_masks; /**<  */
    uint8_t  pad1[22]; /**<  */
} xcb_gesture_get_selected_events_reply_t;

/** Opcode for xcb_gesture_notify. */
#define XCB_GESTURE_NOTIFY 0

/**
 * @brief xcb_gesture_notify_event_t
 **/
typedef struct xcb_gesture_notify_event_t {
    uint8_t               response_type; /**<  */
    uint8_t               extension; /**<  */
    uint16_t              sequence; /**<  */
    uint32_t              length; /**<  */
    uint16_t              event_type; /**<  */
    uint16_t              gesture_id; /**<  */
    uint16_t              gesture_type; /**<  */
    uint16_t              device_id; /**<  */
    xcb_timestamp_t       time; /**<  */
    xcb_window_t          root; /**<  */
    xcb_window_t          event; /**<  */
    xcb_window_t          child; /**<  */
    uint32_t              full_sequence; /**<  */
    xcb_gesture_float32_t focus_x; /**<  */
    xcb_gesture_float32_t focus_y; /**<  */
    uint16_t              status; /**<  */
    uint16_t              num_props; /**<  */
} xcb_gesture_notify_event_t;

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_gesture_float32_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_gesture_float32_t)
 */

/*****************************************************************************
 **
 ** void xcb_gesture_float32_next
 ** 
 ** @param xcb_gesture_float32_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_gesture_float32_next (xcb_gesture_float32_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_gesture_float32_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_gesture_float32_end
 ** 
 ** @param xcb_gesture_float32_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_gesture_float32_end (xcb_gesture_float32_iterator_t i  /**< */);


/*****************************************************************************
 **
 ** uint32_t * xcb_gesture_event_mask_mask_data
 ** 
 ** @param const xcb_gesture_event_mask_t *R
 ** @returns uint32_t *
 **
 *****************************************************************************/
 
uint32_t *
xcb_gesture_event_mask_mask_data (const xcb_gesture_event_mask_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_gesture_event_mask_mask_data_length
 ** 
 ** @param const xcb_gesture_event_mask_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_gesture_event_mask_mask_data_length (const xcb_gesture_event_mask_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_gesture_event_mask_mask_data_end
 ** 
 ** @param const xcb_gesture_event_mask_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_gesture_event_mask_mask_data_end (const xcb_gesture_event_mask_t *R  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_gesture_event_mask_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_gesture_event_mask_t)
 */

/*****************************************************************************
 **
 ** void xcb_gesture_event_mask_next
 ** 
 ** @param xcb_gesture_event_mask_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_gesture_event_mask_next (xcb_gesture_event_mask_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_gesture_event_mask_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_gesture_event_mask_end
 ** 
 ** @param xcb_gesture_event_mask_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_gesture_event_mask_end (xcb_gesture_event_mask_iterator_t i  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

/*****************************************************************************
 **
 ** xcb_gesture_query_version_cookie_t xcb_gesture_query_version
 ** 
 ** @param xcb_connection_t *c
 ** @param uint16_t          major_version
 ** @param uint16_t          minor_version
 ** @returns xcb_gesture_query_version_cookie_t
 **
 *****************************************************************************/
 
xcb_gesture_query_version_cookie_t
xcb_gesture_query_version (xcb_connection_t *c  /**< */,
                           uint16_t          major_version  /**< */,
                           uint16_t          minor_version  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will cause
 * a reply to be generated. Any returned error will be
 * placed in the event queue.
 */

/*****************************************************************************
 **
 ** xcb_gesture_query_version_cookie_t xcb_gesture_query_version_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @param uint16_t          major_version
 ** @param uint16_t          minor_version
 ** @returns xcb_gesture_query_version_cookie_t
 **
 *****************************************************************************/
 
xcb_gesture_query_version_cookie_t
xcb_gesture_query_version_unchecked (xcb_connection_t *c  /**< */,
                                     uint16_t          major_version  /**< */,
                                     uint16_t          minor_version  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_gesture_query_version_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

/*****************************************************************************
 **
 ** xcb_gesture_query_version_reply_t * xcb_gesture_query_version_reply
 ** 
 ** @param xcb_connection_t                    *c
 ** @param xcb_gesture_query_version_cookie_t   cookie
 ** @param xcb_generic_error_t                **e
 ** @returns xcb_gesture_query_version_reply_t *
 **
 *****************************************************************************/
 
xcb_gesture_query_version_reply_t *
xcb_gesture_query_version_reply (xcb_connection_t                    *c  /**< */,
                                 xcb_gesture_query_version_cookie_t   cookie  /**< */,
                                 xcb_generic_error_t                **e  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_gesture_select_events_checked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @param uint16_t          device_id
 ** @param uint16_t          mask_len
 ** @param const uint32_t   *mask
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_gesture_select_events_checked (xcb_connection_t *c  /**< */,
                                   xcb_window_t      window  /**< */,
                                   uint16_t          device_id  /**< */,
                                   uint16_t          mask_len  /**< */,
                                   const uint32_t   *mask  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_gesture_select_events
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @param uint16_t          device_id
 ** @param uint16_t          mask_len
 ** @param const uint32_t   *mask
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_gesture_select_events (xcb_connection_t *c  /**< */,
                           xcb_window_t      window  /**< */,
                           uint16_t          device_id  /**< */,
                           uint16_t          mask_len  /**< */,
                           const uint32_t   *mask  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

/*****************************************************************************
 **
 ** xcb_gesture_get_selected_events_cookie_t xcb_gesture_get_selected_events
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @returns xcb_gesture_get_selected_events_cookie_t
 **
 *****************************************************************************/
 
xcb_gesture_get_selected_events_cookie_t
xcb_gesture_get_selected_events (xcb_connection_t *c  /**< */,
                                 xcb_window_t      window  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will cause
 * a reply to be generated. Any returned error will be
 * placed in the event queue.
 */

/*****************************************************************************
 **
 ** xcb_gesture_get_selected_events_cookie_t xcb_gesture_get_selected_events_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @returns xcb_gesture_get_selected_events_cookie_t
 **
 *****************************************************************************/
 
xcb_gesture_get_selected_events_cookie_t
xcb_gesture_get_selected_events_unchecked (xcb_connection_t *c  /**< */,
                                           xcb_window_t      window  /**< */);


/*****************************************************************************
 **
 ** int xcb_gesture_get_selected_events_masks_length
 ** 
 ** @param const xcb_gesture_get_selected_events_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_gesture_get_selected_events_masks_length (const xcb_gesture_get_selected_events_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_gesture_event_mask_iterator_t xcb_gesture_get_selected_events_masks_iterator
 ** 
 ** @param const xcb_gesture_get_selected_events_reply_t *R
 ** @returns xcb_gesture_event_mask_iterator_t
 **
 *****************************************************************************/
 
xcb_gesture_event_mask_iterator_t
xcb_gesture_get_selected_events_masks_iterator (const xcb_gesture_get_selected_events_reply_t *R  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_gesture_get_selected_events_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

/*****************************************************************************
 **
 ** xcb_gesture_get_selected_events_reply_t * xcb_gesture_get_selected_events_reply
 ** 
 ** @param xcb_connection_t                          *c
 ** @param xcb_gesture_get_selected_events_cookie_t   cookie
 ** @param xcb_generic_error_t                      **e
 ** @returns xcb_gesture_get_selected_events_reply_t *
 **
 *****************************************************************************/
 
xcb_gesture_get_selected_events_reply_t *
xcb_gesture_get_selected_events_reply (xcb_connection_t                          *c  /**< */,
                                       xcb_gesture_get_selected_events_cookie_t   cookie  /**< */,
                                       xcb_generic_error_t                      **e  /**< */);


#ifdef __cplusplus
}
#endif

#endif

/**
 * @}
 */
