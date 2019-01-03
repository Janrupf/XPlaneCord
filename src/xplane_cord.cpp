//
// Created by jannis on 30.12.18.
//

#include <XPLMPlugin.h>
#include <XPLMUtilities.h>
#include <XPLMDataAccess.h>
#include <XPLMProcessing.h>
#include <XPLMMenus.h>
#include <XPLMDisplay.h>
#include <XPLMGraphics.h>
#include <XPWidgets.h>
#include <XPStandardWidgets.h>
#include <XRAAS_ND_msg_decode.h>
#include <string>
#include <cstring>
#include <chrono>
#include <discord_rpc.h>

std::string state;
std::string details;
std::string large_image_key;
std::string large_image_text;
int64_t start_timestamp;

XPLMDataRef aircraft_icao_ref;
XPLMDataRef aircraft_on_ground_ref;
int old_aircraft_on_ground_value = -1;

void update_presence();
float flight_loop_callback(float elapsed_since_last_call, float elapsed_time_since_last_flight_loop, int counter, void *refcon);

XPWidgetID about_window;
XPWidgetID xraas2_status_caption;
void create_window();
void create_menu();

DiscordEventHandlers handlers = {};

int window_handler(XPWidgetMessage msg, XPWidgetID widget, intptr_t l_param, intptr_t h_param);

enum class xraas2_status {
    PRE_SEARCH,
    SEARCHING,
    FOUND,
    NOT_FOUND
};

xraas2_status xraas2_status = xraas2_status::PRE_SEARCH;
XPLMDataRef xraas2_data_ref;
char *xraas2_status_string;
uint8_t xraas_countdown = 0;
std::string xraas2_msg;

PLUGIN_API int XPluginStart(char *name, char *signature, char *description) {
    start_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    std::strcpy(signature, "net.jan.XPlaneCord");
    std::strcpy(name, "XPlaneCord");
    std::strcpy(description, "Discord Rich Presence for your X-Plane");

    xraas2_status_string = new char[19];
    std::strcpy(xraas2_status_string, "X-RAAS2: searching");

    create_window();
    create_menu();

    return true;
}

PLUGIN_API void XPluginStop() {
    XPDestroyWidget(about_window, 1);

    delete[] xraas2_status_string;
}

PLUGIN_API int XPluginEnable() {

    int xplane_version;
    int xplm_version;
    XPLMHostApplicationID host_id;
    XPLMGetVersions(&xplane_version, &xplm_version, &host_id);

    if(host_id != xplm_Host_XPlane) {
        return false;
    }

    Discord_Initialize("529061942673932308", &handlers, true, "269950");

    auto version_string = std::to_string(xplane_version);
    if(version_string.length() < 4) {
        version_string = version_string.substr(0, 1) + "." + version_string.substr(1, 2);
    } else {
        version_string = version_string.substr(0, 2) + "." + version_string.substr(2, 2);
    }

    version_string = "X-Plane " + version_string;

    state = "";
    details = "In the menu";
    large_image_text = version_string.c_str();

    if(xplane_version < 1000) {
        large_image_key = "xplane_9";
    } else {
        large_image_key = "xplane";
    }

    update_presence();

    XPLMRegisterFlightLoopCallback(flight_loop_callback, -1, nullptr);

    aircraft_icao_ref = XPLMFindDataRef("sim/aircraft/view/acf_ICAO");
    aircraft_on_ground_ref = XPLMFindDataRef("sim/flightmodel/failures/onground_any");

    return XPLMIsDataRefGood(aircraft_icao_ref) && XPLMIsDataRefGood(aircraft_on_ground_ref);
};

PLUGIN_API void XPluginDisable() {
    XPLMUnregisterFlightLoopCallback(flight_loop_callback, nullptr);

    Discord_Shutdown();
};

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID from, int id, void *param) {
    if(id == XPLM_MSG_PLANE_LOADED) {
        xraas2_status = xraas2_status::SEARCHING;

        char aircraft_icao[40];
        XPLMGetDatab(aircraft_icao_ref, aircraft_icao, 0, 40);
        details = "Flying " + std::string(aircraft_icao);

        update_presence();
    }
};

float flight_loop_callback(float elapsed_since_last_call, float elapsed_time_since_last_flight_loop, int counter, void *refcon) {
    if(xraas2_status == xraas2_status::PRE_SEARCH) {
        xraas2_status = xraas2_status::SEARCHING;
    } else if(xraas2_status == xraas2_status::SEARCHING) {
        xraas2_data_ref = XPLMFindDataRef("xraas/ND_alert");
        if(XPLMIsDataRefGood(xraas2_data_ref)) {
            xraas2_status = xraas2_status::FOUND;
            std::strcpy(xraas2_status_string, "X-RAAS2: found");
        } else {
            xraas2_status = xraas2_status::NOT_FOUND;
            std::strcpy(xraas2_status_string, "X-RAAS2: not found");
        }
        XPDestroyWidget(xraas2_status_caption, 0);
        xraas2_status_caption = XPCreateWidget(
                60, 600, 300, 525,
                1,
                xraas2_status_string,
                0,
                about_window,
                xpWidgetClass_Caption
        );
    }

    if(details == "In the menu") {
        char aircraft_icao[40];
        XPLMGetDatab(aircraft_icao_ref, aircraft_icao, 0, 40);
        details = "Flying " + std::string(aircraft_icao);
    }

    bool changed = false;
    if(xraas2_status == xraas2_status::FOUND) {
        char buf[16];
        int color;

        int new_data = XPLMGetDatai(aircraft_on_ground_ref);
        if(new_data < 1) {
            if (XRAAS_ND_msg_decode(XPLMGetDatai(xraas2_data_ref), buf, &color)) {
                std::string new_xraas_string(buf);
                if(new_xraas_string != xraas2_msg) {
                    changed = true;
                    xraas_countdown = 30;
                    state = "In the air (" + new_xraas_string + ")";
                } else {
                    xraas_countdown--;
                }
            } else {
                if(xraas_countdown > 0) {
                    xraas_countdown--;
                } else {
                    changed = true;
                    xraas2_msg = "";
                    state = "In the air";
                }
            }
        } else if (new_data != old_aircraft_on_ground_value) {
            xraas2_msg= "";
            xraas_countdown = 0;
            state = "On the ground";
            changed = true;
        }

        old_aircraft_on_ground_value = new_data;
        if(changed) {
            update_presence();
        }
    } else {
        int new_data = XPLMGetDatai(aircraft_on_ground_ref);
        if(new_data != old_aircraft_on_ground_value) {
            old_aircraft_on_ground_value = new_data;

            if(old_aircraft_on_ground_value < 1) {
                state = "In the air";
            } else {
                state = "On the ground";
            }

            changed = true;
        }
    }
    if(changed) {
        update_presence();
    }

    return 2;
}

void update_presence() {
    DiscordRichPresence presence = {};
    presence.state = state.c_str();
    presence.details = details.c_str();
    presence.largeImageKey = large_image_key.c_str();
    presence.largeImageText = large_image_text.c_str();
    presence.startTimestamp = start_timestamp;

    Discord_UpdatePresence(&presence);
}

void create_window() {
    about_window = XPCreateWidget(
            50, 600, 300, 500,
            0,
            "XPlaneCord",
            1,
            nullptr,
            xpWidgetClass_MainWindow
            );

    XPCreateWidget(
            60, 600, 300, 550,
            1,
            "XPlaneCord v1.1a by Janrupf",
            0,
            about_window,
            xpWidgetClass_Caption);

    xraas2_status_caption = XPCreateWidget(
            60, 600, 300, 525,
            1,
            xraas2_status_string,
            0,
            about_window,
            xpWidgetClass_Caption
            );

    XPSetWidgetProperty(about_window, xpProperty_MainWindowHasCloseBoxes, 1);
    XPSetWidgetProperty(about_window, xpProperty_MainWindowType, xpMainWindowStyle_MainWindow);
    XPAddWidgetCallback(about_window, window_handler);
    XPHideWidget(about_window);
}

void create_menu() {
    int menu_item_id = XPLMAppendMenuItem(
            XPLMFindPluginsMenu(),
            "XPlaneCord",
            nullptr,
            0);

    XPLMMenuID menu_id = XPLMCreateMenu(
            "XPlaneCord",
            XPLMFindPluginsMenu(),
            menu_item_id,
            [](void *, void *item_ref){
                switch ((uint64_t) item_ref) {
                    case 0x1: {
                        XPShowWidget(about_window);
                        break;
                    }

                    case 0x2: {
                        XPLMReloadPlugins();
                    }

                    default:
                        break;
                }

            },
            nullptr
            );

    XPLMAppendMenuItem(
            menu_id,
            "About",
            (void *) 0x1,
            0
    );

    XPLMAppendMenuItem(
            menu_id,
            "Reload",
            (void *) 0x2,
            0
    );
}

int window_handler(XPWidgetMessage msg, XPWidgetID widget, intptr_t l_param, intptr_t h_param) {
    if(msg == xpMessage_CloseButtonPushed) {
        XPHideWidget(about_window);
        return 1;
    }

    return 0;
}