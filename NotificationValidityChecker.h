#include "includes.h"

#ifndef NOTIFICATIONVALIDITYCHECKER_H
#define NOTIFICATIONVALIDITYCHECKER_H

// Checks the validity of a notification format.
class NotificationValidityChecker
{
    public:
    static bool is_topic_valid(notification *notif);
    static bool is_data_type_valid(notification *notif);
    static bool is_payload_valid(notification *notif);

    private:
    static bool is_INT_valid(notification *notif);
    static bool is_SHORT_REAL_valid(notification *notif);
    static bool is_FLOAT_valid(notification *notif);
    static bool is_STRING_valid(notification *notif);
};

#endif