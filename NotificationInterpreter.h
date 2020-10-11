#include "includes.h"
#include <iostream>
#include <cmath>

#ifndef NOTIFICATIONINTERPRETER_H
#define NOTIFICATIONINTERPRETER_H

// Interprets a notification from server and returns the associated string.
class NotificationInterpreter
{
    public:
    // Returns the associated string of notification.
    static std::string notification_to_string(notification notif);

    private:
    NotificationInterpreter();
    ~NotificationInterpreter();
    static std::string get_int(notification notif);
    static std::string get_short_real(notification notif);
    static std::string get_float(notification notif);
    static std::string get_string(notification notif);
    // Returns the associated string for the payload of notification.
    static std::string get_payload(notification notif);
    // Returns ip as string.
    static std::string get_ip(notification notif);
    // Returns port as string.
    static std::string get_port(notification notif);
    // Returns topic asa string.
    static std::string get_topic(notification notif);
    // Returns data_type as string.
    static std::string get_data_type(notification notif);
};

#endif