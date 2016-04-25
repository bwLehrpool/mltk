#include <dbus/dbus.h>
#include <sys/types.h>

struct _CkConnector;
typedef struct _CkConnector CkConnector;


struct _CkConnector
{
	int             refcount;
	char           *cookie;
	dbus_bool_t     session_created;
	DBusConnection *connection;
};

void ck_connector_unref (CkConnector *connector)
{
	return;
}

CkConnector * ck_connector_ref (CkConnector *connector)
{
	return connector;
}

CkConnector * ck_connector_new (void)
{
	return (CkConnector*)1;
}

dbus_bool_t ck_connector_open_session (CkConnector *connector, DBusError   *error)
{
	return 1;
}

dbus_bool_t ck_connector_open_session_with_parameters (CkConnector *connector, DBusError *error, const char *first_parameter_name, ...)
{
	return 1;
}

dbus_bool_t ck_connector_open_session_for_user (CkConnector *connector, uid_t user, const char *display_device, const char *x11_display, DBusError  *error)
{
	return 1;
}

const char * ck_connector_get_cookie (CkConnector *connector)
{
	return NULL;
}

dbus_bool_t ck_connector_close_session (CkConnector *connector, DBusError *error)
{
	return 1;
}

