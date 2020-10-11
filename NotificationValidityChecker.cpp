#include "NotificationValidityChecker.h"

bool NotificationValidityChecker::is_topic_valid(notification *notif)
{
    int i = 0;
    while (notif->topic[i] != 0 && i < 50)
    {
        // Topic is invalid if it has spaces.
        if (notif->topic[i] == ' ')
            return false;
        i++;
    }
    return notif->topic[0] != 0;
}

bool NotificationValidityChecker::is_data_type_valid(notification *notif)
{
    return notif->data_type <= 3;
}

bool NotificationValidityChecker::is_payload_valid(notification *notif)
{
    if (notif->data_type == 0)
        return is_INT_valid(notif);
    if (notif->data_type == 1)
        return is_SHORT_REAL_valid(notif);
    if (notif->data_type == 2)
        return is_FLOAT_valid(notif);
    return is_STRING_valid(notif);
}

bool NotificationValidityChecker::is_INT_valid(notification *notif)
{
    if (notif->len != 5)
        return false;
    // Check sign byte.
    if (notif->payload[0] > 1)
        return false;
    return true;
}

bool NotificationValidityChecker::is_SHORT_REAL_valid(notification *notif)
{
    return notif->len == 2;
}

bool NotificationValidityChecker::is_FLOAT_valid(notification *notif)
{
    if (notif->len != 6)
        return false;
    return notif->payload[0] < 2;
}

bool NotificationValidityChecker::is_STRING_valid(notification *notif)
{
    return notif->len <= 1500 && notif->len >= 0;
}