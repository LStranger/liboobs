/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* Copyright (C) 2005 Carlos Garnacho
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: Carlos Garnacho Parro  <carlosg@gnome.org>
 */

#include <dbus/dbus.h>
#include <glib-object.h>
#include "oobs-object.h"
#include "oobs-object-private.h"
#include "oobs-session.h"
#include "oobs-session-private.h"
#include "oobs-error.h"
#include "utils.h"

/**
 * SECTION:oobs-object
 * @title: OobsObject
 * @short_description: Base object for all configuration objects
 * @see_also: #OobsList
 **/

#define OOBS_OBJECT_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_OBJECT, OobsObjectPrivate))

typedef struct _OobsObjectPrivate OobsObjectPrivate;
typedef struct _OobsObjectAsyncCallbackData OobsObjectAsyncCallbackData;

struct _OobsObjectPrivate
{
  OobsSession *session;
  DBusError    dbus_error;

  gchar       *remote_object;
  gchar       *path;
  gchar       *method;

  GList       *pending_calls;

  guint        update_requests;
  guint        updated : 1;
};

struct _OobsObjectAsyncCallbackData
{
  OobsObject *object;
  gboolean update;
  OobsObjectAsyncFunc func;
  gpointer data;
};

enum _OobsObjectCommitMethod
{
  METHOD_COMMIT,
  METHOD_ADD,
  METHOD_DELETE
};
typedef enum _OobsObjectCommitMethod _OobsObjectCommitMethod;

static void oobs_object_class_init (OobsObjectClass *class);
static void oobs_object_init       (OobsObject      *object);
static void oobs_object_finalize   (GObject         *object);

static GObject * oobs_object_constructor (GType                  type,
					  guint                  n_construct_properties,
					  GObjectConstructParam *construct_params);

static void oobs_object_set_property (GObject       *object,
				      guint          prop_id,
				      const GValue  *value,
				      GParamSpec    *pspec);
static void oobs_object_get_property (GObject       *object,
				      guint          prop_id,
				      GValue        *value,
				      GParamSpec    *pspec);

static void connect_object_to_session (OobsObject *object);

enum
{
  UPDATED,
  COMMITTED,
  CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_REMOTE_OBJECT
};

static GQuark dbus_connection_quark;

static guint object_signals [LAST_SIGNAL] = { 0 };

G_DEFINE_ABSTRACT_TYPE (OobsObject, oobs_object, G_TYPE_OBJECT);

static void
oobs_object_class_init (OobsObjectClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->constructor  = oobs_object_constructor;
  object_class->get_property = oobs_object_get_property;
  object_class->set_property = oobs_object_set_property;
  object_class->finalize     = oobs_object_finalize;

  /* some object types don't need to be singletons, they override this */
  class->singleton = TRUE;

  dbus_connection_quark = g_quark_from_static_string ("oobs-dbus-connection");

  g_object_class_install_property (object_class,
				   PROP_REMOTE_OBJECT,
				   g_param_spec_string ("remote-object",
							"Remote object to deal with",
							"Name of the remote object at the other side of the connection",
							NULL,
							G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

  object_signals [UPDATED] = g_signal_new ("updated",
					   G_OBJECT_CLASS_TYPE (object_class),
					   G_SIGNAL_RUN_LAST,
					   G_STRUCT_OFFSET (OobsObjectClass, updated),
					   NULL, NULL,
					   g_cclosure_marshal_VOID__VOID,
					   G_TYPE_NONE, 0);
  object_signals [COMMITTED] = g_signal_new ("committed",
					     G_OBJECT_CLASS_TYPE (object_class),
					     G_SIGNAL_RUN_LAST,
					     G_STRUCT_OFFSET (OobsObjectClass, committed),
					     NULL, NULL,
					     g_cclosure_marshal_VOID__VOID,
					     G_TYPE_NONE, 0);
  object_signals [CHANGED] = g_signal_new ("changed",
					   G_OBJECT_CLASS_TYPE (object_class),
					   G_SIGNAL_RUN_LAST,
					   G_STRUCT_OFFSET (OobsObjectClass, changed),
					   NULL, NULL,
					   g_cclosure_marshal_VOID__VOID,
					   G_TYPE_NONE, 0);

  g_type_class_add_private (object_class,
			    sizeof (OobsObjectPrivate));
}

static void
oobs_object_init (OobsObject *object)
{
  OobsObjectPrivate *priv;

  priv = OOBS_OBJECT_GET_PRIVATE (object);

  /* We need session to stay alive until object is destroyed */
  priv->session = oobs_session_get ();
  g_object_ref (priv->session);
  priv->remote_object = NULL;
  dbus_error_init (&priv->dbus_error);

  object->_priv = priv;
}

static void
oobs_object_finalize (GObject *object)
{
  OobsObject *obj;
  OobsObjectPrivate *priv;

  g_return_if_fail (OOBS_IS_OBJECT (object));

  obj  = OOBS_OBJECT (object);
  priv = OOBS_OBJECT (object)->_priv;

  /* Cancel all pending calls, they're going to be orphaned soon */
  g_list_foreach (priv->pending_calls, (GFunc) dbus_pending_call_cancel, NULL);
  g_list_foreach (priv->pending_calls, (GFunc) dbus_pending_call_unref, NULL);
  g_list_free (priv->pending_calls);

  _oobs_session_unregister_object (priv->session, obj);

  /* Only now, we don't care about session */
  g_object_unref (priv->session);

  g_free (priv->remote_object);
  g_free (priv->path);
  g_free (priv->method);

  if (G_OBJECT_CLASS (oobs_object_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_object_parent_class)->finalize) (object);
}

static GObject *
oobs_object_constructor (GType                  type,
			 guint                  n_construct_properties,
			 GObjectConstructParam *construct_params)
{
  OobsObjectClass *class;
  static GHashTable *type_table;
  GObject *object;

  /* some objects that inherit from OobsObject have to be
   * singletons, keep a hash table with references to them
   */
  if (!type_table)
    type_table = g_hash_table_new (g_direct_hash, g_direct_equal);

  object = g_hash_table_lookup (type_table, GINT_TO_POINTER (type));

  if (object)
    {
      class = OOBS_OBJECT_GET_CLASS (object);

      /* if has to be singleton and exists, return it */
      if (class->singleton)
	return object;
    }

  /* this class should not be limited to singletons, create (possibly new) object anyway */
  object = (* G_OBJECT_CLASS (oobs_object_parent_class)->constructor) (type,
                                                                       n_construct_properties,
                                                                       construct_params);
  g_hash_table_insert (type_table, GINT_TO_POINTER (type), object);
  connect_object_to_session (OOBS_OBJECT (object));

  return object;
}

static gboolean
object_changed_idle (gpointer data)
{
  OobsObject *object;

  object = OOBS_OBJECT (data);

  g_signal_emit (object, object_signals [CHANGED], 0);

  return FALSE;
}

static DBusHandlerResult
changed_signal_filter (DBusConnection *connection,
		       DBusMessage    *message,
		       void           *user_data)
{
  OobsObject        *object;
  OobsObjectPrivate *priv;

  object = OOBS_OBJECT (user_data);
  priv   = OOBS_OBJECT (object)->_priv;

  if (dbus_message_is_signal (message, priv->method, "changed") &&
      dbus_message_has_path (message, priv->path))
    g_idle_add (object_changed_idle, object);

  /* we want the rest of the objects of
   * the same type to get the signal too
   */
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static void
connect_object_to_session (OobsObject *object)
{
  OobsObjectPrivate *priv;
  DBusConnection    *connection;
  gchar             *rule;

  priv = OOBS_OBJECT (object)->_priv;
  connection = _oobs_session_get_connection_bus (priv->session);

  if (!connection)
    {
      g_warning ("OobsSession object hasn't connected to the bus, cannot register OobsObject");
      return;
    }

  _oobs_session_register_object (priv->session, object);
  dbus_connection_add_filter (connection, changed_signal_filter, object, NULL);

  rule = g_strdup_printf ("type='signal',interface='%s',path='%s'",
			  priv->method, priv->path);
  dbus_bus_add_match (connection, rule, &priv->dbus_error);

  if (dbus_error_is_set (&priv->dbus_error))
    {
      g_critical ("There was an error adding the match function: %s", priv->dbus_error.message);
      dbus_error_free (&priv->dbus_error);
    }

  g_free (rule);
}

static void
oobs_object_set_property (GObject      *object,
			  guint         prop_id,
			  const GValue *value,
			  GParamSpec   *pspec)
{
  OobsObject *obj;
  OobsObjectPrivate *priv;

  g_return_if_fail (OOBS_IS_OBJECT (object));

  obj  = OOBS_OBJECT (object);
  priv = obj->_priv;

  switch (prop_id)
    {
    case PROP_REMOTE_OBJECT:
      priv->remote_object = g_value_dup_string (value);
      priv->path   = g_strconcat (OOBS_DBUS_PATH_PREFIX, "/", priv->remote_object, NULL);
      priv->method = g_strdup (OOBS_DBUS_METHOD_PREFIX);
      break;
    }
}

static void
oobs_object_get_property (GObject      *object,
			  guint         prop_id,
			  GValue       *value,
			  GParamSpec   *pspec)
{
  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

DBusMessage*
_oobs_object_get_dbus_message (OobsObject *object)
{
  return (DBusMessage *) g_object_get_qdata (G_OBJECT (object), dbus_connection_quark);
}

void
_oobs_object_set_dbus_message (OobsObject *object, DBusMessage *message)
{
  g_object_set_qdata_full (G_OBJECT (object), dbus_connection_quark,
			   message, (GDestroyNotify) dbus_message_unref);
}

static OobsResult
update_object_from_message (OobsObject  *object,
			    DBusMessage *message)
{
  OobsObjectPrivate *priv;
  OobsObjectClass *class;

  class = OOBS_OBJECT_GET_CLASS (object);

  if (G_UNLIKELY (!class->update))
    {
      g_critical ("There is no update() implementation for this object");
      return OOBS_RESULT_MALFORMED_DATA;
    }

  priv = object->_priv;
  priv->updated = TRUE;

  if (priv->update_requests == 0)
    g_critical ("update requests count already reached 0");
  else
    priv->update_requests--;

  g_object_set_qdata (G_OBJECT (object), dbus_connection_quark, message);
  class->update (object);
  g_object_steal_qdata (G_OBJECT (object), dbus_connection_quark);

  g_signal_emit (object, object_signals [UPDATED], 0);

  return OOBS_RESULT_OK;
}

static DBusMessage*
run_message (OobsObject  *object,
	     DBusMessage *message,
	     OobsResult  *result)
{
  OobsObjectPrivate *priv;
  DBusConnection    *connection;
  DBusMessage       *reply;

  priv = object->_priv;

  if (!oobs_session_get_connected (priv->session))
    {
      g_warning ("Could not send message, OobsSession hasn't connected to the bus");
      return NULL;
    }

  connection = _oobs_session_get_connection_bus (priv->session);
  reply = dbus_connection_send_with_reply_and_block (connection, message, -1, &priv->dbus_error);

  if (dbus_error_is_set (&priv->dbus_error))
    {
      if (dbus_error_has_name (&priv->dbus_error, DBUS_ERROR_ACCESS_DENIED))
	*result = OOBS_RESULT_ACCESS_DENIED;
      else
	g_warning ("There was an unknown error communicating with the backends: %s", priv->dbus_error.message);

      dbus_error_free (&priv->dbus_error);
      return NULL;
    }

  *result = OOBS_RESULT_OK;
  return reply;
}

static void
async_message_cb (DBusPendingCall *pending_call, gpointer data)
{
  OobsObjectPrivate *priv;
  OobsObjectAsyncCallbackData *async_data;
  OobsResult result = OOBS_RESULT_MALFORMED_DATA;
  DBusMessage *reply;
  DBusError error;

  dbus_error_init (&error);
  async_data = (OobsObjectAsyncCallbackData*) data;
  reply = dbus_pending_call_steal_reply (pending_call);

  if (dbus_set_error_from_message (&error, reply))
    {
      if (dbus_error_has_name (&error, DBUS_ERROR_ACCESS_DENIED))
	result = OOBS_RESULT_ACCESS_DENIED;
      else
	{
	  /* FIXME: process error */
	  result = OOBS_RESULT_MALFORMED_DATA;
	}
      g_warning ("There was an unknown error communicating asynchronously with the backends: %s", error.message);

      dbus_error_free (&error);
    }
  else
    {
      if (async_data->update)
	result = update_object_from_message (OOBS_OBJECT (async_data->object), reply);
      else
	{
	  g_signal_emit (async_data->object, object_signals [COMMITTED], 0);
	  result = OOBS_RESULT_OK;
	}
    }

  priv = async_data->object->_priv;
  priv->pending_calls = g_list_remove (priv->pending_calls, pending_call);

  if (async_data->func)
    (* async_data->func) (OOBS_OBJECT (async_data->object), result, async_data->data);

  dbus_message_unref (reply);
  g_object_unref (async_data->object);
  dbus_pending_call_unref (pending_call);
}

static void
run_message_async (OobsObject          *object,
		   DBusMessage         *message,
		   gboolean             update,
		   OobsObjectAsyncFunc  func,
		   gpointer             data)
{
  OobsObjectPrivate *priv;
  DBusPendingCall *call;
  OobsObjectAsyncCallbackData *async_data;
  DBusConnection *connection;

  priv = object->_priv;

  if (!oobs_session_get_connected (priv->session))
    {
      g_warning ("could not send message, OobsSession hasn't connected to the bus");
      return;
    }

  connection = _oobs_session_get_connection_bus (priv->session);
  dbus_connection_send_with_reply (connection, message, &call, -1);

  async_data = g_new0 (OobsObjectAsyncCallbackData, 1);
  async_data->object = g_object_ref (object);
  async_data->update = update;
  async_data->func = func;
  async_data->data = data;

  dbus_pending_call_set_notify (call, async_message_cb, async_data, g_free);
  priv->pending_calls = g_list_prepend (priv->pending_calls, call);
}

static DBusMessage*
get_commit_message (_OobsObjectCommitMethod method, OobsObject *object)
{
  OobsObjectClass   *class;
  OobsObjectPrivate *priv;
  DBusMessage       *message;
  gchar *suffix;

  priv  = object->_priv;
  class = OOBS_OBJECT_GET_CLASS (object);

  if (!priv->session)
    {
      g_critical ("Trying to commit changes after the session has terminated, "
		  "this reflects a bug in the application");
      return NULL;
    }


  switch (method)
    {
      case METHOD_COMMIT:
	suffix = "set";
	break;
      case METHOD_ADD:
	suffix = "add";
	break;
      case METHOD_DELETE:
	suffix = "del";
	break;
      default:
	g_critical ("Unknown commit method");
        return NULL;
    }

  if (!class->commit)
    {
      g_critical ("There is no commit() implementation for this object");
      return NULL;
    }

  message = dbus_message_new_method_call (OOBS_DBUS_DESTINATION, priv->path, priv->method, suffix);

  /* Let the commit() implementation fill the message */
  _oobs_object_set_dbus_message (object, message);
  class->commit (object);
  message = g_object_steal_qdata (G_OBJECT (object), dbus_connection_quark);

  if (!message)
    {
      /* a NULL message means malformed configuration */
      g_critical ("Not committing due to inconsistencies in the "
		  "configuration, this reflects a bug in the application\n");
    }

  return message;
}

static DBusMessage*
get_update_message (OobsObject *object)
{
  OobsObjectPrivate *priv;

  priv = object->_priv;

  if (!priv->session)
    {
      g_critical ("Trying to update the object after the session has terminated, "
		  "this reflects a bug in the application");
      return NULL;
    }

  return dbus_message_new_method_call (OOBS_DBUS_DESTINATION, priv->path, priv->method, "get");
}

/*
 * Do the real work for oobs_object_commit() oobs_object_add() and oobs_object_delete().
 */
static OobsResult
do_commit (_OobsObjectCommitMethod method, OobsObject *object)
{
  OobsObjectPrivate *priv;
  DBusMessage *message;
  DBusMessage *reply;
  DBusMessageIter iter;
  OobsResult result;

  g_return_val_if_fail (OOBS_IS_OBJECT (object), OOBS_RESULT_MALFORMED_DATA);

  message = get_commit_message (method, object);

  if (!message)
    return OOBS_RESULT_MALFORMED_DATA;

  reply = run_message (object, message, &result);
  dbus_message_unref (message);

  /* Objects can use this to update themselves when the backend provides it */
  if (reply)
    {
      dbus_message_iter_init (reply, &iter);
      if (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_STRUCT)
	{
	  priv = object->_priv;

	  priv->update_requests++;
	  result = update_object_from_message (object, reply);
	  dbus_message_unref (reply);
	}
    }

  g_signal_emit (object, object_signals [COMMITTED], 0);

  return result;
}

/*
 * Do the real work for oobs_object_commit_async(),
 * oobs_object_add_async() and oobs_object_delete_async().
 */
static OobsResult
do_commit_async (_OobsObjectCommitMethod method,
                 OobsObject             *object,
                 OobsObjectAsyncFunc     func,
                 gpointer                data)
{
  DBusMessage *message;

  g_return_val_if_fail (OOBS_IS_OBJECT (object), OOBS_RESULT_MALFORMED_DATA);

  message = get_commit_message (method, object);

  if (!message)
    return OOBS_RESULT_MALFORMED_DATA;

  run_message_async (object, message, FALSE, func, data);
  dbus_message_unref (message);

  return OOBS_RESULT_OK;
}

/**
 * oobs_object_commit:
 * @object: an #OobsObject
 *
 * Commits to the system all the changes done
 * to the configuration held by an #OobsObject.
 *
 * Return value: an #OobsResult enum with the error code.
 **/
OobsResult
oobs_object_commit (OobsObject *object)
{
  return do_commit (METHOD_COMMIT, object);
}

/**
 * oobs_object_commit_async:
 * @object: An #OobsObject.
 * @func: An #OobsObjectAsyncFunc that will be called when the asynchronous operation has ended.
 * @data: Additional data to pass to @func.
 *
 * Commits to the system all the changes done to the configuration held by an #OobsObject.
 * This change will be asynchronous, being run the function @func when the change has been done.
 *
 * Return value: an #OobsResult enum with the error code. Due to the asynchronous nature
 * of the function, only OOBS_RESULT_MALFORMED and OOBS_RESULT_OK can be returned.
 **/
OobsResult
oobs_object_commit_async (OobsObject          *object,
			  OobsObjectAsyncFunc  func,
			  gpointer             data)
{
  return do_commit_async (METHOD_COMMIT, object, func, data);
}

/**
 * oobs_object_add:
 * @object: an #OobsObject
 *
 * Add an #OobsObject to the system's configuration. This is only
 * supported by certain individual objects, others are added by committing
 * a modified objects list.
 *
 * Return value: an #OobsResult enum with the error code.
 **/
OobsResult
oobs_object_add (OobsObject *object)
{
  return do_commit (METHOD_ADD, object);
}

/**
 * oobs_object_add_async:
 * @object: An #OobsObject.
 * @func: An #OobsObjectAsyncFunc that will be called when the asynchronous operation has ended.
 * @data: Additional data to pass to @func.
 *
 * Add an #OobsObject to the system's configuration.
 * This change will be asynchronous, being run the function @func when the change has been done.
 *
 * Return value: an #OobsResult enum with the error code. Due to the asynchronous nature
 * of the function, only OOBS_RESULT_MALFORMED and OOBS_RESULT_OK can be returned.
 **/
OobsResult
oobs_object_add_async (OobsObject          *object,
                       OobsObjectAsyncFunc  func,
                       gpointer             data)
{
  return do_commit_async (METHOD_ADD, object, func, data);
}

/**
 * oobs_object_delete:
 * @object: an #OobsObject
 *
 * Delete an #OobsObject from the system's configuration. This is only
 * supported by certain individual objects, others are added by committing
 * a modified objects list.
 *
 * Return value: an #OobsResult enum with the error code.
 **/
OobsResult
oobs_object_delete (OobsObject *object)
{
  return do_commit (METHOD_DELETE, object);
}

/**
 * oobs_object_delete_async:
 * @object: An #OobsObject.
 * @func: An #OobsObjectAsyncFunc that will be called when the asynchronous operation has ended.
 * @data: Additional data to pass to @func.
 *
 * Delete an #OobsObject from the system's configuration.
 * This change will be asynchronous, being run the function @func when the change has been done.
 *
 * Return value: an #OobsResult enum with the error code. Due to the asynchronous nature
 * of the function, only OOBS_RESULT_MALFORMED and OOBS_RESULT_OK can be returned.
 **/
OobsResult
oobs_object_delete_async (OobsObject          *object,
                          OobsObjectAsyncFunc  func,
                          gpointer             data)
{
  return do_commit_async (METHOD_DELETE, object, func, data);
}

/**
 * oobs_object_update:
 * @object: an #OobsObject
 *
 * Synchronizes the configuration held by the #OobsObject
 * with the actual system configuration. All the changes done
 * to the configuration held by the #OobsObject will be forgotten.
 *
 * Return value: an #OobsResult enum with the error code.
 **/
OobsResult
oobs_object_update (OobsObject *object)
{
  OobsObjectPrivate *priv;
  DBusMessage *message, *reply;
  OobsResult result = OOBS_RESULT_MALFORMED_DATA;

  g_return_val_if_fail (OOBS_IS_OBJECT (object), OOBS_RESULT_MALFORMED_DATA);

  priv = object->_priv;
  message = get_update_message (object);

  if (!message)
    return OOBS_RESULT_MALFORMED_DATA;

  priv->update_requests++;
  reply = run_message (object, message, &result);

  if (reply)
    {
      result = update_object_from_message (object, reply);
      dbus_message_unref (reply);
    }

  dbus_message_unref (message);
  return result;
}

/**
 * oobs_object_update_async:
 * @object: An #OobsObject
 * @func: An #OobsObjectAsyncFunc that will be called when the asynchronous operation has ended.
 * @data: Aditional data to pass to @func.
 * 
 * Synchronizes the configuration held by the #OobsObject
 * with the actual system configuration. All the changes done
 * to the configuration held by the #OobsObject will be forgotten.
 * The update operation will be asynchronous, being run the
 * function @func when the update has been done.
 * 
 * Return value: an #OobsResult enum with the error code. Due to the asynchronous nature
 * of the function, only OOBS_RESULT_MALFORMED and OOBS_RESULT_OK can be returned.
 **/
OobsResult
oobs_object_update_async (OobsObject          *object,
			  OobsObjectAsyncFunc  func,
			  gpointer             data)
{
  OobsObjectPrivate *priv;
  DBusMessage *message;

  priv = object->_priv;
  message = get_update_message (object);

  if (!message)
    return OOBS_RESULT_MALFORMED_DATA;

  priv->update_requests++;
  run_message_async (object, message, TRUE, func, data);
  dbus_message_unref (message);

  return OOBS_RESULT_OK;
}

/**
 * oobs_object_process_requests:
 * @object: An #OobsObject
 * 
 * Blocks until all pending asynchronous requests to this object have been processed.
 **/
void
oobs_object_process_requests (OobsObject *object)
{
  OobsObjectPrivate *priv;

  g_return_if_fail (OOBS_IS_OBJECT (object));
  priv = object->_priv;
  g_list_foreach (priv->pending_calls, (GFunc) dbus_pending_call_block, NULL);
}

/**
 * oobs_object_has_updated:
 * @object: An #OobsObject
 *
 * Returns whether the object has been updated since its creation.
 * see oobs_object_update() and oobs_object_update_async().
 *
 * Return Value: #TRUE if the object has been updated.
 **/
gboolean
oobs_object_has_updated (OobsObject *object)
{
  OobsObjectPrivate *priv;

  g_return_val_if_fail (OOBS_IS_OBJECT (object), FALSE);

  priv = object->_priv;
  return priv->updated;
}

/**
 * oobs_object_ensure_update:
 * @object: An #OobsObject
 *
 * Ensures that the given object has been updated. If not
 * it either blocks until any update request sent is
 * dispatched or updates synchronously.
 **/
void
oobs_object_ensure_update (OobsObject *object)
{
  OobsObjectPrivate *priv;

  g_return_if_fail (OOBS_IS_OBJECT (object));

  if (oobs_object_has_updated (object))
    return;

  priv = object->_priv;

  if (priv->update_requests > 0)
    {
      /* it's in the middle of
       * asynchronous update request(s)
       */
      oobs_object_process_requests (object);
    }
  else
    oobs_object_update (object);
}

/**
 * oobs_object_authenticate:
 * @object: An #OobsObject.
 * @error: Return location for error or NULL.
 *
 * Performs a PolicyKit authentication via the backends for the action
 * required by the given object. User interaction will occur synchronously
 * if needed.
 *
 * You may want to check the returned error for %OOBS_ERROR_AUTHENTICATION_CANCELLED,
 * in which case you should avoid showing an error dialog to the user.
 *
 * Return Value: %TRUE if allowed to commit @object, %FALSE otherwise.
 **/
gboolean
oobs_object_authenticate (OobsObject *object,
                          GError    **error)
{
  OobsObjectPrivate *priv;
  DBusConnection    *connection;
  DBusMessage       *message;
  DBusMessage       *reply;
  DBusMessageIter   iter;
  gboolean result;

  g_return_val_if_fail (OOBS_IS_OBJECT (object), FALSE);

  priv = OOBS_OBJECT_GET_PRIVATE (object);

  message = dbus_message_new_method_call (OOBS_DBUS_DESTINATION, priv->path,
                                          "org.freedesktop.SystemToolsBackends.Authentication",
                                          "authenticate");

  if (!oobs_session_get_connected (priv->session))
    {
      g_warning ("Could not send message, OobsSession hasn't connected to the bus");
      return FALSE;
    }

  connection = _oobs_session_get_connection_bus (priv->session);
  reply = dbus_connection_send_with_reply_and_block (connection, message, -1, &priv->dbus_error);

  if (dbus_error_is_set (&priv->dbus_error))
    {
      if (dbus_error_has_name (&priv->dbus_error,
                               "org.freedesktop.SystemToolsBackends.AuthenticationCancelled"))
	g_set_error_literal (error, OOBS_ERROR,
	                     OOBS_ERROR_AUTHENTICATION_CANCELLED,
	                     priv->dbus_error.message);
      else
	g_set_error_literal (error, OOBS_ERROR,
	                     OOBS_ERROR_AUTHENTICATION_FAILED,
	                     priv->dbus_error.message);

      dbus_error_free (&priv->dbus_error);
      return FALSE;
    }

  dbus_message_iter_init (reply, &iter);
  result = utils_get_boolean (&iter);

  return result;

}
