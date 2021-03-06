/*
 * This file is part of Espruino, a JavaScript interpreter for Microcontrollers
 *
 * Copyright (C) 2013 Gordon Williams <gw@pur3.co.uk>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ----------------------------------------------------------------------------
 * This file is designed to be parsed during the build process
 *
 * JavaScript methods for Objects and Functions
 * ----------------------------------------------------------------------------
 */
#include "jswrap_object.h"
#include "jsparse.h"
#include "jsinteractive.h"
#include "jswrapper.h"
#ifdef __MINGW32__
#include "malloc.h" // needed for alloca
#endif//__MINGW32__

/*JSON{ "type":"class",
        "class" : "Hardware",
        "check" : "jsvIsRoot(var)",
        "description" : ["This is the built-in class for the Espruino device. It is the 'root scope', as 'Window' is for JavaScript on the desktop." ]
}*/
/*JSON{ "type":"class",
        "class" : "Object",
        "check" : "jsvIsObject(var)",
        "description" : ["This is the built-in class for Objects" ]
}*/
/*JSON{ "type":"class",
        "class" : "Function",
        "check" : "jsvIsFunction(var)",
        "description" : ["This is the built-in class for Functions" ]
}*/
/*JSON{ "type":"class",
        "class" : "Integer", "prototype" : "Number",
        "check" : "jsvIsInt(var)",
        "description" : ["This is the built-in class for Integer values" ]
}*/
/*JSON{ "type":"class",
        "class" : "Double", "prototype" : "Number",
        "check" : "jsvIsFloat(var)",
        "description" : ["This is the built-in class for Floating Point values" ]
}*/

/*JSON{ "type":"property", "class": "Object", "name" : "length",
         "description" : "Find the length of the object",
         "generate" : "jswrap_object_length",
         "return" : ["JsVar", "The value of the string"]
}*/
JsVar *jswrap_object_length(JsVar *parent) {
  if (jsvIsArray(parent)) {
    return jsvNewFromInteger(jsvGetArrayLength(parent));
  } else if (jsvIsArrayBuffer(parent)) {
      return jsvNewFromInteger((JsVarInt)jsvGetArrayBufferLength(parent));
  } else if (jsvIsString(parent)) {
    return jsvNewFromInteger((JsVarInt)jsvGetStringLength(parent));
  }
  return 0;
}

/*JSON{ "type":"method", "class": "Object", "name" : "valueOf",
         "description" : "Returns the primitive value of this object.",
         "generate" : "jswrap_object_valueOf",
         "return" : ["JsVar", "The primitive value of this object"]
}*/
JsVar *jswrap_object_valueOf(JsVar *parent) {
  return jsvLockAgain(parent);
}

/*JSON{ "type":"method", "class": "Object", "name" : "toString",
         "description" : "Convert the Object to a string",
         "generate" : "jswrap_object_toString",
         "params" : [ [ "radix", "JsVar", "If the object is an integer, the radix (between 2 and 36) to use. NOTE: Setting a radix does not work on floating point numbers."] ],
         "return" : ["JsVar", "A String representing the object"]
}*/
JsVar *jswrap_object_toString(JsVar *parent, JsVar *arg0) {
  if (jsvIsInt(arg0) && jsvIsNumeric(parent)) {
    JsVarInt radix = jsvGetInteger(arg0);
    if (radix>=2 && radix<=36) {
      char buf[JS_NUMBER_BUFFER_SIZE];
      if (jsvIsInt(parent))
        itoa(jsvGetInteger(parent), buf, (unsigned int)radix);
      else
        ftoa_bounded_extra(jsvGetFloat(parent), buf, sizeof(buf), (int)radix, -1);
      return jsvNewFromString(buf);
    }
  }
  return jsvAsString(parent, false);
}

/*JSON{ "type":"method", "class": "Object", "name" : "clone",
         "description" : "Copy this object completely",
         "generate" : "jswrap_object_clone",
         "return" : ["JsVar", "A copy of this Object"]
}*/
JsVar *jswrap_object_clone(JsVar *parent) {
  return jsvCopy(parent);
}

/*JSON{ "type":"staticmethod", "class": "Object", "name" : "keys",
         "description" : "Return all enumerable keys of the given object",
         "generate" : "jswrap_object_keys",
         "params" : [ [ "object", "JsVar", "The object to return keys for"] ],
         "return" : ["JsVar", "An array of strings - one for each key on the given object"]
}*/
JsVar *jswrap_object_keys(JsVar *obj) {
  // strings are iterable, but we shouldn't try and show keys for them
  if (jsvIsIterable(obj) && !jsvIsString(obj)) {
    bool (*checkerFunction)(JsVar*) = 0;
    if (jsvIsFunction(obj)) checkerFunction = jsvIsInternalFunctionKey;
    else if (jsvIsObject(obj)) checkerFunction = jsvIsInternalObjectKey;

    JsVar *arr = jsvNewWithFlags(JSV_ARRAY);
    if (!arr) return 0;
    JsvIterator it;
    jsvIteratorNew(&it, obj);
    while (jsvIteratorHasElement(&it)) {
      JsVar *key = jsvIteratorGetKey(&it);
      if (!(checkerFunction && checkerFunction(key))) {
        JsVar *name = jsvAsString(key,false);
        if (name) {
          jsvArrayPushAndUnLock(arr, name);
        }
      }
      jsvUnLock(key);
      jsvIteratorNext(&it);
    }
    jsvIteratorFree(&it);
    return arr;
  } else {
    jsWarn("Object.keys called on non-object");
    return 0;
  }
}

/*JSON{ "type":"method", "class": "Object", "name" : "hasOwnProperty",
         "description" : ["Return true if the object (not its prototype) has the given property.","NOTE: This currently returns false-positives for built-in functions in prototypes"],
         "generate" : "jswrap_object_hasOwnProperty",
         "params" : [ [ "name", "JsVar", "The name of the property to search for"] ],
         "return" : ["bool", "True if it exists, false if it doesn't"]
}*/
bool jswrap_object_hasOwnProperty(JsVar *parent, JsVar *name) {

  JsVar *propName = jsvAsArrayIndex(name);

  bool contains = false;
  if (jsvHasChildren(parent)) {
    JsVar *foundVar =  jsvFindChildFromVar(parent, propName, false);
    if (foundVar) {
      contains = true;
      jsvUnLock(foundVar);
    }
  }

  if (!contains) { // search builtins
    char str[32];
    jsvGetString(propName, str, sizeof(str));

    JsVar *foundVar = jswFindBuiltInFunction(parent, str);
    if (foundVar) {
      contains = true;
      jsvUnLock(foundVar);
    }
  }

  jsvUnLock(propName);
  return contains;
}



// --------------------------------------------------------------------------
//                                            These should be in EventEmitter

/*JSON{ "type":"method", "class": "Object", "name" : "on",
         "description" : ["Register an event listener for this object, for instance ```http.on('data', function(d) {...})```. See Node.js's EventEmitter."],
         "generate" : "jswrap_object_on",
         "params" : [ [ "event", "JsVar", "The name of the event, for instance 'data'"],
                      [ "listener", "JsVar", "The listener to call when this event is received"] ]
}*/
void jswrap_object_on(JsVar *parent, JsVar *event, JsVar *listener) {
  if (!jsvIsObject(parent)) {
      jsWarn("Parent must be a proper object - not a String, Integer, etc.");
      return;
    }
  if (!jsvIsString(event)) {
      jsWarn("First argument to EventEmitter.on(..) must be a string");
      return;
    }
  if (!jsvIsFunction(listener) && !jsvIsString(listener)) {
    jsWarn("Second argument to EventEmitter.on(..) must be a function or a String (containing code)");
    return;
  }
  char eventName[16] = "#on";
  jsvGetString(event, &eventName[3], sizeof(eventName)-4);

  JsVar *eventList = jsvFindChildFromString(parent, eventName, true);
  JsVar *eventListeners = jsvSkipName(eventList);
  if (jsvIsUndefined(eventListeners)) {
    // just add
    jsvSetValueOfName(eventList, listener);
  } else {
    if (jsvIsArray(eventListeners)) {
      // we already have an array, just add to it
      jsvArrayPush(eventListeners, listener);
    } else {
      // not an array - we need to make it an array
      JsVar *arr = jsvNewWithFlags(JSV_ARRAY);
      jsvArrayPush(arr, eventListeners);
      jsvArrayPush(arr, listener);
      jsvSetValueOfName(eventList, arr);
      jsvUnLock(arr);
    }
  }
  jsvUnLock(eventListeners);
  jsvUnLock(eventList);
}

/*JSON{ "type":"method", "class": "Object", "name" : "emit",
         "description" : ["Call the event listeners for this object, for instance ```http.emit('data', 'Foo')```. See Node.js's EventEmitter."],
         "generate" : "jswrap_object_emit",
         "params" : [ [ "event", "JsVar", "The name of the event, for instance 'data'"],
                      [ "args", "JsVarArray", "Optional arguments"] ]
}*/
void jswrap_object_emit(JsVar *parent, JsVar *event, JsVar *argArray) {
  if (!jsvIsObject(parent)) {
      jsWarn("Parent must be a proper object - not a String, Integer, etc.");
      return;
    }
  if (!jsvIsString(event)) {
    jsWarn("First argument to EventEmitter.emit(..) must be a string");
    return;
  }
  char eventName[16] = "#on";
  jsvGetString(event, &eventName[3], sizeof(eventName)-4);

  // extract data
  const int MAX_ARGS = 4;
  JsVar *args[MAX_ARGS];
  int n = 0;
  JsvArrayIterator it;
  jsvArrayIteratorNew(&it, argArray);
  while (jsvArrayIteratorHasElement(&it)) {
    if (n>=MAX_ARGS) {
      jsWarn("Too many arguments");
      break;
    }
    args[n++] = jsvArrayIteratorGetElement(&it);
    jsvArrayIteratorNext(&it);
  }
  jsvArrayIteratorFree(&it);


  jsiQueueObjectCallbacks(parent, eventName, args, n);
  // unlock
  while (n--)
    jsvUnLock(args[n]);
}

/*JSON{ "type":"method", "class": "Object", "name" : "removeAllListeners",
         "description" : ["Removes all listeners, or those of the specified event."],
         "generate" : "jswrap_object_removeAllListeners",
         "params" : [ [ "event", "JsVar", "The name of the event, for instance 'data'"] ]
}*/
void jswrap_object_removeAllListeners(JsVar *parent, JsVar *event) {
  if (!jsvIsObject(parent)) {
      jsWarn("Parent must be a proper object - not a String, Integer, etc.");
      return;
    }
  if (jsvIsString(event)) {
    // remove the whole child containing listeners
    char eventName[16] = "#on";
    jsvGetString(event, &eventName[3], sizeof(eventName)-4);
    JsVar *eventList = jsvFindChildFromString(parent, eventName, true);
    if (eventList) {
      jsvRemoveChild(parent, eventList);
      jsvUnLock(eventList);
    }
  } else if (jsvIsUndefined(event)) {
    // Eep. We must remove everything beginning with '#on'
    JsvObjectIterator it;
    jsvObjectIteratorNew(&it, parent);
    while (jsvObjectIteratorHasElement(&it)) {
      JsVar *key = jsvObjectIteratorGetKey(&it);
      jsvObjectIteratorNext(&it);
      if (jsvIsString(key) &&
          key->varData.str[0]=='#' &&
          key->varData.str[1]=='o' &&
          key->varData.str[2]=='n') {
        // begins with #on - we must kill it
        jsvRemoveChild(parent, key);
      }
      jsvUnLock(key);
    }
    jsvObjectIteratorFree(&it);
  } else {
    jsWarn("First argument to EventEmitter.removeAllListeners(..) must be a string, or undefined");
    return;
  }
}

// ------------------------------------------------------------------------------

/*JSON{ "type":"method", "class": "Function", "name" : "replaceWith",
         "description" : ["This replaces the function with the one in the argument - while keeping the old function's scope. This allows inner functions to be edited, and is used when edit() is called on an inner function."],
         "generate" : "jswrap_function_replaceWith",
         "params" : [ [ "newFunc", "JsVar", "The new function to replace this function with"] ]
}*/
void jswrap_function_replaceWith(JsVar *oldFunc, JsVar *newFunc) {
  if (!jsvIsFunction(newFunc)) {
    jsWarn("First argument of replaceWith should be a function - ignoring");
    return;
  }
  // Grab scope - the one thing we want to keep
  JsVar *scope = jsvFindChildFromString(oldFunc, JSPARSE_FUNCTION_SCOPE_NAME, false);
  // so now remove all existing entries
  jsvRemoveAllChildren(oldFunc);
  // now re-add scope
  if (scope) jsvAddName(oldFunc, scope);
  jsvUnLock(scope);
  // now re-add other entries
  JsvObjectIterator it;
  jsvObjectIteratorNew(&it, newFunc);
  while (jsvObjectIteratorHasElement(&it)) {
    JsVar *el = jsvObjectIteratorGetKey(&it);
    jsvObjectIteratorNext(&it);
    if (!jsvIsStringEqual(el, JSPARSE_FUNCTION_SCOPE_NAME)) {
      JsVar *copy = jsvCopy(el);
      if (copy) {
        jsvAddName(oldFunc, copy);
        jsvUnLock(copy);
      }
    }
    jsvUnLock(el);
  }
  jsvObjectIteratorFree(&it);

}

/*JSON{ "type":"method", "class": "Function", "name" : "call",
         "description" : ["This executes the function with the supplied 'this' argument and parameters"],
         "generate" : "jswrap_function_apply_or_call",
         "params" : [ [ "this", "JsVar", "The value to use as the 'this' argument when executing the function"],
                      [ "params", "JsVarArray", "Optional Parameters"]
                    ],
         "return" : [ "JsVar", "The return value of executing this function" ]
}*/
// ... it just so happens that the way JsVarArray is parsed means that apply and call can be exactly the same function!


/*JSON{ "type":"method", "class": "Function", "name" : "apply",
         "description" : ["This executes the function with the supplied 'this' argument and parameters"],
         "generate" : "jswrap_function_apply_or_call",
         "params" : [ [ "this", "JsVar", "The value to use as the 'this' argument when executing the function"],
                      [ "args", "JsVar", "Optional Array of Aruments"]
                    ],
         "return" : [ "JsVar", "The return value of executing this function" ]
}*/
JsVar *jswrap_function_apply_or_call(JsVar *parent, JsVar *thisArg, JsVar *argsArray) {
  unsigned int i;
  JsVar **args = 0;
  size_t argC = 0;

  if (jsvIsArray(argsArray)) {
    argC = (size_t)jsvGetArrayLength(argsArray);
    if (argC>64) argC=64; // sanity
    args = (JsVar**)alloca((size_t)argC * sizeof(JsVar*));


    for (i=0;i<argC;i++) args[i] = 0;
    JsvArrayIterator it;
    jsvArrayIteratorNew(&it, argsArray);
    while (jsvArrayIteratorHasElement(&it)) {
      JsVarInt idx = jsvGetIntegerAndUnLock(jsvArrayIteratorGetIndex(&it));
      if (idx>=0 && idx<(int)argC) {
        assert(!args[idx]); // just in case there were dups
        args[idx] = jsvArrayIteratorGetElement(&it);
      }
      jsvArrayIteratorNext(&it);
    }
    jsvArrayIteratorFree(&it);
  } else if (!jsvIsUndefined(argsArray)) {
    jsWarn("Second argument to Function.apply must be an array");
  }

  JsVar *r = jspeFunctionCall(parent, 0, thisArg, false, (int)argC, args);
  for (i=0;i<argC;i++) jsvUnLock(args[i]);
  return r;
}
