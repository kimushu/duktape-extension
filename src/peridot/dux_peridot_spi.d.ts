/// <reference path="./dux_peridot.d.ts"/>
/// <reference path="../hw/dux_spicon.d.ts"/>

declare namespace Dux {
    interface PeridotSPIPins {
        /** SCLK (Clock) */
        sclk: PeridotPin;

        /** MOSI (Data, Master to Slave) */
        mosi: PeridotPin;

        /** MISO (Data, Slave to Master) */
        miso: PeridotPin;
    }

    interface PeridotSPIPinsWithSS extends PeridotSPIPins {
        /** SS (Slave select [Active low]) */
        ss_n: PeridotPin;
    }

    interface PeridotSPIConstructor {
        readonly prototype: PeridotSPI;

        /**
         * Construct a new SPI connector with given pins (except for slave select)
         */
        new (pins: PeridotSPIPins): PeridotSPI;

        /**
         * Connect to SPI device with given pins
         */
        connect(pins: PeridotSPIPinsWithSS, bitrate?: number): SPIConnection;
    }

    interface PeridotSPI {
        /**
         * Connect to SPI device with given slave select pin
         */
        connect(ss_n_pin: number | string, bitrate?: number): SPIConnection;
    }
}

declare module "peridot" {
    const SPI: Dux.PeridotSPIConstructor;
    type SPI = Dux.PeridotSPI;
}
