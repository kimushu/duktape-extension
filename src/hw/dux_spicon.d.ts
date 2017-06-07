declare namespace Hardware {
    export class SPIConnection {
        read(readLen: number, filler?: number): Promise<Buffer>;
        read(readLen: number, callback: (error: Error, readData: Buffer) => void, filler?: number): void;
        transfer(writeData: ArrayBuffer|Buffer|Uint8Array|string, readLen: number, filler?: number): Promise<Buffer>;
        transfer(writeData: ArrayBuffer|Buffer|Uint8Array|string, readLen: number, callback: (error: Error, readData: Buffer) => void, filler?: number): void;
        exchange(writeData: ArrayBuffer|Buffer|Uint8Array|string): Promise<Buffer>;
        exchange(writeData: ArrayBuffer|Buffer|Uint8Array|string, callback: (error: Error, readData: Buffer) => void): void;
        write(writeData: ArrayBuffer|Buffer|Uint8Array|string): Promise<void>;
        write(writeData: ArrayBuffer|Buffer|Uint8Array|string, callback: (error: Error) => void): void;

        bitrate: number;
        lsbFirst: boolean;
        mode: 0|1|2|3;
        msbFirst: boolean;
        readonly slaveSelect: any; /* FIXME */
    }
}
