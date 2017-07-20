// Type definition for Duktape 2.x

declare var global: Global;

interface Global {
    /* ECMA script objects */
    NaN: typeof NaN;
    Infinity: typeof Infinity;
    undefined: typeof undefined;
    Object: typeof Object;
    Function: typeof Function;
    Array: typeof Array;
    String: typeof String;
    Boolean: typeof Boolean;
    Number: typeof Number;
    Date: typeof Date;
    RegExp: typeof RegExp;
    Error: typeof Error;
    EvalError: typeof EvalError;
    RangeError: typeof RangeError;
    ReferenceError: typeof ReferenceError;
    SyntaxError: typeof SyntaxError;
    TypeError: typeof TypeError;
    URIError: typeof URIError;
    Math: typeof Math;
    JSON: typeof JSON;
    ArrayBuffer: typeof ArrayBuffer;
    DataView: typeof DataView;
    Int8Array: typeof Int8Array;
    Uint8Array: typeof Uint8Array;
    Uint8ClampedArray: typeof Uint8ClampedArray;
    Int16Array: typeof Int16Array;
    Uint16Array: typeof Uint16Array;
    Int32Array: typeof Int32Array;
    Uint32Array: typeof Uint32Array;
    Float32Array: typeof Float32Array;
    Float64Array: typeof Float64Array;
    eval: typeof eval;
    parseInt: typeof parseInt;
    parseFloat: typeof parseFloat;
    isNaN: typeof isNaN;
    isFinite: typeof isFinite;
    decodeURI: typeof decodeURI;
    decodeURIComponent: typeof decodeURIComponent;
    encodeURI: typeof encodeURI;
    encodeURIComponent: typeof encodeURIComponent;
    escape: (str: string) => string;
    unescape: (str: string) => string;

    /* Post ES5 features */
    Proxy: typeof Proxy;
    Reflect: typeof Reflect;

    /* Node.js objects */
    Buffer: typeof Buffer;

    /* Additional global objects */
    global: Global;
    Duktape: typeof Duktape;
    TextEncoder: typeof TextEncoder;
    TextDecoder: typeof TextDecoder;
}

/**
 * Duktape object
 */
declare module Duktape {
    const version: number;
    const env: string;
    function fin(o: Object, finalizer?: Function): Function;
    function enc(format: "hex" | "base64", value: any): string;
    function enc(format: "jx" | "jc", value: any, replacer?: Function, space?: number): string;
    function dec(format: "hex" | "base64", value: string): any;
    function dec(format: "jx" | "jc", value: string, reviver?: Function): any;
    function info(o: any): ObjectInfo;
    function act(depth: number): CallstackInfo;
    function gc(): void;
    function compact(o: any): any;

    interface Pointer {
        toString(): string;
        valueOf(): any;
    }

    interface PointerConstructor {
        readonly prototype: Pointer;
        new (pointer: any): Pointer;
        (pointer: any): any;
    }
    const Pointer: PointerConstructor;

    interface Thread {
    }

    interface ThreadConstructor {
        readonly prototype: Thread;
        new (fn: Function): Thread;
        (fn: Function): Thread;
    }
    const Thread: ThreadConstructor;
}

interface ObjectInfo {
    type: number;
    itag: number;
    hptr: any;
    refc: number;
    class: number;
    hbytes: number;
    pbytes: number;
    bcbytes: number;
    dbytes: number;
    esize: number;
    enext: number;
    asize: number;
    hsize: number;
    tstate: number;
    variant: number;
}

interface CallstackInfo {
    function: Function;
    pc: number;
    lineNumber: number;
}

/*
 * Post-ES5 features
 */

interface ProxyHandler<T extends object> {
    has?: (target: T, key: string) => boolean;
    get?: (target: T, key: string, receiver: any) => any;
    set?: (target: T, key: string, value: any, receiver: any) => boolean;
    deleteProperty?: (target: T, key: string) => boolean;
    ownKeys?: (target: T) => string[];
}

interface ProxyConstructor {
    new <T extends object>(target: T, handler: ProxyHandler<T>): T;
}
declare var Proxy: ProxyConstructor;

declare namespace Reflect {
    function get(target: object, key: string, receiver?: any): any;
    function set(target: object, key: string, value: any, receiver?: any): boolean;
    function has(target: object, key: string): boolean;
    function deleteProperty(target: object, key: string): boolean;
    function getOwnPropertyDescriptor(target: object, key: string): PropertyDescriptor;
    function defineProperty(target: object, key: string, desc: PropertyDescriptor): boolean;
    function getPrototypeOf(target: object): object;
    function setPrototypeOf(target: object, proto: any): boolean;
    function isExtensible(target: object): boolean;
    function preventExtensions(target: object): boolean;
    function ownKeys(target: object): string[];
    function apply(target: Function, thisArg: any, args: any[]): any;
    function construct(target: Function, args: any[]): any;
}

/*
 * Node.js like objects
 */

interface Buffer extends Uint8Array {
    /**
     * Reads a unsigned 8-bit integer from buf at the specified offset
     * @param offset Where to start reading
     * @param noAssert Skip offset validation
     */
    readUInt8(offset: number, noAssert?: boolean): number;

    /**
     * Reads a signed 8-bit integer from buf at the specified offset
     * @param offset Where to start reading
     * @param noAssert Skip offset validation
     */
    readInt8(offset: number, noAssert?: boolean): number;

    /**
     * Reads a unsigned 16-bit integer from buf at the specified offset with big-endian format
     * @param offset Where to start reading
     * @param noAssert Skip offset validation
     */
    readUInt16BE(offset: number, noAssert?: boolean): number;

    /**
     * Reads a unsigned 16-bit integer from buf at the specified offset with little-endian format
     * @param offset Where to start reading
     * @param noAssert Skip offset validation
     */
    readUInt16LE(offset: number, noAssert?: boolean): number;

    /**
     * Reads a signed 16-bit integer from buf at the specified offset with big-endian format
     * @param offset Where to start reading
     * @param noAssert Skip offset validation
     */
    readInt16BE(offset: number, noAssert?: boolean): number;

    /**
     * Reads a signed 16-bit integer from buf at the specified offset with little-endian format
     * @param offset Where to start reading
     * @param noAssert Skip offset validation
     */
    readInt16LE(offset: number, noAssert?: boolean): number;

    /**
     * Reads a unsigned 32-bit integer from buf at the specified offset with big-endian format
     * @param offset Where to start reading
     * @param noAssert Skip offset validation
     */
    readUInt32BE(offset: number, noAssert?: boolean): number;

    /**
     * Reads a unsigned 32-bit integer from buf at the specified offset with little-endian format
     * @param offset Where to start reading
     * @param noAssert Skip offset validation
     */
    readUInt32LE(offset: number, noAssert?: boolean): number;

    /**
     * Reads a signed 32-bit integer from buf at the specified offset with big-endian format
     * @param offset Where to start reading
     * @param noAssert Skip offset validation
     */
    readInt32BE(offset: number, noAssert?: boolean): number;

    /**
     * Reads a signed 32-bit integer from buf at the specified offset with little-endian format
     * @param offset Where to start reading
     * @param noAssert Skip offset validation
     */
    readInt32LE(offset: number, noAssert?: boolean): number;

    /**
     * Reads a 32-bit float from buf at the specified offset with big-endian format
     * @param offset Where to start reading
     * @param noAssert Skip offset validation
     */
    readFloatBE(offset: number, noAssert?: boolean): number;

    /**
     * Reads a 32-bit float from buf at the specified offset with little-endian format
     * @param offset Where to start reading
     * @param noAssert Skip offset validation
     */
    readFloatLE(offset: number, noAssert?: boolean): number;

    /**
     * Reads a 64-bit double from buf at the specified offset with big-endian format
     * @param offset Where to start reading
     * @param noAssert Skip offset validation
     */
    readDoubleBE(offset: number, noAssert?: boolean): number;

    /**
     * Reads a 64-bit double from buf at the specified offset with little-endian format
     * @param offset Where to start reading
     * @param noAssert Skip offset validation
     */
    readDoubleLE(offset: number, noAssert?: boolean): number;

    /**
     * Reads byteLength number of bytes from buf at the specified offset with big-endian format
     * and interprets the result as an unsigned integer
     * @param offset Where to start reading
     * @param byteLength How many bytes to read
     * @param noAssert Skip offset validation
     */
    readUIntBE(offset: number, byteLength: number, noAssert?: boolean): number;

    /**
     * Reads byteLength number of bytes from buf at the specified offset with little-endian format
     * and interprets the result as an unsigned integer
     * @param offset Where to start reading
     * @param byteLength How many bytes to read
     * @param noAssert Skip offset validation
     */
    readUIntLE(offset: number, byteLength: number, noAssert?: boolean): number;

    /**
     * Reads byteLength number of bytes from buf at the specified offset with big-endian format
     * and interprets the result as a two's complement signed value (Up to 48 bits accuracy)
     * @param offset Where to start reading
     * @param byteLength How many bytes to read
     * @param noAssert Skip offset validation
     */
    readIntBE(offset: number, byteLength: number, noAssert?: boolean): number;

    /**
     * Reads byteLength number of bytes from buf at the specified offset with little-endian format
     * and interprets the result as a two's complement signed value (Up to 48 bits accuracy)
     * @param offset Where to start reading
     * @param byteLength How many bytes to read
     * @param noAssert Skip offset validation
     */
    readIntLE(offset: number, byteLength: number, noAssert?: boolean): number;

    /**
     * Writes unsigned 8-bit value to buf at the specified offset
     * @param value Number to be written to buf
     * @param offset Where to start writing
     * @param noAssert Skip value and offset validation
     * @return offset plus number of bytes written
     */
    writeUInt8(value: number, offset: number, noAssert?: boolean): number;

    /**
     * Writes signed 8-bit value to buf at the specified offset
     * @param value Number to be written to buf
     * @param offset Where to start writing
     * @param noAssert Skip value and offset validation
     * @return offset plus number of bytes written
     */
    writeInt8(value: number, offset: number, noAssert?: boolean): number;

    /**
     * Writes unsigned 16-bit value to buf at the specified offset with big-endian format
     * @param value Number to be written to buf
     * @param offset Where to start writing
     * @param noAssert Skip value and offset validation
     * @return offset plus number of bytes written
     */
    writeUInt16BE(value: number, offset: number, noAssert?: boolean): number;

    /**
     * Writes unsigned 16-bit value to buf at the specified offset with little-endian format
     * @param value Number to be written to buf
     * @param offset Where to start writing
     * @param noAssert Skip value and offset validation
     * @return offset plus number of bytes written
     */
    writeUInt16LE(value: number, offset: number, noAssert?: boolean): number;

    /**
     * Writes signed 16-bit value to buf at the specified offset with big-endian format
     * @param value Number to be written to buf
     * @param offset Where to start writing
     * @param noAssert Skip value and offset validation
     * @return offset plus number of bytes written
     */
    writeInt16BE(value: number, offset: number, noAssert?: boolean): number;

    /**
     * Writes signed 16-bit value to buf at the specified offset with little-endian format
     * @param value Number to be written to buf
     * @param offset Where to start writing
     * @param noAssert Skip value and offset validation
     * @return offset plus number of bytes written
     */
    writeInt16LE(value: number, offset: number, noAssert?: boolean): number;

    /**
     * Writes unsigned 32-bit value to buf at the specified offset with big-endian format
     * @param value Number to be written to buf
     * @param offset Where to start writing
     * @param noAssert Skip value and offset validation
     * @return offset plus number of bytes written
     */
    writeUInt32BE(value: number, offset: number, noAssert?: boolean): number;

    /**
     * Writes unsigned 32-bit value to buf at the specified offset with little-endian format
     * @param value Number to be written to buf
     * @param offset Where to start writing
     * @param noAssert Skip value and offset validation
     * @return offset plus number of bytes written
     */
    writeUInt32LE(value: number, offset: number, noAssert?: boolean): number;

    /**
     * Writes signed 32-bit value to buf at the specified offset with big-endian format
     * @param value Number to be written to buf
     * @param offset Where to start writing
     * @param noAssert Skip value and offset validation
     * @return offset plus number of bytes written
     */
    writeInt32BE(value: number, offset: number, noAssert?: boolean): number;

    /**
     * Writes signed 32-bit value to buf at the specified offset with little-endian format
     * @param value Number to be written to buf
     * @param offset Where to start writing
     * @param noAssert Skip value and offset validation
     * @return offset plus number of bytes written
     */
    writeInt32LE(value: number, offset: number, noAssert?: boolean): number;

    /**
     * Writes 32-bit float value to buf at the specified offset with big-endian format
     * @param value Number to be written to buf
     * @param offset Where to start writing
     * @param noAssert Skip value and offset validation
     * @return offset plus number of bytes written
     */
    writeFloatBE(value: number, offset: number, noAssert?: boolean): number;

    /**
     * Writes 32-bit float value to buf at the specified offset with little-endian format
     * @param value Number to be written to buf
     * @param offset Where to start writing
     * @param noAssert Skip value and offset validation
     * @return offset plus number of bytes written
     */
    writeFloatLE(value: number, offset: number, noAssert?: boolean): number;

    /**
     * Writes 64-bit double value to buf at the specified offset with big-endian format
     * @param value Number to be written to buf
     * @param offset Where to start writing
     * @param noAssert Skip value and offset validation
     * @return offset plus number of bytes written
     */
    writeDoubleBE(value: number, offset: number, noAssert?: boolean): number;

    /**
     * Writes 64-bit double value to buf at the specified offset with little-endian format
     * @param value Number to be written to buf
     * @param offset Where to start writing
     * @param noAssert Skip value and offset validation
     * @return offset plus number of bytes written
     */
    writeDoubleLE(value: number, offset: number, noAssert?: boolean): number;

    /**
     * Writes byteLength bytes of unsigned value to buf at the specified offset with big-endian format (Up to 48 bits accuracy)
     * @param value Number to be written to buf
     * @param offset Where to start writing
     * @param byteLength How many bytes to write
     * @param noAssert Skip value and offset validation
     * @return offset plus number of bytes written
     */
    writeUIntBE(value: number, offset: number, byteLength: number, noAssert?: boolean): number;

    /**
     * Writes byteLength bytes of unsigned value to buf at the specified offset with little-endian format (Up to 48 bits accuracy)
     * @param value Number to be written to buf
     * @param offset Where to start writing
     * @param byteLength How many bytes to write
     * @param noAssert Skip value and offset validation
     * @return offset plus number of bytes written
     */
    writeUIntLE(value: number, offset: number, byteLength: number, noAssert?: boolean): number;

    /**
     * Writes byteLength bytes of signed value to buf at the specified offset with big-endian format (Up to 48 bits accuracy)
     * @param value Number to be written to buf
     * @param offset Where to start writing
     * @param byteLength How many bytes to write
     * @param noAssert Skip value and offset validation
     * @return offset plus number of bytes written
     */
    writeIntBE(value: number, offset: number, byteLength: number, noAssert?: boolean): number;

    /**
     * Writes byteLength bytes of signed value to buf at the specified offset with little-endian format (Up to 48 bits accuracy)
     * @param value Number to be written to buf
     * @param offset Where to start writing
     * @param byteLength How many bytes to write
     * @param noAssert Skip value and offset validation
     * @return offset plus number of bytes written
     */
    writeIntLE(value: number, offset: number, byteLength: number, noAssert?: boolean): number;

    /**
     * Returns a JSON representation of buf
     */
    toJSON(): Object;

    /**
     * Returns true if both buf and otherBuffer have exactly the same bytes, false otherwise
     * @param otherBuffer A Buffer to compare to
     */
    equals(otherBuffer: Buffer): boolean;

    /**
     * Compares buf with target and returns a number indicating whether buf comes becore, after, or it the same
     * as target in sort order. Comparison is based on the acutual sequence of bytes in each Buffer
     * @param target A Buffer to compare to
     * @param targetStart The offset within target at which to begin comparison
     * @param targetEnd The offset with target at which to end comparison (not inclusive)
     * @param sourceStart The offset within buf at which to begin comparison
     * @param sourceEnd The offset within buf at which to end comparison (not inclusive)
     * @return 0: target is the same as buf, 1: target should come before buf when sorted, -1: target should come after buf when sorted
     */
    compare(target: Buffer, targetStart?: number, targetEnd?: number, sourceStart?: number, sourceEnd?: number): number;

    /**
     * Copies data from a region of buf to a region in target even if the target memory region overlaps with buf
     * @param target A Buffer or Uint8Array to copy into
     * @param targetStart The offset within target at which to begin copying to
     * @param sourceStart The offset within buf at which to begin copying from
     * @param sourceEnd The offset within buf at which to stop copying (not inclusive)
     * @return The number of bytes copied
     */
    copy(target: Buffer, targetStart?: number, sourceStart?: number, sourceEnd?: number): number;

    /**
     * Returns a new Buffer that references the same memory as the original,
     * but offset and cropped by the start and end indices
     * @param start Where the new Buffer will start
     * @param end Where the new Buffer will end (not inclusive)
     */
    slice(start?: number, end?: number): Buffer;

    /**
     * Writes string to buf at the offset according to the specified character encoding
     * @param string String to be written to buf
     * @param offset Where to start writing string
     * @param length How many bytes to write
     * @param encoding The character encoding of string (only "utf8" is accepted in Duktape)
     * @return Number of bytes written
     */
    write(string: string, offset?: number, length?: number, encoding?: string): number;
}

interface BufferConstructor {
    readonly prototype: Buffer;

    /**
     * Allocates a new buffer using an array of octets.
     * @param array The octets to store
     */
    new (array: Uint8Array): Buffer;

    /**
     * Copies the passed buffer data onto a new Buffer instance.
     * @param buffer The buffer to copy
     */
    new (buffer: Buffer): Buffer;

    /**
     * Allocates a new buffer containing the given string
     * @param str String to encode
     * @param encoding Encoding to use (optional)
     */
    new (str: string, encoding?: string): Buffer;

    /**
     * Returns true if the encoding is a valid encoding argument, or false otherwise.
     * @param encoding Encoding to test
     */
    isEncoding(encoding: string): boolean;

    /**
     * Tests if obj is a Buffer
     * @param obj Object to test
     */
    isBuffer(obj: any): obj is Buffer;

    /**
     * Gives the actual byte length of a string. encoding defaults to 'utf8'.
     * This is not the same as String.prototype.length since that returns
     * the number of characters in a string.
     * @param string String to encode
     * @param encoding Encoding to use (optional)
     */
    byteLength(string: string, encoding?: string): number;

    /**
     * Returns a buffer which is the result of concatenating all the buffers
     * in the list together.
     * @param list List of Buffer objects to concat
     * @param totalLength Total length of the buffers when concatenated
     */
    concat(list: Buffer[], totalLength?: number): Buffer;

    /**
     * The same as buf1.compare(buf2). Useful for sorting an Array of Buffers:
     * @param buf1
     * @param buf2
     */
    compare(buf1: Buffer, buf2: Buffer): number;
}

/**
 * Raw byte data store (based on Buffer of Node.js v0.12.1)
 */
declare const Buffer: BufferConstructor;

interface TextEncoder {
    readonly encoding: "utf-8";
    encode(input?: string): Uint8Array;
}

interface TextEncoderConstructor {
    new (): TextEncoder;
}

/**
 * Encoder for string to Uint8Array by UTF-8 encoding
 * based WHATWG Encoding API
 */
declare const TextEncoder: TextEncoderConstructor;

interface TextDecoderOptions {
    fatal: boolean;
    ignoreBOM: boolean;
}

interface TextDecodeOptions {
    stream: boolean;
}

interface TextDecoder {
    readonly encoding: "utf-8";
    readonly fatal: boolean;
    readonly ignoreBOM: boolean;
    decode(input?: ArrayBufferView | ArrayBuffer, options?: TextDecodeOptions);
}

interface TextDecoderConstructor {
    readonly prototype: TextDecoder;
    new (label?: "utf-8", options?: TextDecoderOptions): TextDecoder;
}

/**
 * Decoder for Uint8Array to string by UTF-8 encoding
 * based WHATWG Encoding API
 */
declare const TextDecoder: TextDecoderConstructor;
