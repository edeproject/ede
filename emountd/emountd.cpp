/*
 * $Id$
 *
 * emountd, mount/unmount notifier
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2008 Sanel Zukan <karijes@equinox-project.org>
 *
 * This program is licensed under the terms of the 
 * GNU General Public License version 2 or later.
 * See COPYING for the details.
 */

/* 
 * A bunch of this code is inspired from Thunar (http://thunar.xfce.org)
 * since HAL documentation pretty sucks.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>

#ifdef HAVE_HAL
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <libhal-storage.h>
#include <edelib/List.h>
#include <edelib/Missing.h>

/* HAL can return NULL as error message if not checked */
#define CHECK_STR(s) ((s != NULL) ? s : "<unknown>")

#define EMOUNTD_INTERFACE   "org.equinoxproject.Emountd"
#define EMOUNTD_OBJECT_PATH "/org/equinoxproject/Emountd"

#define EDE_SHUTDOWN_INTERFACE "org.equinoxproject.Shutdown"
#define EDE_SHUTDOWN_MEMBER    "Shutdown"

struct DeviceInfo {
	unsigned int udi_hash;
	const char*  label;
	const char*  mount_point;
	const char*  device_file;

	LibHalDriveType      drive_type;
	LibHalVolumeDiscType cdrom_type;  /* Valid only if drive_type == LIBHAL_DRIVE_TYPE_CDROM */

	int read_only;
};

/* audio CD are special */
struct DeviceAudioInfo {
	unsigned int udi_hash;
	const char*  label;
	const char*  device_file;
};

typedef edelib::list<unsigned int>           UIntList;
typedef edelib::list<unsigned int>::iterator UIntListIter;

static int             quit_signaled = 0;
static DBusConnection* bus_connection = NULL;
static UIntList        known_audio_udis;

void quit_signal(int sig) {
	quit_signaled = 1;
}

/* A hash function from Dr.Dobbs Journal.  */
unsigned int do_hash(const char* key, int keylen) {
	unsigned hash ;
	int i;

	for (i = 0, hash = 0; i < keylen ;i++) {
		hash += (long)key[i] ;
		hash += (hash<<10);
		hash ^= (hash>>6) ;
	}

	hash += (hash <<3);
	hash ^= (hash >>11);
	hash += (hash <<15);

	return hash ;
}

void send_dbus_signal_mounted(DeviceInfo* info) {
	int          vi;
	dbus_bool_t  vb;
	const char*  vs;

	DBusMessage* msg = dbus_message_new_signal(EMOUNTD_OBJECT_PATH, EMOUNTD_INTERFACE, "Mounted");

	DBusMessageIter it;
	dbus_message_iter_init_append(msg, &it);

	/* open sub-iterator */
	DBusMessageIter sub;
	dbus_message_iter_open_container(&it, DBUS_TYPE_STRUCT, NULL, &sub);

	/* hash UDI is first member*/
	dbus_message_iter_append_basic(&sub, DBUS_TYPE_UINT32, &info->udi_hash);

	vs = info->label;
	dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &vs);
	vs = info->mount_point;
	dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &vs);
	vs = info->device_file;
	dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &vs);

	vb = (dbus_bool_t)info->read_only;
	dbus_message_iter_append_basic(&sub, DBUS_TYPE_BOOLEAN, &vb);

	vi = (int)info->drive_type;
	dbus_message_iter_append_basic(&sub, DBUS_TYPE_INT32, &vi);

	vi = (int)info->cdrom_type;
	dbus_message_iter_append_basic(&sub, DBUS_TYPE_INT32, &vi);

	/* close sub-iterator */
	dbus_message_iter_close_container(&it, &sub);

	dbus_uint32_t serial;
	if(!dbus_connection_send(bus_connection, msg, &serial))
		printf("Failed to send message (%i)\n", serial);

	dbus_connection_flush(bus_connection);
	dbus_message_unref(msg);
}

void send_dbus_signal_audio_cd_added(DeviceAudioInfo* info) {
	const char*  vs;
	DBusMessage* msg = dbus_message_new_signal(EMOUNTD_OBJECT_PATH, EMOUNTD_INTERFACE, "AudioCDAdded");

	DBusMessageIter it;
	dbus_message_iter_init_append(msg, &it);

	/* open sub-iterator */
	DBusMessageIter sub;
	dbus_message_iter_open_container(&it, DBUS_TYPE_STRUCT, NULL, &sub);

	/* hash UDI is first member*/
	dbus_message_iter_append_basic(&sub, DBUS_TYPE_UINT32, &info->udi_hash);

	vs = info->label;
	dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &vs);
	vs = info->device_file;
	dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &vs);

	/* close sub-iterator */
	dbus_message_iter_close_container(&it, &sub);

	dbus_uint32_t serial;
	if(!dbus_connection_send(bus_connection, msg, &serial))
		printf("Failed to send message (%i)\n", serial);

	dbus_connection_flush(bus_connection);
	dbus_message_unref(msg);
}

void send_dbus_signal_removed(unsigned int udi_hash, int is_audio) {
	const char* sig;
	if(is_audio)
		sig = "AudioCDRemoved";
	else
		sig = "Unmounted";

	DBusMessage* msg = dbus_message_new_signal(EMOUNTD_OBJECT_PATH, EMOUNTD_INTERFACE, sig);
	DBusMessageIter it;

	dbus_message_iter_init_append(msg, &it);
	dbus_message_iter_append_basic(&it, DBUS_TYPE_UINT32, &udi_hash);

	dbus_uint32_t serial;
	if(!dbus_connection_send(bus_connection, msg, &serial))
		printf("Failed to send message (%i)\n", serial);

	dbus_connection_flush(bus_connection);
	dbus_message_unref(msg);
}

void device_info_init(DeviceInfo* i) {
	i->udi_hash = 0;
	i->label = 0;
	i->mount_point = 0;
	i->device_file = 0;

	i->drive_type = (LibHalDriveType)-1; /* 0 is LIBHAL_DRIVE_TYPE_REMOVABLE_DISK */
	i->cdrom_type = (LibHalVolumeDiscType)-1;
	i->read_only = 0;
}

void device_audio_info_init(DeviceAudioInfo* i) {
	i->udi_hash = 0;
	i->label = 0;
	i->device_file = 0;
}

void device_info_send(LibHalContext* ctx, const char* udi) {
	/* spec said devices with this property should be ignored */
	if(libhal_device_get_property_bool(ctx, udi, "volume.ignore", 0))
		return;

	/* check volume */
	LibHalVolume* vol = libhal_volume_from_udi(ctx, udi);
	if(!vol)
		return;

	/* determine udi of the drive this volume belongs to */
	const char* drive_udi = libhal_volume_get_storage_device_udi(vol);
	if(drive_udi) {
		/* determine the drive for the volume */
		LibHalDrive* drive = libhal_drive_from_udi(ctx, drive_udi);
		if(drive) {
			const char* label = libhal_device_get_property_string(ctx, udi, "volume.label", 0);
			const char* dev   = libhal_drive_get_device_file(drive);
			unsigned int udi_hash = do_hash(udi, strlen(udi));

			/* for audio CD's we send special signals */
			if(libhal_volume_disc_has_audio(vol) && !libhal_volume_disc_has_data(vol)) {
				DeviceAudioInfo d;
				device_audio_info_init(&d);

				d.udi_hash = udi_hash;
				d.label = label;
				d.device_file = dev;

				/* 
				 * register hash so when Audio CD is ejected (device_removed() will be called)
				 * we knows should we send AudioCDRemoved signal
				 */
				known_audio_udis.push_back(udi_hash);

				/* send signal about Audio CD */
				send_dbus_signal_audio_cd_added(&d);
			} else {
				/* 
				 * Check if volume is mounted, since device_info_send() is called at emountd startup
				 * and collects known volumes. Also, this function is called from device_property_modified()
				 * and that will be when device mounts/unmounts. In either case, we send unmount signal
				 */
				if(!libhal_device_get_property_bool(ctx, udi, "volume.is_mounted", 0)) {
					send_dbus_signal_removed(udi_hash, 0); /* TODO: or to send drive_udi? */
				} else {
					DeviceInfo d;
					device_info_init(&d);

					d.udi_hash = udi_hash;
					d.label = label;
					d.device_file = dev;
					d.mount_point = libhal_device_get_property_string(ctx, udi, "volume.mount_point", 0);
					d.read_only = (int)libhal_device_get_property_bool(ctx, udi, "volume.is_mounted_read_only", 0);
					d.drive_type = libhal_drive_get_type(drive);

					/* determine CD type if we got it */
					if(d.drive_type == LIBHAL_DRIVE_TYPE_CDROM)
						d.cdrom_type = libhal_volume_get_disc_type(vol);

					/* send signal about mounted device*/
					send_dbus_signal_mounted(&d);

					libhal_free_string((char*)d.mount_point);
				}
			}

			libhal_free_string((char*)label);
			libhal_drive_free(drive);
		}
	}

	libhal_volume_free(vol);
}

void device_added(LibHalContext* ctx, const char* udi) {
	/* only used for Audio CD's */
	device_info_send(ctx, udi);
}

void device_removed(LibHalContext* ctx, const char* udi) {
	unsigned int udi_hash = do_hash(udi, strlen(udi));

	/* signal audio CD removal if udi match ours */
	UIntListIter it = known_audio_udis.begin(), it_end = known_audio_udis.end();
	for(; it != it_end; ++it) {
		if((*it) == udi_hash) {
			send_dbus_signal_removed(udi_hash, 1);
			known_audio_udis.erase(it);
			break;
		}
	}
}

void device_property_modified(LibHalContext* ctx, const char* udi, const char* key, dbus_bool_t is_rem, dbus_bool_t is_added) {
	/* 
	 * we watch only 'volume.mount_point' property; 
	 * it is always set after 'volue.is_mounted' property so we can check it manualy
	 */
	if(strcmp(key, "volume.mount_point") == 0)
		device_info_send(ctx, udi);
}

#endif // HAVE_HAL

void help(void) {
	puts("Usage: emountd [--no-daemon]");
	puts("EDE mount/unmount notify manager");
}

int main(int argc, char** argv) {
	int go_daemon = 1;
	if(argc > 1) {
		if(strcmp(argv[1], "--no-daemon") == 0)
			go_daemon = 0;
		else {
			help();
			return 1;
		}
	}

#ifndef HAVE_HAL
	printf("HAL support not enabled!\n");
	printf("Make sure you have system supporting HAL and installed HAL package and libraries\n");
	printf("For more details, please visit: 'http://freedesktop.org/HAL'\n");
	return 1;
#else
	/* run in background */
	if(go_daemon)
		edelib_daemon(0, 0);

	DBusError err;
	DBusConnection* conn = NULL, *session_conn = NULL;
	DBusMessage* down_msg = NULL;

	LibHalContext* ctx = NULL;
	LibHalDrive*   hd;

	char** drive_udis;
	int    n_drive_udis;

	char** udis;
	int    n_udis;

	signal(SIGINT, quit_signal);
	signal(SIGTERM, quit_signal);
	signal(SIGKILL, quit_signal);
	signal(SIGQUIT, quit_signal);

	ctx = libhal_ctx_new();
	if(!ctx) {
		puts("Unable to get libhal context");
		goto error;
	}

	dbus_error_init(&err);

	/* open system bus for HAL */
	conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
	if(dbus_error_is_set(&err)) {
		printf("Unable to connect to system bus %s : %s\n", err.name, err.message);
		dbus_error_free(&err);
		goto error;
	}

	/* open session bus for EDE */
	session_conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
	if(dbus_error_is_set(&err)) {
		printf("Unable to connect to session bus %s : %s\n", err.name, err.message);
		dbus_error_free(&err);
		goto error;
	}

	bus_connection = session_conn;

	/* 
	 * make sure we are running only one instance; this is done by requesting ownership
	 * of the interface name
	 */
	if(dbus_bus_request_name(bus_connection, EMOUNTD_INTERFACE, 0, 0) != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
		printf("One instance of emountd is already running! Balling out...\n");
		goto error;
	}

	/* let libhal use our system bus connection */
	libhal_ctx_set_dbus_connection(ctx, conn);

	if(!libhal_ctx_init(ctx, &err)) {
		printf("Unable to init libhal context %s : %s\n", CHECK_STR(err.name), CHECK_STR(err.message));
		printf("Does hald daemon is running?\n");
		dbus_error_free(&err);
		goto error;
	}

	libhal_ctx_set_device_added(ctx, device_added);
	libhal_ctx_set_device_removed(ctx, device_removed);
	libhal_ctx_set_device_property_modified(ctx, device_property_modified);

	/* explicitly state to watch all properties or nothing will be received */
	if(!libhal_device_property_watch_all(ctx, &err)) {
		printf("Unable to watch properties %s : %s\n", CHECK_STR(err.name), CHECK_STR(err.message));
		dbus_error_free(&err);
		goto error;
	}

	/* now lookup storage devices already known by HAL */
	drive_udis = libhal_find_device_by_capability(ctx, "storage", &n_drive_udis, &err);
	if(drive_udis) {
		for(int i = 0; i < n_drive_udis; i++) {
			hd = libhal_drive_from_udi(ctx, drive_udis[i]);
			if(!hd) 
				continue;

			/* check for floppy since it does not have volumes */
			if(libhal_drive_get_type(hd) == LIBHAL_DRIVE_TYPE_FLOPPY) {
				device_info_send(ctx, drive_udis[i]);
			} else {
				/* determine all volumes for the given drive */
				udis = libhal_drive_find_all_volumes(ctx, hd, &n_udis);
				if(udis) {
					for(int j = 0; j < n_udis; j++) {
						device_info_send(ctx, udis[j]);
						/* HAL bug #5279 */
						free(udis[j]);
					}
					/* HAL bug #5279 */
					free(udis);
				}
			}

			libhal_drive_free(hd);
		}

		libhal_free_string_array(drive_udis);
	}

	/* add matcher for quit dbus signal */
	dbus_bus_add_match(bus_connection, "type='signal',interface='"EDE_SHUTDOWN_INTERFACE"'", 0);

	/* 
	 * Now handle both system and session bus messages: system messages
	 * are to receive HAL notifications and bus messages are to dispatch them to EDE clients
	 * and to receive Shutdown signal
	 */
	while(1) {
		if(quit_signaled)
			break;

		/* fetch and dispatch system bus messages */
		dbus_connection_read_write_dispatch(conn, 10);

		/* but only fetch session bus messages; they are processed here */
		dbus_connection_read_write(bus_connection, 10);

		down_msg = dbus_connection_pop_message(bus_connection);
		if(down_msg && dbus_message_is_signal(down_msg, EDE_SHUTDOWN_INTERFACE, EDE_SHUTDOWN_MEMBER))
			break;

		sleep(1);
	}

error:
	if(!ctx)
		libhal_ctx_free(ctx);

	return 0;
#endif // HAVE_HAL
}
