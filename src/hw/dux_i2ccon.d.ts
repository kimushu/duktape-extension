declare namespace Dux {
    interface I2CConnectionConstructor {
        readonly prototype: I2CConnection;
    }

    type I2CWriteData = ArrayBuffer | Buffer | Array<number> | string;

    interface I2CConnection {
        /**
         * Read bytes from I2C device
         * @param readLen Number of bytes to read
         * @param filler Written value to write (Optional. Default 0xff)
         */
        read(readLen: number): Promise<Buffer>;

        /**
         * Read bytes from I2C device
         * @param readLen Number of bytes to read
         * @param callback Callback
         */
        read(readLen: number, callback: (error: Error, readData: Buffer) => void): void;

        /**
         * Write bytes to I2C device
         * @param writeData The buffer stores octets to write
         */
        write(writeData: I2CWriteData): Promise<void>;

        /**
         * Write bytes to I2C device
         * @param writeData The buffer stores octets to write
         * @param callback Callback
         */
        write(writeData: I2CWriteData, callback: (error: Error) => void): void;

        /**
         * Write bytes, and then read bytes from I2C device.
         * This uses START-(write)-RESTART-(read)-STOP sequence.
         * @param writeData The buffer stores octets to write
         * @param readLen Number of bytes to read
         */
        transfer(writeData: I2CWriteData, readLen: number): Promise<Buffer>;

        /**
         * Write bytes, and then read bytes from I2C device.
         * This uses START-(write)-RESTART-(read)-STOP sequence.
         * @param writeData The buffer stores octets to write
         * @param readLen Number of bytes to read
         * @param callback Callback
         */
        transfer(writeData: I2CWriteData, readLen: number, callback: (error: Error, readData: Buffer) => void): void;

        /** Bitrate */
        bitrate: number;

        /** Slave address in 7-bit representation (0~127) */
        readonly slaveAddress: number;
    }
}

declare module "hardware" {
    const I2CConnection: Dux.I2CConnectionConstructor;
    type I2CConnection = Dux.I2CConnection;
}
