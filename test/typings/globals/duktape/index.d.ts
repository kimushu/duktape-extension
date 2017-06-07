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

interface Buffer extends Uint8Array {}

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
