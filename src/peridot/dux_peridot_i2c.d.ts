/// <reference path="./dux_peridot.d.ts"/>
/// <reference path="../hw/dux_i2ccon.d.ts"/>

declare namespace Dux {
    interface PeridotI2CPins {
        /* SCL (Clock) */
        scl: PeridotPin;

        /* SDA (Data) */
        sda: PeridotPin;
    }

    interface PeridotI2CConstructor {
        readonly prototype: PeridotI2C;

        /**
         * Construct a new I2C connector with given pins
         */
        new (pins: PeridotI2CPins): PeridotI2C;

        /**
         * Connect to I2C device with given pins and slave address
         */
        connect(pins: PeridotI2CPins, slaveAddress: number, bitrate?: number): I2CConnection;
    }

    interface PeridotI2C {
        /**
         * Connect to I2C device with given slave address
         */
        connect(slaveAddress: number, bitrate?: number): I2CConnection;
    }
}

declare module "peridot" {
    const I2C: Dux.PeridotI2CConstructor;
    type I2C = Dux.PeridotI2C;
}
