/*
 * Copyright (C) 2015 Ole André Vadla Ravnås <oleavr@nowsecure.com>
 *
 * Licence: wxWindows Library Licence, Version 3.1
 */

#include "gumjscriptvalue.h"

#include "gumjscript-priv.h"

#define GUM_SCRIPT_MAX_ARRAY_LENGTH (1024 * 1024)

gint
_gumjs_int_from_value (JSContextRef ctx,
                       JSValueRef value)
{
  gint i;
  JSValueRef exception;

  if (!_gumjs_try_int_from_value (ctx, value, &i, &exception))
    _gumjs_panic (ctx, exception);

  return i;
}

gboolean
_gumjs_try_int_from_value (JSContextRef ctx,
                           JSValueRef value,
                           gint * i,
                           JSValueRef * exception)
{
  JSValueRef ex = NULL;
  double number;

  number = JSValueToNumber (ctx, value, &ex);
  if (ex == NULL)
    *i = (gint) number;

  if (exception != NULL)
    *exception = ex;

  return ex == NULL;
}

gchar *
_gumjs_string_get (JSStringRef str)
{
  gsize size;
  gchar * result;

  size = JSStringGetMaximumUTF8CStringSize (str);
  result = g_malloc (size);
  JSStringGetUTF8CString (str, result, size);

  return result;
}

gchar *
_gumjs_string_from_value (JSContextRef ctx,
                          JSValueRef value)
{
  gchar * str;
  JSValueRef exception;

  if (!_gumjs_try_string_from_value (ctx, value, &str, &exception))
    _gumjs_panic (ctx, exception);

  return str;
}

gboolean
_gumjs_try_string_from_value (JSContextRef ctx,
                              JSValueRef value,
                              gchar ** str,
                              JSValueRef * exception)
{
  JSStringRef s;

  s = JSValueToStringCopy (ctx, value, exception);
  if (s == NULL)
    return FALSE;
  *str = _gumjs_string_get (s);
  JSStringRelease (s);

  return TRUE;
}

JSValueRef
_gumjs_string_to_value (JSContextRef ctx,
                        const gchar * str)
{
  JSValueRef result;
  JSStringRef str_js;

  str_js = JSStringCreateWithUTF8CString (str);
  result = JSValueMakeString (ctx, str_js);
  JSStringRelease (str_js);

  return result;
}

JSValueRef
_gumjs_object_get (JSContextRef ctx,
                   JSObjectRef object,
                   const gchar * key)
{
  JSValueRef value, exception;

  if (!_gumjs_object_try_get (ctx, object, key, &value, &exception))
    _gumjs_panic (ctx, exception);

  return value;
}

gboolean
_gumjs_object_try_get (JSContextRef ctx,
                       JSObjectRef object,
                       const gchar * key,
                       JSValueRef * value,
                       JSValueRef * exception)
{
  JSStringRef property;
  JSValueRef ex = NULL;

  property = JSStringCreateWithUTF8CString (key);
  *value = JSObjectGetProperty (ctx, object, property, &ex);
  JSStringRelease (property);

  if (exception != NULL)
    *exception = ex;

  return ex == NULL;
}

guint
_gumjs_object_get_uint (JSContextRef ctx,
                        JSObjectRef object,
                        const gchar * key)
{
  guint value;
  JSValueRef exception;

  if (!_gumjs_object_try_get_uint (ctx, object, key, &value, &exception))
    _gumjs_panic (ctx, exception);

  return value;
}

gboolean
_gumjs_object_try_get_uint (JSContextRef ctx,
                            JSObjectRef object,
                            const gchar * key,
                            guint * value,
                            JSValueRef * exception)
{
  JSValueRef v, ex = NULL;
  double number;

  if (!_gumjs_object_try_get (ctx, object, key, &v, exception))
    return FALSE;

  if (!JSValueIsNumber (ctx, v))
    goto invalid_type;

  number = JSValueToNumber (ctx, v, &ex);
  if (ex != NULL)
    goto propagate_exception;

  *value = (guint) number;
  return TRUE;

invalid_type:
  {
    _gumjs_throw (ctx, exception, "expected '%s' to be a number", key);
    return FALSE;
  }
propagate_exception:
  {
    if (exception != NULL)
      *exception = ex;
    return FALSE;
  }
}

gchar *
_gumjs_object_get_string (JSContextRef ctx,
                          JSObjectRef object,
                          const gchar * key)
{
  gchar * value;
  JSValueRef exception;

  if (!_gumjs_object_try_get_string (ctx, object, key, &value, &exception))
    _gumjs_panic (ctx, exception);

  return value;
}

gboolean
_gumjs_object_try_get_string (JSContextRef ctx,
                              JSObjectRef object,
                              const gchar * key,
                              gchar ** value,
                              JSValueRef * exception)
{
  JSValueRef v;

  if (!_gumjs_object_try_get (ctx, object, key, &v, exception))
    return FALSE;

  if (!JSValueIsString (ctx, v))
    goto invalid_type;

  return _gumjs_try_string_from_value (ctx, v, value, exception);

invalid_type:
  {
    _gumjs_throw (ctx, exception, "expected '%s' to be a string", key);
    return FALSE;
  }
}

void
_gumjs_object_set (JSContextRef ctx,
                   JSObjectRef object,
                   const gchar * key,
                   JSValueRef value)
{
  JSValueRef exception;

  if (!_gumjs_object_try_set (ctx, object, key, value, &exception))
    _gumjs_panic (ctx, exception);
}

gboolean
_gumjs_object_try_set (JSContextRef ctx,
                       JSObjectRef object,
                       const gchar * key,
                       JSValueRef value,
                       JSValueRef * exception)
{
  JSStringRef property;
  JSValueRef ex = NULL;

  property = JSStringCreateWithUTF8CString (key);
  JSObjectSetProperty (ctx, object, property, value,
      kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete, &ex);
  JSStringRelease (property);

  if (exception != NULL)
    *exception = ex;

  return ex == NULL;
}

void
_gumjs_object_set_string (JSContextRef ctx,
                          JSObjectRef object,
                          const gchar * key,
                          const gchar * value)
{
  JSValueRef exception;

  if (!_gumjs_object_try_set_string (ctx, object, key, value, &exception))
    _gumjs_panic (ctx, exception);
}

gboolean
_gumjs_object_try_set_string (JSContextRef ctx,
                              JSObjectRef object,
                              const gchar * key,
                              const gchar * value,
                              JSValueRef * exception)
{
  return _gumjs_object_try_set (ctx, object, key,
      _gumjs_string_to_value (ctx, value), exception);
}

void
_gumjs_object_set_function (JSContextRef ctx,
                            JSObjectRef object,
                            const gchar * key,
                            JSObjectCallAsFunctionCallback callback)
{
  JSValueRef exception;

  if (!_gumjs_object_try_set_function (ctx, object, key, callback, &exception))
    _gumjs_panic (ctx, exception);
}

gboolean
_gumjs_object_try_set_function (JSContextRef ctx,
                                JSObjectRef object,
                                const gchar * key,
                                JSObjectCallAsFunctionCallback callback,
                                JSValueRef * exception)
{
  JSStringRef name;
  JSObjectRef func;
  JSValueRef ex = NULL;

  name = JSStringCreateWithUTF8CString (key);
  func = JSObjectMakeFunctionWithCallback (ctx, name, callback);
  JSObjectSetProperty (ctx, object, name, func,
      kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete, &ex);
  JSStringRelease (name);

  if (exception != NULL)
    *exception = ex;

  return ex == NULL;
}

gboolean
_gumjs_callbacks_try_get (JSContextRef ctx,
                          JSValueRef callbacks,
                          const gchar * name,
                          JSObjectRef * callback,
                          JSValueRef * exception)
{
  if (!_gumjs_callbacks_try_get_opt (ctx, callbacks, name, callback, exception))
    return FALSE;

  if (*callback == NULL)
    goto callback_required;

  return TRUE;

callback_required:
  {
    _gumjs_throw (ctx, exception, "'%s' callback required", name);
    return FALSE;
  }
}

gboolean
_gumjs_callbacks_try_get_opt (JSContextRef ctx,
                              JSValueRef callbacks,
                              const gchar * name,
                              JSObjectRef * callback,
                              JSValueRef * exception)
{
  JSObjectRef obj;
  JSValueRef value;

  if (!JSValueIsObject (ctx, callbacks))
    goto invalid_argument;
  obj = (JSObjectRef) callbacks;

  if (!_gumjs_object_try_get (ctx, obj, name, &value, exception))
    return FALSE;

  return _gumjs_callback_try_get_opt (ctx, value, callback, exception);

invalid_argument:
  {
    _gumjs_throw (ctx, exception, "expected object containing callbacks");
    return FALSE;
  }
}

gboolean
_gumjs_callback_try_get (JSContextRef ctx,
                         JSValueRef value,
                         JSObjectRef * callback,
                         JSValueRef * exception)
{
  if (!_gumjs_callback_try_get_opt (ctx, value, callback, exception))
    return FALSE;

  if (*callback == NULL)
    goto callback_required;

  return TRUE;

callback_required:
  {
    _gumjs_throw (ctx, exception, "callback required");
    return FALSE;
  }
}

gboolean
_gumjs_callback_try_get_opt (JSContextRef ctx,
                             JSValueRef value,
                             JSObjectRef * callback,
                             JSValueRef * exception)
{
  if (!JSValueIsUndefined (ctx, value) && !JSValueIsNull (ctx, value))
  {
    JSObjectRef obj;

    if (!JSValueIsObject (ctx, value))
      goto invalid_argument;

    obj = (JSObjectRef) value;
    if (!JSObjectIsFunction (ctx, obj))
      goto invalid_argument;

    *callback = obj;
  }
  else
  {
    *callback = NULL;
  }

  return TRUE;

invalid_argument:
  {
    _gumjs_throw (ctx, exception, "expected function");
    return FALSE;
  }
}

void
_gumjs_throw (JSContextRef ctx,
              JSValueRef * exception,
              const gchar * format,
              ...)
{
  va_list args;
  gchar * message;
  JSValueRef message_value;

  va_start (args, format);
  message = g_strdup_vprintf (format, args);
  va_end (args);

  message_value = _gumjs_string_to_value (ctx, message);

  g_free (message);

  if (exception != NULL)
    *exception = JSObjectMakeError (ctx, 1, &message_value, NULL);
}