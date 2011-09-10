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

var util = require('util');
var binding = process.binding('zlib');
var constants = process.binding('constants');
var Stream = require('stream').Stream;
var EventEmitter = require('events').EventEmitter;

var zlib = exports;

// Constants
// compression levels
zlib.Z_NO_COMPRESSION  = 0;
zlib.Z_BEST_SPEED  = 1;
zlib.Z_BEST_COMPRESSION  = 9;
zlib.Z_DEFAULT_COMPRESSION  = -1;

// compression strategy
zlib.Z_FILTERED  = 1;
zlib.Z_HUFFMAN_ONLY  = 2;
zlib.Z_RLE  = 3;
zlib.Z_FIXED  = 4;
zlib.Z_DEFAULT_STRATEGY  = 0;

// data type
zlib.Z_BINARY  = 0;
zlib.Z_TEXT  = 1;
zlib.Z_ASCII  = 1;
zlib.Z_UNKNOWN  = 2;


// generic zipstream
var ZipStream = zlib.ZipStream = function() {
  Stream.call(this);
  this._zlibStream = new binding.ZlibStream();
};

util.inherits(ZipStream, Stream);

ZipStream.prototype.writable = true;
ZipStream.prototype.readable = true;
ZipStream.prototype.end = function() {
  this.emit('end');
}


var InflateStream = zlib.InflateStream = function() {
  ZipStream.call(this);
  this._zlibStream.initInflate();
};

util.inherits(InflateStream, ZipStream);

InflateStream.prototype.write = function(buffer) {
  var result = binding.inflate(this._zlibStream, buffer);
  if (result) {
    this.emit('data', result);
    return true;
  }
  return false;
};


var DeflateStream = zlib.DeflateStream = function(level, strategy, type) {
  ZipStream.call(this);

  level = level || zlib.Z_DEFAULT_COMPRESSION;
  strategy = strategy || zlib.Z_DEFAULT_STRATEGY;
  type = type || zlib.Z_UNKNOWN;

  this._zlibStream.initDeflate(level, strategy, type);
};

util.inherits(DeflateStream, ZipStream);


DeflateStream.prototype.write = function(buffer) {
  var result = binding.deflate(this._zlibStream, buffer);
  if (result) {
    this.emit('data', result);
    return true;
  }
  return false;
};




zlib.gzip = {
  createInflateStream : function() {
    return new InflateStream();
  },

  // Create a deflate (writable) stream
  createDeflateStream : function(level, strategy, type) {
    return new DeflateStream(level, strategy, type);
  }
};
