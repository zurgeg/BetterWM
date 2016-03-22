#include "agent.h"

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <getopt.h>
#include <string.h>
#include <dbus/dbus.h>

static volatile sig_atomic_t __io_terminated = 0;

DBusConnection *conn;
char match_string[128], default_path[128], *adapter_id = NULL;
char *adapter_path = NULL, *agent_path = NULL;

static DBusHandlerResult agent_filter(DBusConnection *conn,
						DBusMessage *msg, void *data)
{
	const char *name, *old, *new;

	if (!dbus_message_is_signal(msg, DBUS_INTERFACE_DBUS,
						"NameOwnerChanged"))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	if (!dbus_message_get_args(msg, NULL,
					DBUS_TYPE_STRING, &name,
					DBUS_TYPE_STRING, &old,
					DBUS_TYPE_STRING, &new,
					DBUS_TYPE_INVALID)) {
		fprintf(stderr, "Invalid arguments for NameOwnerChanged signal");
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	if (!strcmp(name, "org.bluez") && *new == '\0') {
		fprintf(stderr, "Agent has been terminated\n");
		__io_terminated = 1;
	}

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static DBusHandlerResult request_pincode_message(DBusConnection *conn,
						DBusMessage *msg, void *data)
{
	DBusMessage *reply;
	const char *path;

	//if (!agentcommand)
	//return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_OBJECT_PATH, &path,
							DBUS_TYPE_INVALID)) {
		fprintf(stderr, "Invalid arguments for RequestPinCode method");
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	reply = dbus_message_new_method_return(msg);
	if (!reply) {
		fprintf(stderr, "Can't create reply message\n");
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}

	printf("Pincode request for device %s\n", path);


	dbus_message_append_args(reply, DBUS_TYPE_STRING, &agentcommand,
							DBUS_TYPE_INVALID);

send:
	dbus_connection_send(conn, reply, NULL);

	dbus_connection_flush(conn);

	dbus_message_unref(reply);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult request_passkey_message(DBusConnection *conn,
						DBusMessage *msg, void *data)
{
	DBusMessage *reply;
	const char *path;
	//unsigned int passkey;

	//if (!agentcommand)
	//	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_OBJECT_PATH, &path,
							DBUS_TYPE_INVALID)) {
		fprintf(stderr, "Invalid arguments for RequestPasskey method");
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	reply = dbus_message_new_method_return(msg);
	if (!reply) {
		fprintf(stderr, "Can't create reply message\n");
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}

	printf("Passkey request for device %s\n", path);


	//passkey = strtoul(agentcommand, NULL, 10);

	dbus_message_append_args(reply, DBUS_TYPE_UINT32, &agentcommand,
							DBUS_TYPE_INVALID);

send:
	dbus_connection_send(conn, reply, NULL);

	dbus_connection_flush(conn);

	dbus_message_unref(reply);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult request_confirmation_message(DBusConnection *conn,
						DBusMessage *msg, void *data)
{
	DBusMessage *reply;
	const char *path;
	unsigned int passkey;

	if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_OBJECT_PATH, &path,
						DBUS_TYPE_UINT32, &passkey,
							DBUS_TYPE_INVALID)) {
		fprintf(stderr, "Invalid arguments for RequestPasskey method");
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	reply = dbus_message_new_method_return(msg);
	if (!reply) {
		fprintf(stderr, "Can't create reply message\n");
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}

	printf("Confirmation request of %u for device %s\n", passkey, path);


send:
	dbus_connection_send(conn, reply, NULL);

	dbus_connection_flush(conn);

	dbus_message_unref(reply);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult authorize_message(DBusConnection *conn,
						DBusMessage *msg, void *data)
{
	DBusMessage *reply;
	const char *path, *uuid;

	if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_OBJECT_PATH, &path,
						DBUS_TYPE_STRING, &uuid,
							DBUS_TYPE_INVALID)) {
		fprintf(stderr, "Invalid arguments for Authorize method");
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	reply = dbus_message_new_method_return(msg);
	if (!reply) {
		fprintf(stderr, "Can't create reply message\n");
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}

	printf("Authorizing request for %s\n", path);

send:
	dbus_connection_send(conn, reply, NULL);

	dbus_connection_flush(conn);

	dbus_message_unref(reply);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult cancel_message(DBusConnection *conn,
						DBusMessage *msg, void *data)
{
	DBusMessage *reply;

	if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_INVALID)) {
		fprintf(stderr, "Invalid arguments for passkey Confirm method");
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	printf("Request canceled\n");

	reply = dbus_message_new_method_return(msg);
	if (!reply) {
		fprintf(stderr, "Can't create reply message\n");
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}

	dbus_connection_send(conn, reply, NULL);

	dbus_connection_flush(conn);

	dbus_message_unref(reply);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult release_message(DBusConnection *conn,
						DBusMessage *msg, void *data)
{
	DBusMessage *reply;

	if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_INVALID)) {
		fprintf(stderr, "Invalid arguments for Release method");
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	if (!agent_done)
		fprintf(stderr, "Agent has been released\n");

	//agent_ready = 0;

	reply = dbus_message_new_method_return(msg);
	if (!reply) {
		fprintf(stderr, "Can't create reply message\n");
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
	}

	dbus_connection_send(conn, reply, NULL);

	dbus_connection_flush(conn);

	dbus_message_unref(reply);

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult agent_message(DBusConnection *conn,
						DBusMessage *msg, void *data)
{
	if (dbus_message_is_method_call(msg, "org.bluez.Agent",
							"RequestPinCode"))
		return request_pincode_message(conn, msg, data);

	if (dbus_message_is_method_call(msg, "org.bluez.Agent",
							"RequestPasskey"))
		return request_passkey_message(conn, msg, data);

	if (dbus_message_is_method_call(msg, "org.bluez.Agent",
							"RequestConfirmation"))
		return request_confirmation_message(conn, msg, data);

	if (dbus_message_is_method_call(msg, "org.bluez.Agent", "Authorize"))
		return authorize_message(conn, msg, data);

	if (dbus_message_is_method_call(msg, "org.bluez.Agent", "Cancel"))
		return cancel_message(conn, msg, data);

	if (dbus_message_is_method_call(msg, "org.bluez.Agent", "Release"))
		return release_message(conn, msg, data);

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static const DBusObjectPathVTable agent_table = {
	.message_function = agent_message,
};

static int register_agent(DBusConnection *conn, const char *adapter_path,
						const char *agent_path,
						const char *capabilities)
{
	DBusMessage *msg, *reply;
	DBusError err;

	msg = dbus_message_new_method_call("org.bluez", adapter_path,
					"org.bluez.Adapter", "RegisterAgent");
	if (!msg) {
		fprintf(stderr, "Can't allocate new method call\n");
		return -1;
	}

	dbus_message_append_args(msg, DBUS_TYPE_OBJECT_PATH, &agent_path,
					DBUS_TYPE_STRING, &capabilities,
					DBUS_TYPE_INVALID);

	dbus_error_init(&err);

	reply = dbus_connection_send_with_reply_and_block(conn, msg, -1, &err);

	dbus_message_unref(msg);

	if (!reply) {
		fprintf(stderr, "Can't register agent\n");
		if (dbus_error_is_set(&err)) {
			fprintf(stderr, "%s\n", err.message);
			dbus_error_free(&err);
		}
		return -1;
	}

	dbus_message_unref(reply);

	dbus_connection_flush(conn);

	return 0;
}

static int unregister_agent(DBusConnection *conn, const char *adapter_path,
							const char *agent_path)
{
	DBusMessage *msg, *reply;
	DBusError err;

	msg = dbus_message_new_method_call("org.bluez", adapter_path,
					"org.bluez.Adapter", "UnregisterAgent");
	if (!msg) {
		fprintf(stderr, "Can't allocate new method call\n");
		return -1;
	}

	dbus_message_append_args(msg, DBUS_TYPE_OBJECT_PATH, &agent_path,
							DBUS_TYPE_INVALID);

	dbus_error_init(&err);

	reply = dbus_connection_send_with_reply_and_block(conn, msg, -1, &err);

	dbus_message_unref(msg);

	if (!reply) {
		fprintf(stderr, "Can't unregister agent\n");
		if (dbus_error_is_set(&err)) {
			fprintf(stderr, "%s\n", err.message);
			dbus_error_free(&err);
		}
		return -1;
	}

	dbus_message_unref(reply);

	dbus_connection_flush(conn);

	dbus_connection_unregister_object_path(conn, agent_path);

	return 0;
}

static char *get_default_adapter_path(DBusConnection *conn)
{
	DBusMessage *msg, *reply;
	DBusError err;
	const char *reply_path;
	char *path;

	msg = dbus_message_new_method_call("org.bluez", "/",
					"org.bluez.Manager", "DefaultAdapter");

	if (!msg) {
		fprintf(stderr, "Can't allocate new method call\n");
		return NULL;
	}

	dbus_error_init(&err);

	reply = dbus_connection_send_with_reply_and_block(conn, msg, -1, &err);

	dbus_message_unref(msg);

	if (!reply) {
		fprintf(stderr,
			"Can't get default adapter\n");
		if (dbus_error_is_set(&err)) {
			fprintf(stderr, "%s\n", err.message);
			dbus_error_free(&err);
		}
		return NULL;
	}

	if (!dbus_message_get_args(reply, &err,
					DBUS_TYPE_OBJECT_PATH, &reply_path,
					DBUS_TYPE_INVALID)) {
		fprintf(stderr,
			"Can't get reply arguments\n");
		if (dbus_error_is_set(&err)) {
			fprintf(stderr, "%s\n", err.message);
			dbus_error_free(&err);
		}
		dbus_message_unref(reply);
		return NULL;
	}

	path = strdup(reply_path);

	dbus_message_unref(reply);

	dbus_connection_flush(conn);

	return path;
}

static char *get_adapter_path(DBusConnection *conn, const char *adapter)
{
	DBusMessage *msg, *reply;
	DBusError err;
	const char *reply_path;
	char *path;

	if (!adapter)
		return get_default_adapter_path(conn);

	msg = dbus_message_new_method_call("org.bluez", "/",
					"org.bluez.Manager", "FindAdapter");

	if (!msg) {
		fprintf(stderr, "Can't allocate new method call\n");
		return NULL;
	}

	dbus_message_append_args(msg, DBUS_TYPE_STRING, &adapter,
					DBUS_TYPE_INVALID);

	dbus_error_init(&err);

	reply = dbus_connection_send_with_reply_and_block(conn, msg, -1, &err);

	dbus_message_unref(msg);

	if (!reply) {
		fprintf(stderr,
			"Can't find adapter %s\n", adapter);
		if (dbus_error_is_set(&err)) {
			fprintf(stderr, "%s\n", err.message);
			dbus_error_free(&err);
		}
		return NULL;
	}

	if (!dbus_message_get_args(reply, &err,
					DBUS_TYPE_OBJECT_PATH, &reply_path,
					DBUS_TYPE_INVALID)) {
		fprintf(stderr,
			"Can't get reply arguments\n");
		if (dbus_error_is_set(&err)) {
			fprintf(stderr, "%s\n", err.message);
			dbus_error_free(&err);
		}
		dbus_message_unref(reply);
		return NULL;
	}

	path = strdup(reply_path);

	dbus_message_unref(reply);

	dbus_connection_flush(conn);

	return path;
}

int remove_paired_linkkey()
{

	DBusMessage *msg, *reply;
	DBusError err;
	const char *reply_path;

	msg = dbus_message_new_method_call("org.bluez", adapter_path,
					"org.bluez.Adapter", "FindDevice");
	if (!msg) {
		fprintf(stderr, "Can't allocate new method call\n");
		return -1;
	}

	dbus_message_append_args(msg, DBUS_TYPE_STRING, &bdaddr,
					DBUS_TYPE_INVALID);

	dbus_error_init(&err);

	reply = dbus_connection_send_with_reply_and_block(conn, msg, -1, &err);

	dbus_message_unref(msg);

	if (!reply) {
		fprintf(stderr, "Can't find device\n");
		if (dbus_error_is_set(&err)) {
			fprintf(stderr, "%s\n", err.message);
			dbus_error_free(&err);
		}
		return -1;
	}

	dbus_message_get_args(reply, &err, DBUS_TYPE_OBJECT_PATH, &reply_path, DBUS_TYPE_INVALID);
	printf("%s \n", reply_path);

	dbus_message_unref(reply);

	dbus_connection_flush(conn);

	//new msg

	msg = dbus_message_new_method_call("org.bluez", adapter_path,
					"org.bluez.Adapter", "RemoveDevice");
	if (!msg) {
		fprintf(stderr, "Can't allocate new method call\n");
		return -1;
	}

	dbus_message_append_args(msg, DBUS_TYPE_OBJECT_PATH, &reply_path,
					DBUS_TYPE_INVALID);

	dbus_error_init(&err);

	reply = dbus_connection_send_with_reply_and_block(conn, msg, -1, &err);

	dbus_message_unref(msg);

	if (!reply) {
		fprintf(stderr, "Can't remove device\n");
		if (dbus_error_is_set(&err)) {
			fprintf(stderr, "%s\n", err.message);
			dbus_error_free(&err);
		}
		return -1;
	}

	dbus_message_unref(reply);

	dbus_connection_flush(conn);

	return 0;
}

void * agent_run(void * arg)
{

	snprintf(default_path, sizeof(default_path),
					"/org/bluez/agent_%d", getpid());

	agent_path = strdup(default_path);

	conn = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);
	if (!conn) {
		fprintf(stderr, "Can't get on system bus");
		return NULL;
	}

	adapter_path = get_adapter_path(conn, adapter_id);
	if (!adapter_path)
		return NULL;

	if (!dbus_connection_register_object_path(conn, agent_path,
							&agent_table, NULL)) {
		fprintf(stderr, "Can't register object path for agent\n");
		return NULL;
	}

	if (register_agent(conn, adapter_path, agent_path,
							"DisplayYesNo") < 0) {
			dbus_connection_unref(conn);
			return NULL;
	}


	if (!dbus_connection_add_filter(conn, agent_filter, NULL, NULL))
		fprintf(stderr, "Can't add signal filter");

	snprintf(match_string, sizeof(match_string),
			"interface=%s,member=NameOwnerChanged,arg0=%s",
			DBUS_INTERFACE_DBUS, "org.bluez");

	dbus_bus_add_match(conn, match_string, NULL);

	agent_status = 1;


	while (!agent_done && !__io_terminated) {
		if (dbus_connection_read_write_dispatch(conn, 500) != TRUE)
			break;
		if(remove_linkkey)
		{
			remove_paired_linkkey();
			agent_status = 1;
			remove_linkkey = 0;
		}
	}

	agent_status = 0;

	if (!__io_terminated)
		unregister_agent(conn, adapter_path, agent_path);

	printf("Agent unregistered");

	free(adapter_path);
	free(agent_path);

	dbus_connection_unref(conn);

	return NULL;
}
