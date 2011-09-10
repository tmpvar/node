// Copyright Joyent, Inc. and other Node contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <node.h>
#include <zlib.h>
#include <zutil.h>
#include "node_zlib.h"
#include "node_buffer.h"

#define MODE_INFLATE 1
#define MODE_DEFLATE 2

int inline destroy(z_stream *stream, int mode) {
  int err = Z_OK;
  if (stream->state != NULL) {
    if (mode == MODE_INFLATE) {
      inflateEnd(stream);
    } else if (mode == MODE_DEFLATE) {
      deflateEnd(stream);
    }
  }

  if (stream->next_in) {
    free(stream->next_in);
  }

  return err;
}

namespace node {
using namespace v8;

Persistent<FunctionTemplate> ZlibStream::constructor_template;

ZlibStream::ZlibStream(Handle<Object> wrapper) : ObjectWrap() {
  Wrap(wrapper);

  this->zstream_.zalloc = (alloc_func)0;
  this->zstream_.zfree = (free_func)0;
  this->zstream_.opaque = (voidpf)0;
  this->zstream_.next_in = Z_NULL;
  this->zstream_.next_out = Z_NULL;
  this->zstream_.avail_in = this->zstream_.avail_out = 0;

  V8::AdjustAmountOfExternalAllocatedMemory(sizeof(this->zstream_));
}

ZlibStream::~ZlibStream() {

}


Handle<Value> ZlibStream::InitDeflate(const Arguments &args) {
  HandleScope scope;
  ZlibStream *stream = ObjectWrap::Unwrap<ZlibStream>(args.This());

  int level = args[0]->Int32Value();
  int strategy = args[1]->Int32Value();

  int err = deflateInit2(&(stream->zstream_), level,
                           Z_DEFLATED, -MAX_WBITS, DEF_MEM_LEVEL, strategy);

  stream->zstream_.next_out = (Byte*)malloc(Z_BUFSIZE);
  V8::AdjustAmountOfExternalAllocatedMemory(Z_BUFSIZE);

  // TODO: collect better error information
  if (err != Z_OK || stream->zstream_.next_out == Z_NULL) {
    destroy(&stream->zstream_, MODE_DEFLATE);
    return ThrowException(Exception::Error(String::New(
            "There was an error initializing the deflate stream")));
  }

  stream->zstream_.avail_out = Z_BUFSIZE;

  deflateParams (&(stream->zstream_), level, strategy);

  return Undefined();
}

Handle<Value> ZlibStream::InitInflate(const Arguments &args) {
  HandleScope scope;
  ZlibStream *stream = ObjectWrap::Unwrap<ZlibStream>(args.This());

  stream->zstream_.next_in  = (Byte*)malloc(Z_BUFSIZE);
  V8::AdjustAmountOfExternalAllocatedMemory(Z_BUFSIZE);

  int err = inflateInit2(&(stream->zstream_), -MAX_WBITS);

  // TODO: collect better error information
  if (err != Z_OK || stream->zstream_.next_in == Z_NULL) {
    destroy(&stream->zstream_, MODE_INFLATE);

    return ThrowException(Exception::Error(String::New(
            "There was an error initializing the inflate stream")));
  }

  stream->zstream_.avail_out = Z_BUFSIZE;

  return Undefined();
}


Handle<Value> ZlibStream::New(const Arguments &args) {
  HandleScope scope;
  new ZlibStream(args.This());

  return args.This();
}


void ZlibStream::Initialize(Handle<Object> target) {
  Local<FunctionTemplate> t = FunctionTemplate::New(ZlibStream::New);
  constructor_template = Persistent<FunctionTemplate>::New(t);
  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(String::NewSymbol("ZlibStream"));


  NODE_SET_PROTOTYPE_METHOD(constructor_template, "initDeflate", ZlibStream::InitDeflate);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "initInflate", ZlibStream::InitInflate);

  target->Set(String::NewSymbol("ZlibStream"), constructor_template->GetFunction());
}

static Handle<Value> Inflate(const Arguments& args) {
  HandleScope scope;

  if (args.Length() < 2 || !Buffer::HasInstance(args[1])) {
      return ThrowException(Exception::Error(String::New(
            "Invalid arguments, expects: (ZlibStream, Buffer)")));
  }

  ZlibStream *stream = ObjectWrap::Unwrap<ZlibStream>(args[0]->ToObject());

  return scope.Close(args[1]->ToObject());
}


static Handle<Value> Deflate(const Arguments& args) {
  HandleScope scope;

  if (args.Length() < 2 || !Buffer::HasInstance(args[1])) {
      return ThrowException(Exception::Error(String::New(
            "Invalid arguments, expects: (ZlibStream, Buffer)")));
  }

  ZlibStream *stream = ObjectWrap::Unwrap<ZlibStream>(args[0]->ToObject());
  Buffer *buffer = ObjectWrap::Unwrap<Buffer>(args[1]->ToObject());

  Local<Object> buffer_handle = args[1]->ToObject();

  char *data = Buffer::Data(buffer_handle);

  stream->zstream_.next_in = (Bytef *)data;
  stream->zstream_.avail_in = Buffer::Length(buffer_handle);

  deflate(&(stream->zstream_), Z_NO_FLUSH);

  // push out the buffer immediately


  return scope.Close(args[1]->ToObject());
}

void InitZlib(Handle<Object> target) {
  HandleScope scope;
  ZlibStream::Initialize(target);

  NODE_SET_METHOD(target, "inflate", Inflate);
  NODE_SET_METHOD(target, "deflate", Deflate);
}

}  // end namespace node

NODE_MODULE(node_zlib, node::InitZlib);
