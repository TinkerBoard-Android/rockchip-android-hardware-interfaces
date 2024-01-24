#ifndef RK_HDMI_CONNECTION_9383892929_H
#define RK_HDMI_CONNECTION_9383892929_H


#include <hardware/hdmi_cec.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <cutils/log.h>
#include <cutils/atomic.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


struct hdmi_connection_context_t {
	event_callback_t connection_event_callback;
	void* connection_arg;
	struct hdmi_port_info port;
	int fd;
	int phy_addr;
	bool hotplug;
	bool cec_init;
};

int rk_hdmi_connection_init(struct hdmi_connection_context_t *dev);
int rk_hdmi_connection_destroy(struct hdmi_connection_context_t *ctx);


void hdmi_connection_get_port_info(struct hdmi_connection_context_t* ctx, struct hdmi_port_info* list[], int* total);
void hdmi_connection_register_event_callback(struct hdmi_connection_context_t* ctx, event_callback_t callback, void* arg);
int hdmi_connection_is_connected(struct hdmi_connection_context_t* ctx, int port_id);
int hdmi_connection_set_phd_signal(struct hdmi_connection_context_t* ctx, int port_id, int signal);
int hdmi_connection_get_phd_signal(struct hdmi_connection_context_t* ctx, int port_id);



#endif
