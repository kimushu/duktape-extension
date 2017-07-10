declare namespace Dux {
    interface SPIConnectionConstructor {
        readonly prototype: SPIConnection;
    }

    type SPIWriteData = ArrayBuffer | Buffer | Array<number> | string;

    interface SPIConnection {
        /**
         * Read bytes from SPI device
         * @param readLen Number of bytes to read
         * @param filler Written value to write (Optional. Default 0xff)
         */
        read(readLen: number, filler?: number): Promise<Buffer>;

        /**
         * Read bytes from SPI device
         * @param readLen Number of bytes to read
         * @param callback Callback
         * @param filler Written value to write (Optional. Default 0xff)
         */
        read(readLen: number, callback: (error: Error, readData: Buffer) => void, filler?: number): void;

        /**
         * Write bytes to SPI device
         * @param writeData The buffer stores octets to write
         */
        write(writeData: SPIWriteData): Promise<void>;

        /**
         * Write bytes to SPI device
         * @param writeData The buffer stores octets to write
         * @param callback Callback
         */
        write(writeData: SPIWriteData, callback: (error: Error) => void): void;

        /**
         * Write bytes, and then read bytes from SPI device
         * @param writeData The buffer stores octets to write
         * @param readLen Number of bytes to read
         * @param filler Written value to write in read phase (Optional. Default 0xff)
         */
        transfer(writeData: SPIWriteData, readLen: number, filler?: number): Promise<Buffer>;

        /**
         * Write bytes, and then read bytes from SPI device
         * @param writeData The buffer stores octets to write
         * @param readLen Number of bytes to read
         * @param callback Callback
         * @param filler Written value to write in read phase (Optional. Default 0xff)
         */
        transfer(writeData: SPIWriteData, readLen: number, callback: (error: Error, readData: Buffer) => void, filler?: number): void;

        /**
         * Write and read bytes simultaneously. The number of bytes to read is the same as the number of bytes to write.
         * @param writeData The buffer stores octets to write
         */
        exchange(writeData: ArrayBuffer|Buffer|Uint8Array|string): Promise<Buffer>;

        /**
         * Write and read bytes simultaneously. The number of bytes to read is the same as the number of bytes to write.
         * @param writeData The buffer stores octets to write
         * @param callback Callback
         */
        exchange(writeData: ArrayBuffer|Buffer|Uint8Array|string, callback: (error: Error, readData: Buffer) => void): void;

        /** Bitrate */
        bitrate: number;

        /** If true, LSB (Least Significant Bit) will be sent the first. */
        lsbFirst: boolean;

        /** SPI signal mode */
        mode: 0|1|2|3;

        /** If true, MSB (Most Significant Bit) will be sent the first. */
        msbFirst: boolean;

        readonly slaveSelect: any; /* FIXME */
    }
}

declare module "hardware" {
    const SPIConnection: Dux.SPIConnectionConstructor;
    type SPIConnection = Dux.SPIConnection;
}
