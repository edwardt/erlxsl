/*
 * erlxsl_api.h
 * 
 * -----------------------------------------------------------------------------
 * Copyright (c) 2008-2010 Tim Watson (watson.timothy@gmail.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -----------------------------------------------------------------------------
 *
 *  This header file contains internal and external api functions. 
 */

/* pervasive includes */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef _ERLXSL_H
#define  _ERLXSL_H

#ifdef  __cplusplus
extern "C" {
#endif

    /* macro definitions and other hash-defines */
#ifdef DEBUG
#define DebugOut(/* (char*) */message, args...)  \
    fprintf (stderr, message , ## args);
#else
#define DebugOut(message, args...) /* DEBUG OUTPUT DISABLED */
#endif

#define ERROR(str, ...)  \
    LOG(stderr, str, ##__VA_ARGS__);

#define INFO(str, ...)  \
    LOG(stdout, str, ##__VA_ARGS__);

#define LOG(stream, str, ...)  \
    fprintf(stream, str, ##__VA_ARGS__);

  // TODO: investigate using the Apache Portable Runtime
  
  /* Data Types & Aliasing */
  
  typedef uint8_t        UInt8;
  typedef uint16_t       UInt16;
  typedef uint32_t       UInt32;
  typedef int8_t         Int8;
  typedef int16_t       Int16;
  typedef int32_t       Int32;
  typedef int64_t        Int64;
  
  /*
   * Packages information about a parameter (its name and value)
   * and the next parameter in line - i.e. this is a linked list.
   */
  typedef struct {
    /*
     * The name of the parameter (as passed to xslt).
     */
    char* key;
    /*
     * The value assigned to the parameter (as passed to xslt).
     */
    char* value;
    /*
     * Stores a pointer to the next parameter, or NULL if this
     * element is the tail.
     */
    void* next;
  } param_info;
  
  typedef enum {
    Ok,
    Error,
    XmlParseError,
    XslTransformError,
    OutOfMemoryError
  } EngineState;

  typedef enum input_type {
    File = 1,
    Buffer = 2,
    Stream = 3
  } InputType;
  
  typedef enum { Binary, Text } ResultFormat;
    
  /*
   * transform_request packages the data used during
   * transformation, which is (usually) gathered from the
   * input buffer supplied by the Erlang runtime during
   * a call to one of the control/input callback functions,
   * with some useful flags.
   */
  typedef struct {
    // Denotes the kind of input (e.g. buffer, stream or file based) for the data.
    InputType input_kind;
    // Denotes the kind of xsl input (e.g. buffer, stream or file based) for the stylesheet.
    InputType xsl_kind;
    // A pointer to the head element in a linked list of param_info structures.
    param_info* parameters;
    // A pointer to the input data.
    char* input;
    // A pointer to the stylesheet data.
    char* stylesheet;
  } transform_job;
  
  /* comms related data structures (e.g. for interacting with the emulator) */    
  
  /*
   * Stores the current request context, which consists of the port,
   * target process id (note that this doesn't have to be the Pid of 
   * the *connected process* if you want to send data to another Pid) and
   * a pointer to the current request.
   */
  typedef struct {
      /* Stores the port associated with the current driver instance. */
      void* port;
      /* Stores the calling process' Pid. */
      unsigned long caller_pid;
      /* Stores a pointer to the current request data. */
      transform_job* job;
  } request_context;
    
  /*
   * Stores response data operated on when calling back into the emulator.
   */
  typedef struct {
    /* Stores the current request context. */
    request_context* context;
    /*
     * If anything went wrong with request processing, this field will
     * (hopefully) contain a helpful error message.
     */
    char * errorMessage;
    /* Stores the response size (either the buffer length or the 
     * number of terms in the supplied composite ErlDrvTermData array).
     * NB: This field is optional, as the protocol implementation performs
     * the same bounds checking regardless.
     */
    int size;
    /* The format in which the resulting data is stored */
    ResultFormat format;
    /* Stores the response data itself. */
    union { 
      /* Maintaining our response data in a buffer. */
      char* buffer;
      /* Maintaining our response data in the *driver term format*; this
       is a pointer to the start of an array in the appropriate format.*/
      void* data;
    } payload;
  } transform_result;

/************************  THE EXTERNAL API ***********************************/

  /*
   * This function is the request handler hook used by plugins (such as 
   * implementations for sablotron and/or libxslt).
   * 
   * The response structure passed contains the context required to get to
   * underlying data, along with fields which the plugin can fill in to 
   * get data sent back to erlang.
   *
   */
  typedef EngineState TransformFunc(transform_result* result); 
  
  /*
   * This function gives the implementing plugin a chance to cleanup 
   * after handle_request has returned. 
   *
   * Returns a value from the 'DriverState' enumeration to indicate
   * current system state. A return value of DriverState::ProviderError
   * will cause the driver to exit, which the erlang port handler will
   * interpret as and error and unload the driver library.
   */
  typedef EngineState PostTransformFunc(transform_result* result);
    
  /*
   * Gives the implementation provider a change to cleanup. Because this function
   * is only called when the driver is being stopped, the provider cannot supply
   * a meaningful return value, therefore the function returns void.
   *
   * After this function returns, the memory allocated for the xsl_engine will be
   * freed, which means that the engine must fully release all references/pointers 
   * held before returning.
   */
   typedef void ShutdownFunc(void* state);
   
  /* Represents an XSLT engine. */
  typedef struct {
    /* The following function pointers will need 
       be set by the provider on startup */
    TransformFunc*      transform;
    PostTransformFunc*  after_transform;
    ShutdownFunc*       shutdown;
    /* Generic storage location, so providers can stash whatever they need. */
    void*               providerData;
  } xsl_engine;

/******************************************************************************/    
    

#ifdef  __cplusplus
}
#endif

#endif  /* _ERLXSL_H */