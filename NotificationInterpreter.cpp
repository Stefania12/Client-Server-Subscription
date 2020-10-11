#include "NotificationInterpreter.h"

NotificationInterpreter::NotificationInterpreter()
{
}

NotificationInterpreter::~NotificationInterpreter()
{
}

std::string NotificationInterpreter::get_int(notification notif)
{
    std::string sign;
    if (notif.payload[0] == 0)
        sign.assign("");
    else
        sign.assign("-");   

    uint32_t num = (uint32_t)ntohl(*(uint32_t*)(notif.payload+1));    

    return sign + std::to_string(num);
}

std::string NotificationInterpreter::get_short_real(notification notif)
{
    uint16_t num = ntohs(*(uint16_t*)notif.payload);
    unsigned int rounded, dec;
    rounded = num / 100;
    dec = num % 100;

    std::string s = std::to_string(rounded);
    s.append(".");
    if(dec < 10)
        s.append("0");
    s.append(std::to_string(dec));
    return s;
}

std::string NotificationInterpreter::get_float(notification notif)
{
    std::string sign;
    if (notif.payload[0] == 0)
        sign.assign("");
    else
        sign.assign("-");
    
    uint32_t num = ntohl(*(uint32_t*)(notif.payload + 1));
    uint8_t exp_module = notif.payload[5];
    uint32_t div = pow(10, exp_module);

    unsigned int rounded;
    rounded = num / div;

    std::string s = sign, dec_str = std::string("");
    s.append(std::to_string(rounded));
    
    if (exp_module > 0)
        s.append(".");

    for (uint8_t i = 0; i < exp_module; i++)
    {
        dec_str = std::to_string(num % 10) + dec_str;
        num /= 10;
    }

    s.append(dec_str);
    return s;
}

std::string NotificationInterpreter::get_string(notification notif)
{
    char buf[1600];
    memset(buf, 0, 1600);
    memcpy(buf, notif.payload, notif.len);

    std::string s;
    s.assign(buf, buf + strlen(buf));
    return s;
}

std::string NotificationInterpreter::get_payload(notification notif)
{
    switch (notif.data_type)
    {
        case 0: return get_int(notif);
                break;
        case 1: return get_short_real(notif);
                break;
        case 2: return get_float(notif);
                break;
        case 3: return get_string(notif);
                break;
    }
    return std::string();
}

std::string NotificationInterpreter::get_ip(notification notif)
{
    uint8_t* ip_ptr = (uint8_t*)&(notif.ip);

    std::string ip = std::to_string((uint8_t)(ip_ptr[0]));
    ip.append(".");
    ip.append(std::to_string((uint8_t)(ip_ptr[1])));
    ip.append(".");
    ip.append(std::to_string((uint8_t)(ip_ptr[2])));
    ip.append(".");
    ip.append(std::to_string((uint8_t)(ip_ptr[3])));

    return ip;
}


std::string NotificationInterpreter::get_port(notification notif)
{
    return std::to_string(ntohs(notif.port));
}


std::string NotificationInterpreter::get_topic(notification notif)
{
    char buf[100];
    memset(buf, 0, 100);
    memcpy(buf, notif.topic, sizeof(notif.topic));

    std::string s;
    s.assign(buf, buf+strlen(buf));
    return s;
}

std::string NotificationInterpreter::get_data_type(notification notif)
{
    switch(notif.data_type)
    {
        case 0: return std::string("INT");
                break;
        case 1: return std::string("SHORT_REAL");
                break;
        case 2: return std::string("FLOAT");
                break;
        case 3: return std::string("STRING");
                break;
    }
    return std::string();
}

std::string NotificationInterpreter::notification_to_string(notification notif)
{
    return get_ip(notif) + std::string(":") + get_port(notif)
            + std::string(" — ") + get_topic(notif) + std::string(" — ")
            + get_data_type(notif) + std::string(" — ") + get_payload(notif);
}