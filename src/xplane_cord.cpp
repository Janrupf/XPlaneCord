//
// Created by jannis on 30.12.18.
//

#include <XPLM/XPLMPlugin.h>
#include <cstring>

PLUGIN_API int XPluginStart(char *name, char *signature, char *description) {
    std::strcpy(signature, "net.jan.XPlaneCord");
    std::strcpy(name, "XPlaneCord");
    std::strcpy(description, "Discord Rich Presence for your XPlane");

    return 1;
}

PLUGIN_API void XPluginStop() {}

PLUGIN_API int XPluginEnable() {
    return 1;
};

PLUGIN_API void XPluginDisable() {};

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID from, int msg, void *param) {};