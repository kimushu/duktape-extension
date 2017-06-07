declare namespace Hardware {
    export class I2CConnection {
        read(readLen: number): Promise<Buffer>;
        read(readLen: number, callback: (error: Error, readData: Buffer) => void): void;
        write(writeData: ArrayBuffer|Buffer|Uint8Array|string): Promise<void>;
        write(writeData: ArrayBuffer|Buffer|Uint8Array|string, callback: (error: Error) => void): void;
        transfer(writeData: ArrayBuffer|Buffer|Uint8Array|string, readLen: number): Promise<Buffer>;
        transfer(writeData: ArrayBuffer|Buffer|Uint8Array|string, readLen: number, callback: (error: Error, readData: Buffer) => void): void;

        bitrate: number;
        readonly slaveAddress: number;
    }
}
