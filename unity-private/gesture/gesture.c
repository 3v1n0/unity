/*
 * This file generated automatically from gesture.xml by c_client.py.
 * Edit at your peril.
 */

#include <string.h>
#include <assert.h>
#include "xcb/xcbext.h"
#include "gesture.h"
#include "X11/Xproto.h"

xcb_extension_t xcb_gesture_id = { "GestureExtension", 0 };


/*****************************************************************************
 **
 ** void xcb_gesture_float32_next
 ** 
 ** @param xcb_gesture_float32_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_gesture_float32_next (xcb_gesture_float32_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_gesture_float32_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_gesture_float32_end
 ** 
 ** @param xcb_gesture_float32_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_gesture_float32_end (xcb_gesture_float32_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** uint32_t * xcb_gesture_event_mask_mask_data
 ** 
 ** @param const xcb_gesture_event_mask_t *R
 ** @returns uint32_t *
 **
 *****************************************************************************/
 
uint32_t *
xcb_gesture_event_mask_mask_data (const xcb_gesture_event_mask_t *R  /**< */)
{
    return (uint32_t *) (R + 1);
}


/*****************************************************************************
 **
 ** int xcb_gesture_event_mask_mask_data_length
 ** 
 ** @param const xcb_gesture_event_mask_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_gesture_event_mask_mask_data_length (const xcb_gesture_event_mask_t *R  /**< */)
{
    return R->mask_len;
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_gesture_event_mask_mask_data_end
 ** 
 ** @param const xcb_gesture_event_mask_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_gesture_event_mask_mask_data_end (const xcb_gesture_event_mask_t *R  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = ((uint32_t *) (R + 1)) + (R->mask_len);
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** void xcb_gesture_event_mask_next
 ** 
 ** @param xcb_gesture_event_mask_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_gesture_event_mask_next (xcb_gesture_event_mask_iterator_t *i  /**< */)
{
    xcb_gesture_event_mask_t *R = i->data;
    xcb_generic_iterator_t child = xcb_gesture_event_mask_mask_data_end(R);
    --i->rem;
    i->data = (xcb_gesture_event_mask_t *) child.data;
    i->index = child.index;
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_gesture_event_mask_end
 ** 
 ** @param xcb_gesture_event_mask_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_gesture_event_mask_end (xcb_gesture_event_mask_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    while(i.rem > 0)
        xcb_gesture_event_mask_next(&i);
    ret.data = i.data;
    ret.rem = i.rem;
    ret.index = i.index;
    return ret;
}


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
                           uint16_t          minor_version  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_gesture_id,
        /* opcode */ XCB_GESTURE_QUERY_VERSION,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_gesture_query_version_cookie_t xcb_ret;
    xcb_gesture_query_version_request_t xcb_out;
    
    xcb_out.major_version = major_version;
    xcb_out.minor_version = minor_version;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


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
                                     uint16_t          minor_version  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_gesture_id,
        /* opcode */ XCB_GESTURE_QUERY_VERSION,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_gesture_query_version_cookie_t xcb_ret;
    xcb_gesture_query_version_request_t xcb_out;
    
    xcb_out.major_version = major_version;
    xcb_out.minor_version = minor_version;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


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
                                 xcb_generic_error_t                **e  /**< */)
{
    return (xcb_gesture_query_version_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}


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
                                   const uint32_t   *mask  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 4,
        /* ext */ &xcb_gesture_id,
        /* opcode */ XCB_GESTURE_SELECT_EVENTS,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[6];
    xcb_void_cookie_t xcb_ret;
    xcb_gesture_select_events_request_t xcb_out;
    
    xcb_out.window = window;
    xcb_out.device_id = device_id;
    xcb_out.mask_len = mask_len;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_parts[4].iov_base = (char *) mask;
    xcb_parts[4].iov_len = mask_len * sizeof(uint32_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


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
                           const uint32_t   *mask  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 4,
        /* ext */ &xcb_gesture_id,
        /* opcode */ XCB_GESTURE_SELECT_EVENTS,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[6];
    xcb_void_cookie_t xcb_ret;
    xcb_gesture_select_events_request_t xcb_out;
    
    xcb_out.window = window;
    xcb_out.device_id = device_id;
    xcb_out.mask_len = mask_len;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_parts[4].iov_base = (char *) mask;
    xcb_parts[4].iov_len = mask_len * sizeof(uint32_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


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
                                 xcb_window_t      window  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_gesture_id,
        /* opcode */ XCB_GESTURE_GET_SELECTED_EVENTS,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_gesture_get_selected_events_cookie_t xcb_ret;
    xcb_gesture_get_selected_events_request_t xcb_out;
    
    xcb_out.window = window;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


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
                                           xcb_window_t      window  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_gesture_id,
        /* opcode */ XCB_GESTURE_GET_SELECTED_EVENTS,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_gesture_get_selected_events_cookie_t xcb_ret;
    xcb_gesture_get_selected_events_request_t xcb_out;
    
    xcb_out.window = window;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** int xcb_gesture_get_selected_events_masks_length
 ** 
 ** @param const xcb_gesture_get_selected_events_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_gesture_get_selected_events_masks_length (const xcb_gesture_get_selected_events_reply_t *R  /**< */)
{
    return R->num_masks;
}


/*****************************************************************************
 **
 ** xcb_gesture_event_mask_iterator_t xcb_gesture_get_selected_events_masks_iterator
 ** 
 ** @param const xcb_gesture_get_selected_events_reply_t *R
 ** @returns xcb_gesture_event_mask_iterator_t
 **
 *****************************************************************************/
 
xcb_gesture_event_mask_iterator_t
xcb_gesture_get_selected_events_masks_iterator (const xcb_gesture_get_selected_events_reply_t *R  /**< */)
{
    xcb_gesture_event_mask_iterator_t i;
    i.data = (xcb_gesture_event_mask_t *) (R + 1);
    i.rem = R->num_masks;
    i.index = (char *) i.data - (char *) R;
    return i;
}


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
                                       xcb_generic_error_t                      **e  /**< */)
{
    return (xcb_gesture_get_selected_events_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

