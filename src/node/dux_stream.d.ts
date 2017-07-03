declare namespace Dux {
    class Stream extends EventEmitter {
        /**
         * Circular reference to Stream
         */
        static "Stream": typeof Stream;
    }

    interface WritableStreamOptions {
        highWaterMark?: number;
        decodeStrings?: string;
        objectMode?: boolean;
        write?: Function;
        writev?: Function;
        destroy?: Function;
        final?: Function;
    }

    type StreamChunk = string | Buffer | Uint8Array | any;

    interface WritableStream {
        cork(): void;
        end(chunk?: StreamChunk, encoding?: string, callback?: Function): void;
        setDefaultEncoding(encoding: string): Stream.Writable;
        uncork(): void;
        write(chunk: StreamChunk, encoding?: string, callback?: Function): boolean;
    }

    interface ReadableStreamOptions {
    }

    interface PipeOptions {
        end?: boolean;
    }

    interface ReadableStream {
        isPaused(): boolean;
        pause(): Stream.Readable;
        pipe(destination: Stream.Writable, options?: PipeOptions): void;
        read(size?: number): string | Buffer;
        resume(): Stream.Readable;
        setEncoding(encoding: string): Stream.Readable;
        unpipe(destination?: Stream.Writable): void;
        unshift(chunk: StreamChunk): void;
    }

    module Stream {
        class Writable extends Stream implements WritableStream {
            cork(): void;
            end(chunk?: StreamChunk, encoding?: string, callback?: Function): void;
            setDefaultEncoding(encoding: string): Writable;
            uncork(): void;
            write(chunk: StreamChunk, encoding?: string, callback?: Function): boolean;
        }

        class Readable extends Stream implements ReadableStream {
            isPaused(): boolean;
            pause(): Readable;
            pipe(destination: Writable, options?: PipeOptions): void;
            read(size?: number): string | Buffer;
            resume(): Readable;
            setEncoding(encoding: string): Readable;
            unpipe(destination?: Writable): void;
            unshift(chunk: StreamChunk): void;
        }

        class Duplex extends Stream
            implements WritableStream, ReadableStream {

            cork(): void;
            end(chunk?: StreamChunk, encoding?: string, callback?: Function): void;
            setDefaultEncoding(encoding: string): Writable;
            uncork(): void;
            write(chunk: StreamChunk, encoding?: string, callback?: Function): boolean;

            isPaused(): boolean;
            pause(): Readable;
            pipe(destination: Writable, options?: PipeOptions): void;
            read(size?: number): string | Buffer;
            resume(): Readable;
            setEncoding(encoding: string): Readable;
            unpipe(destination?: Writable): void;
            unshift(chunk: StreamChunk): void;
        }
    }
}
declare module "stream" {
    export = Dux.Stream;
}
